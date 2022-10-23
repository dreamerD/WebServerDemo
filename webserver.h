#ifndef WEBSERVER_H
#define WEBSERVER_H
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include "http/http_conn.h"
#include "sqlpool/sql_conn_pool.h"
#include "threadpool/threadpool.h"
#include "timer/timer.h"

using std::string;
const int MAX_FD = 65536;
const int MAX_EVENT_NUMBER = 10000;
const int TIMESLOT = 5;

class WebServer {
   public:
    WebServer();
    ~WebServer();
    void Init(int port, string user, string passWord, string databaseName, int log_write, int opt_linger, int trigmode, int sql_num, int thread_num, int close_log, int actor_model);
    void ThreadPool();
    void SqlPool();
    void LogWrite();
    void TrigMode();
    void EventListen();
    void EventLoop();
    void Timer(int connfd, struct sockaddr_in client_address);
    void AdjustTimer(MyTimer* timer);
    void DealTimer(MyTimer* timer, int sockfd);
    void DealClientData();
    void DealSignal(bool& timeout, bool& stop);
    void DealRead(int sockfd);
    void DealWrite(int sockfd);

   public:
    int port_;            //端口
    string root_;         //根目录
    bool is_closed_log_;  // 是否启动日志
    int actormodel_;      // Reactor or Proactor

    // 网络
    int pipefd_[2];    // 相互连接的套接字
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
    epoll_event events_[MAX_EVENT_NUMBER];

    int listenfd_;        //监听套接字
    int OPT_LINGER_;      //是否优雅下线
    int TRIGMode_;        // ET/LT
    int LISTENTrigmode_;  // ET/LT
    int CONNTrigmode_;    // ET/LT

    //定时器
    ClientData* users_timer_;
};
#endif
