#include "threadpool.h"
template <typename T>
MyThreadPool<T>::MyThreadPool(int thread_number, int max_requests_number) {
    assert(thead_number > 0 && max_requests_number > 0);
    this->thread_number_ = thread_number;
    this->max_requests_number_ = max_requests_number;
    this->threads_ = new pthread_t[thread_number];

    for (int i = 0; i < thread_number_; i++) {
        pthread_t cur_pthread;
        assert(pthread_create(&cur_pthread, NULL, worker, this) == 0);
        assert(pthread_detach(&cur_pthread) == 0);
        threads_[i] = cur_pthread;
    }
}

template <typename T>
bool MyThreadPool<T>::AppendTask(T* request) {
    mymutex_.Lock();
    if (work_queue.size() >= max_requests_number_) {
        mymutex_.Unlock();
        return false;
    }
    work_queue.push_back(request);
    mymutex_.Unlock();
    mysem_.Post();
    return true;
}

template <typename T>
void* MyThreadPool<T>::worker(void* arg) {
    MyThreadPool<T>* pool = (MyThreadPool<T>*)arg;

    while (true) {
        pool->mysem_.Wait();
        pool->mymutex_.Lock();

        if (pool->work_queue.empty()) {
            continue;
        }
        T* request = pool->work_queue.front();
        pool->work_queue.pop_front();
        pool->mymutex_.Unlock();
        if (request == nullptr) {
            continue;
        }
        // Reactor
        // 0 是 读事件
        if (0 == request->state_) {
            if (request->read_once()) {
                // request->improv_ = 1;
                // connectionRAII mysqlcon(&request->mysql, m_connPool);
                request->process();
            }
            // else {
            //     request->improv_ = 1;
            //     request->timer_flag = 1;
            // }
        } else {
            request->write();
        }
    }
    return pool;
}