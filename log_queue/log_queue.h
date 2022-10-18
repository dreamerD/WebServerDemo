#ifndef LOG_QUEUE_H
#define LOG_QUEUE_H
#include "lock/locker.h"
template <typename T>
class LogQueue {
   public:
    LogQueue(int max_size = 100);
    ~LogQueue();
    void Clear();

    bool IsFull();
    bool IsEmpty();
    bool Front(T& value);
    bool Back(T& value);
    int Size();
    int MaxSize();
    void Push(const T& item);
    void Pop(T& item);

   private:
    MyMutex mymutex_;
    MySem producer_sem_;
    MySem consumer_sem_;

    T* array_;
    int size_;
    int max_size_;
    int front_;
    int back_;
};
#endif