#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cassert>
#include <list>
#include <memory>

#include "lock/locker.h"
template <typename T>
class ThreadPool {
   public:
    ThreadPool(int thread_number = 8, int max_requests_number = 10000);
    ~ThreadPool() = default;
    bool AppendTask(T* request);

   private:
    static void* worker(void* arg);

   private:
    int thread_number_;
    int max_requests_number_;
    pthread_t* threads_;
    std::list<T*> work_queue;
    MyMutex mymutex_;
    MySem mysem_;
};
#endif