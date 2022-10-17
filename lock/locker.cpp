#include "locker.h"

MySem::MySem(unsigned num) {
    int rval = sem_init(&sem_, 0, num);
    assert(rval == 0);
}

MySem::~MySem() {
    sem_destroy(&sem_);
}

void MySem::Wait() {
    sem_wait(&sem_);
}

void MySem::Post() {
    sem_post(&sem_);
}

MyMutex::MyMutex() {
    int rval = pthread_mutex_init(&mutex_, NULL);
    assert(rval == 0);
}

MyMutex::MyMutex() {
    pthread_mutex_destroy(&mutex_);
}

void MyMutex::Lock() {
    pthread_mutex_lock(&mutex_);
}

void MyMutex::Unlock() {
    pthread_mutex_unlock(&mutex_);
}
