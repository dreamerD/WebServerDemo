#ifndef LOCKER_H
#define LOCKER_H
#include <pthread.h>
#include <semaphore.h>
#include <cassert>
class MySem {
   public:
    MySem(unsigned num = 0);
    ~MySem();
    void Wait();
    void Post();

   private:
    sem_t sem_;
};

class MyMutex {
   public:
    MyMutex();
    ~MyMutex();
    void Lock();
    void Unlock();

   private:
    pthread_mutex_t mutex_;
};
#endif