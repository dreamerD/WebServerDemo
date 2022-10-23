#include "timer.h"

MyTimerList::~MyTimerList() {
    while (!queue_.empty()) {
        delete queue_.top();
        queue_.pop();
    }
}

void MyTimerList::AddTimer(MyTimer* timer) {
    if (timer == nullptr) {
        return;
    }
    queue_.push(timer);
}

void MyTimerList::AdjustTimer(MyTimer* timer) {
}

void MyTimerList::DelTimer(MyTimer* timer) {
    if (timer == nullptr) {
        return;
    }
    timer->is_old_ = true;
}

void MyTimerList::Tick() {
    if (queue_.empty()) {
        return;
    }
    time_t cur = time(NULL);
    while (!queue_.empty()) {
        if (queue_.top()->is_old_) {
            queue_.pop();
            continue;
        }
        if (cur < queue_.top()->expire_) {
            break;
        }
        MyTimer* top = queue_.top();
        top->cb_func(top->data_);
        queue_.pop();
        delete top;
    }
}