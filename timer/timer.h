#ifndef TIMER_H
#define TIMER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <memory>
#include <queue>

class MyTimer;
// 连接信息
struct ClientData {
    sockaddr_in address_;
    int sockfd_;
    MyTimer* mytimer_;
};

struct myGreater {
    bool operator()(MyTimer* a, MyTimer* b) {
        return a->expire_ < b->expire_;  //
    }
};
class MyTimer {
   public:
    MyTimer()
        : prev_(nullptr), next_(nullptr), is_old_(false) {}
    time_t expire_;  // timeout
    ClientData* data_;
    void (*cb_func)(ClientData*);
    MyTimer* prev_;
    MyTimer* next_;
    bool is_old_;
};

class MyTimerList {
   public:
    MyTimerList() = default;
    ~MyTimerList();

    // 添加
    void AddTimer(MyTimer* timer);
    // 调整
    void AdjustTimer(MyTimer* timer);
    // 删除
    void DelTimer(MyTimer* timer);
    // 定时任务
    void Tick();

   private:
    std::priority_queue<MyTimer*, std::vector<MyTimer*>, myGreater> queue_;
};
#endif