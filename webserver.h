#ifndef WEBSERVER_H
#define WEBSERVER_H
#include <sys/epoll.h>
#include <sys/socket.h>
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
    void init();
    void ThreadPool();
    void SqlPool();
    void LogWrite();
    void trigMode();
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
    int pipe_fd_[2];   // 相互连接的套接字
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

    int listnfd_;         //监听套接字
    int OPT_LINGER_;      //是否优雅下线
    int TRIGMode_;        // ET/LT
    int LISTENTrigmode_;  // ET/LT
    int CONNTrigmode_;    // ET/LT

    //定时器
    ClientData* users_timer_;
};
#endif
