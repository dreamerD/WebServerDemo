#ifndef UTILS_H
#define UTILS_H
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cassert>
#include "../http/http_conn.h"
#include "../timer/timer.h"
#include "fcntl.h"
class Utils {
   public:
    static void Init(int timeslot);

    //对文件描述符设置非阻塞
    static int Setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    static void Addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void SigHandler(int sig);

    //设置信号函数
    static void Addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    static void TimerHandler();

    static void ShowError(int connfd, const char* info);

   public:
    static int* pipefd_;              //管道id
    static int epollfd_;              // epollfd
    static int TIMESLOT_;             //最小时间间隙
    static MyTimerList mytimerlist_;  //定时器链表
};
void cb_func(ClientData* user_data);
#endif