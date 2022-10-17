#include "threadpool.h"

template <typename T>
ThreadPool<T>::ThreadPool(int thread_number, int max_requests_number) {
    assert(thead_number > 0 && max_requests_number > 0);
    this->thread_number_ = thread_number;
    this->max_requests_number_ = max_requests_number;
    this->threads_ = std::make_unique<pthread_t>(thread_number);

    for (int i = 0; i < thread_number_; i++) {
        pthread_t cur_pthread;
        assert(pthread_create(&cur_pthread, NULL, worker, this) == 0);
        assert(pthread_detach(&cur_pthread) == 0);
        threads_[i] = cur_pthread;
    }
}

template <typename T>
bool ThreadPool<T>::AppendTask(std::shared_ptr<T>& request) {
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
void* ThreadPool<T>::worker(void* arg) {
    ThreadPool<T>* pool = (ThreadPool<T>*)arg;

    while (true) {
        pool->mysem_.Wait();
        pool->mymutex_.Lock();

        if (pool->work_queue.empty()) {
            continue;
        }
        std::shared_ptr<T> request = pool->work_queue.front();
        pool->work_queue.pop_front();
        pool->mymutex_.Unlock();
        if (request == nullptr) {
            continue;
        }
        request->run();
    }
    return pool;
}