#include "utils.h"
void Utils::Init(int timeslot) {
    TIMESLOT_ = timeslot;
}

//对文件描述符设置非阻塞
int Utils::Setnonblocking(int fd) {
    // fcntl可以获取/设置文件描述符性质
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，是否选择开启EPOLLONESHOT
void Utils::Addfd(int epollfd, int fd, bool one_shot, int TRIGMode) {
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode) {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    } else {
        event.events = EPOLLIN | EPOLLRDHUP;
    }

    //如果对描述符socket注册了EPOLLONESHOT事件，
    //那么操作系统最多触发其上注册的一个可读、可写或者异常事件，且只触发一次。
    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    Setnonblocking(fd);
}

//信号处理函数
void Utils::SigHandler(int sig) {
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(pipefd_[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
// https://www.cnblogs.com/black-mamba/p/6876320.html?utm_source=itdadao&utm_medium=referral
void Utils::Addsig(int sig, void(handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::TimerHandler() {
    mytimerlist_.Tick();
    //最小的时间单位为5s
    alarm(TIMESLOT_);
}

void Utils::ShowError(int connfd, const char* info) {
    send(connfd, info, strlen(info), 0);
    close(connfd);
}
//将事件重置为EPOLLONESHOT
void Utils::Modfd(int epollfd, int fd, int ev, int TRIGMode) {
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    else
        event.events = ev | EPOLLONESHOT | EPOLLRDHUP;

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}
int Utils::epollfd_ = 0;

class Utils;
//定时器回调函数:从内核事件表删除事件，关闭文件描述符，释放连接资源
void cb_func(ClientData* user_data) {
    //删除非活动连接在socket上的注册事件
    epoll_ctl(Utils::epollfd_, EPOLL_CTL_DEL, user_data->sockfd_, 0);
    assert(user_data);
    //删除非活动连接在socket上的注册事件
    close(user_data->sockfd_);
    //减少连接数
    HttpConn::user_count_--;
}