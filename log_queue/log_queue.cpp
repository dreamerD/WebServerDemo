#include "log_queue.h"
#include <cassert>
template <typename T>
LogQueue<T>::LogQueue(int max_size = 1000) {
    assert(max_size > 0);

    max_size_ = max_size;
    array_ = new T[max_size];
    size_ = 0;
    front_ = -1;
    back_ = -1;
}
template <typename T>
LogQueue<T>::~LogQueue() {
    mymutex_.Lock();
    if (array_ != nullptr)
        delete[] array_;

    mymutex_.Unlock();
}
template <typename T>
void LogQueue<T>::Clear() {
    mymutex_.Lock();
    size_ = 0;
    front = -1;
    back_ = -1;
    mymutex_.Unlock();
}

template <typename T>
bool LogQueue<T>::IsFull() {
    bool flag = false;
    mymutex_.Lock();
    flag = (size_ >= max_size_);
    mymutex_.Unlock();
    return flag;
}

template <typename T>
bool LogQueue<T>::IsEmpty() {
    bool flag = false;
    mymutex_.Lock();
    flag = (size_ == 0);
    mymutex_.Unlock();
    return flag;
}

template <typename T>
bool LogQueue<T>::Front(T& value) {
    bool flag = false;
    mymutex_.Lock();
    flag = (size_ == 0);
    if (flag) {
        mymutex_.Unlock();
        return false;
    }
    value = array_[front_];
    mymutex_.Unlock();
    return true;
}

template <typename T>
bool LogQueue<T>::Back(T& value) {
    bool flag = false;
    mymutex_.Lock();
    flag = (size_ == 0);
    if (flag) {
        mymutex_.Unlock();
        return false;
    }
    value = array_[back_];
    mymutex_.Unlock();
    return true;
}

template <typename T>
int LogQueue<T>::Size() {
    int tmp = 0;
    mymutex_.Lock();
    tmp = size_;
    mymutex_.Unlock();
    return tmp;
}

template <typename T>
int LogQueue<T>::MaxSize() {
    int tmp = 0;
    mymutex_.Lock();
    tmp = max_size_;
    mymutex_.Unlock();
    return tmp;
}

template <typename T>
void LogQueue<T>::Push(const T& item) {
    mymutex_.Lock();
    while (size_ >= max_size_) {
        mymutex_.Unlock();
        consumer_sem_.Wait();
        mymutex_.Lock();
    }
    back_ = (back_ + 1) % max_size_;
    array_[back_] = item;
    size_++;
    producer_sem_.Post();
    mymutex_.Unlock();
}

template <typename T>
void LogQueue<T>::Pop(T& item) {
    mymutex_.Lock();
    while (size_ <= 0) {
        mymutex_.Unlock();
        producer_sem_.Wait();
        mymutex_.Lock();
    }
    front_ = (front_ + 1) % max_size_;
    item = array_[front];
    size_--;
    consumer_sem_.Post();
    mymutex_.Unlock();
}