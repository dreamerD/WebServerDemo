#include "webserver.h"
#include "utils/utils.h"
/*
class WebServer {
   public:
    WebServer();
    ~WebServer();
    void init();
    void ThreadPool();
    void SqlPool();
    void LogWrite();
    void TrigMode();
    void EventListen();
    void EventLoop();
    void Timer();
    void AdjustTimer();
    void DelTimer();
    void DealClientData();
    void DealSignal();
    void DealRead();
    void DealWrite();

   public:
    int port_;            //端口
    string root_;         //根目录
    bool is_closed_log_;  // 是否启动日志
    int actormodel_;      // Reactor or Proactor

    // 网络
    int pipefd_[2];   // 相互连接的套接字
    int epollfd_;      // epoll
    HttpConn* users_;  // http连接

    //数据库
    MySQLConnPool* mysql_conn_pool_;
    string conn_user_;
    string conn_password_;
    string database_name_;
    int sqL_conn_number_;

    //线程池
    MyThreadPool<HttpConn>* pool_;
    int thread_num_;

    // epoll_event
    epoll_event events[MAX_EVENT_NUMBER];

    int listenfd_;         //监听套接字
    int OPT_LINGER_;      //是否优雅下线
    int TRIGMode_;        // ET/LT
    int LISTENTrigmode_;  // ET/LT
    int CONNTrigmode_;    // ET/LT

    //定时器
    ClientData* users_timer_;
};
*/

WebServer::WebServer() {
    // http conn
    users_ = new HttpConn[MAX_FD];

    //根目录
    char server_path[100];
    getcwd(server_path, 100);
    root_ = server_path;
    root_ += "/root";

    //定时器
    users_timer_ = new ClientData[MAX_FD];
}

WebServer::~WebServer() {
    close(epollfd_);
    close(listenfd_);
    close(pipefd_[0]);
    close(pipefd_[1]);
    delete[] users_;
    delete[] users_timer_;
    delete pool_;
}

void WebServer::Init(int port, string user, string passWord, string databaseName, int log_write, int opt_linger, int trigmode, int sql_num, int thread_num, int close_log, int actor_model) {
    port_ = port;
    conn_user_ = user;
    conn_password_ = passWord;
    database_name_ = databaseName;
    sqL_conn_number_ = sql_num;
    thread_num_ = thread_num;
    // log_write_ = log_write;
    OPT_LINGER_ = opt_linger;
    TRIGMode_ = trigmode;
    is_closed_log_ = close_log;
    actormodel_ = actor_model;
}

// 设置epoll触发模式
void WebServer::TrigMode() {
    // LT + LT
    if (0 == TRIGMode_) {
        LISTENTrigmode_ = 0;
        CONNTrigmode_ = 0;
    }
    // LT + ET
    else if (1 == TRIGMode_) {
        LISTENTrigmode_ = 0;
        CONNTrigmode_ = 1;
    }
    // ET + LT
    else if (2 == TRIGMode_) {
        LISTENTrigmode_ = 1;
        CONNTrigmode_ = 0;
    }
    // ET + ET
    else if (3 == TRIGMode_) {
        LISTENTrigmode_ = 1;
        CONNTrigmode_ = 1;
    }
}

//初始化日志系统
void WebServer::LogWrite() {
    if (0 == is_closed_log_) {
        //异步日志
        Log::GetInstance()->Init("./ServerLog", is_closed_log_, 800000, 800);
    }
}

void WebServer::SqlPool() {
    mysql_conn_pool_ = MySQLConnPool::GetInstance();
    mysql_conn_pool_->init("localhost", conn_user_, conn_password_, database_name_, 3306, sqL_conn_number_, is_closed_log_);
    // users->initMySql_result(mysql_conn_pool_);
}

void WebServer::ThreadPool() {
    pool_ = new MyThreadPool<HttpConn>(thread_num_);
}

void WebServer::EventListen() {
    listenfd_ = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd_ >= 0);
    //优雅关闭连接
    //有关setsocket函数可以参考：
    // https://baike.baidu.com/item/setsockopt/10069288?fr=aladdin
    if (0 == OPT_LINGER_) {
        struct linger tmp = {0, 1};
        setsockopt(listenfd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    } else if (1 == OPT_LINGER_) {
        struct linger tmp = {1, 1};
        setsockopt(listenfd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    }

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);

    int flag = 1;
    setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(listenfd_, (struct sockaddr*)&address, sizeof(address));
    assert(ret >= 0);
    //表示已连接和未连接的最大队列数总和为5
    ret = listen(listenfd_, 5);
    assert(ret >= 0);

    //设置服务器最小时间间隙
    Utils::Init(TIMESLOT);

    // epoll 创建内核事件表
    epollfd_ = epoll_create(5);
    assert(epollfd_ != -1);
    HttpConn::epollfd_ = epollfd_;
    Utils::Addfd(epollfd_, listenfd_, false, LISTENTrigmode_);

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd_);
    assert(ret != -1);
    Utils::Setnonblocking(pipefd_[1]);
    Utils::Addfd(epollfd_, pipefd_[0], false, 0);

    //
    Utils::pipefd_ = pipefd_;
    Utils::epollfd_ = epollfd_;

    //信号处理
    Utils::Addsig(SIGPIPE, SIG_IGN);
    Utils::Addsig(SIGALRM, Utils::SigHandler, false);
    Utils::Addsig(SIGTERM, Utils::SigHandler, false);

    alarm(TIMESLOT);
}

// 创建定时器
void WebServer::Timer(int connfd, struct sockaddr_in client_address) {
    /*初始化http连接*/
    // users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode, m_close_log, m_user, m_passWord, m_databaseName);

    // 初始化ClientData数据
    users_timer_[connfd].address_ = client_address;
    users_timer_[connfd].sockfd_ = connfd;
    // 初始化MyTimer
    MyTimer* timer = new MyTimer;
    timer->data_ = &users_timer_[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    timer->expire_ = cur + 3 * TIMESLOT;
    users_timer_[connfd].mytimer_ = timer;
    Utils::mytimerlist_.AddTimer(timer);
}

//若数据活跃，则将定时器节点往后延迟3个时间单位
void WebServer::AdjustTimer(MyTimer* timer) {
    time_t cur = time(NULL);
    timer->expire_ = cur + 3 * TIMESLOT;
    Utils::mytimerlist_.AdjustTimer(timer);

    LOG_INFO("%s", "adjust timer once");
}

// 删除定时器节点，关闭连接
void WebServer::DealTimer(MyTimer* timer, int sockfd) {
    timer->cb_func(&users_timer_[sockfd]);
    Utils::mytimerlist_.DelTimer(timer);
    LOG_INFO("close fd %d", users_timer_[sockfd].sockfd_);
}

// 处理新到的用户数据
void WebServer::DealClientData() {
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);

    // listen_fd 考虑LT
    int connfd = accept(listenfd_, (struct sockaddr*)&client_address, &client_addrlength);
    if (connfd < 0) {
        LOG_ERROR("%s:errno is:%d", "accept error", errno);
        return;
    }
    if (HttpConn::user_count_ >= MAX_FD) {
        LOG_ERROR("%s", "Internal server busy");
        return;
    }
    Timer(connfd, client_address);
}

void WebServer::DealSignal(bool& timeout, bool& stop) {
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(pipefd_[0], signals, sizeof(signals), 0);
    if (-1 == ret || 0 == ret) {
        LOG_ERROR("%s", "DealSignal failure");
        return;
    }
    for (int i = 0; i < ret; i++) {
        switch (signals[i]) {
            case SIGALRM: {
                timeout = true;
                break;
            }
            case SIGTERM: {
                stop = true;
                break;
            }
        }
    }
}

void WebServer::DealRead(int sockfd) {
    MyTimer* timer = users_timer_[sockfd].mytimer_;
    // 先做Reactor
    if (1 == actormodel_) {
        if (timer != nullptr) {
            AdjustTimer(timer);
        }
        users_[sockfd].state_ = 0;  //读任务
        pool_->AppendTask(&users_[sockfd]);
        // 还应当增加处理，若是读出错，需要删除定时器
        // 可以再来一套无名套接字
        // 这是参考代码的做法
        while (true) {
            if (1 == users_[sockfd].improv_) {
                if (1 == users_[sockfd].timer_flag_) {
                    DealTimer(timer, sockfd);
                    users_[sockfd].timer_flag_ = 0;
                }
                users_[sockfd].improv_ = 0;
                break;
            }
        }
    } else {
        ;
    }
}

void WebServer::DealWrite(int sockfd) {
    MyTimer* timer = users_timer_[sockfd].mytimer_;
    // 先做Reactor
    if (1 == actormodel_) {
        if (timer != nullptr) {
            AdjustTimer(timer);
        }
        users_[sockfd].state_ = 1;  //写任务
        pool_->AppendTask(&users_[sockfd]);
        // 还应当增加处理，若是读出错，需要删除定时器
        // 可以再来一套无名套接字
        // 这是参考代码的做法
        while (true) {
            if (1 == users_[sockfd].improv_) {
                if (1 == users_[sockfd].timer_flag_) {
                    DealTimer(timer, sockfd);
                    users_[sockfd].timer_flag_ = 0;
                }
                users_[sockfd].improv_ = 0;
                break;
            }
        }
    } else {
        ;
    }
}
// 事件循环
void WebServer::EventLoop() {
    bool timeout = false;
    bool stop = false;
    while (!stop) {
        int number = epoll_wait(epollfd_, events_, MAX_EVENT_NUMBER, -1);

        // https://blog.csdn.net/qq_39781096/article/details/107046804
        if (number < 0 && errno != EINTR) {
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = events_[i].data.fd;
            //新到的客户连接
            if (sockfd == listenfd_) {
                DealClientData();
            }
            //异常事件
            else if (events_[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                //服务器端关闭连接
                DealTimer(users_timer_[sockfd].mytimer_, sockfd);
            }
            //处理定时器事件
            else if ((sockfd == pipefd_[0]) && (events_[i].events & EPOLLIN)) {
                DealSignal(timeout, stop);
            }
            //处理客户连接上接收到的数据
            else if (events_[i].events & EPOLLIN) {
                DealRead(sockfd);
            }
            //处理客户连接上要发送的数据
            else if (events_[i].events & EPOLLOUT) {
                DealWrite(sockfd);
            }
        }
        if (timeout) {
            Utils::TimerHandler();
            LOG_INFO("%s", "timer tick");
            timeout = false;
        }
    }
    LOG_INFO("%s", "Server is stopped");
}