#include "timer.h"

MyTimerList::~MyTimerList() {
    head_ = nullptr;
    tail_ = nullptr;
}

MyTimerList::~MyTimerList() {
    MyTimer* tmp = head_;
    while (tmp) {
        head_ = tmp->next_;
        delete tmp;
        tmp = head_;
    }
}

void MyTimerList::AddTimer(MyTimer* timer) {
    if (timer == nullptr) {
        return;
    }
    if (head_ == nullptr) {
        head_ = tail_ = timer;
        return;
    }
    // 定时器中是按照expire从小到大排序
    // 如果新的定时器超时时间小于当前头部结点
    // 直接将当前定时器结点作为头部结点
    if (timer->expire_ < head_->expire_) {
        timer->next_ = head_;
        head_->prev_ = timer;
        head_ = timer;
        return;
    }
    addTimer(timer, head_);
}

void MyTimerList::AdjustTimer(MyTimer* timer) {
    if (timer == nullptr) {
        return;
    }

    MyTimer* tmp = timer->next_;
    // 被调整的定时器在链表尾部
    // or 定时器超时值仍然小于下一个定时器超时值，不调整
    if (!tmp || (timer->expire_ < tmp->expire_)) {
        return;
    }

    // 被调整定时器是链表头结点，将定时器取出，重新插入
    if (timer == head_) {
        head_ = head_->next_;
        head_->prev_ = nullptr;
        timer->next_ = nullptr;
        addTimer(timer, head_);
    }
    // 被调整定时器在内部，将定时器取出，重新插入
    else {
        timer->prev_->next_ = timer->next_;
        timer->next_->prev_ = timer->prev_;
        addTimer(timer, timer->next_);
    }
}

void MyTimerList::DelTimer(MyTimer* timer) {
    if (!timer) {
        return;
    }
    // 链表中只有一个定时器，需要删除该定时器
    if ((timer == head_) && (timer == tail_)) {
        delete timer;
        head_ = nullptr;
        tail_ = nullptr;
        return;
    }

    // 被删除的定时器为头结点
    if (timer == head_) {
        head_ = head_->next_;
        head_->prev_ = nullptr;
        delete timer;
        return;
    }

    // 被删除的定时器为尾结点
    if (timer == tail_) {
        tail_ = tail_->prev_;
        tail_->next_ = nullptr;
        delete timer;
        return;
    }

    // 被删除的定时器在链表内部，常规链表结点删除
    timer->prev_->next_ = timer->next_;
    timer->next_->prev_ = timer->prev_;
    delete timer;
}

void MyTimerList::Tick() {
    if (head_ == nullptr) {
        return;
    }

    // 获取当前时间
    time_t cur = time(NULL);
    MyTimer* tmp = head_;

    // 遍历定时器链表
    while (tmp) {
        // if当前时间小于定时器的超时时间，后面的定时器也没有到期
        if (cur < tmp->expire_) {
            break;
        }

        // 当前定时器到期，则调用回调函数，执行定时事件
        tmp->cb_func(tmp->data_);

        // 将处理后的定时器从链表容器中删除，并重置头结点
        head_ = tmp->next_;
        if (head_ != nullptr) {
            head_->prev_ = nullptr;
        }
        delete tmp;
        tmp = head_;
    }
}

void MyTimerList::addTimer(MyTimer* timer, MyTimer* lst_head) {
    MyTimer* prev = lst_head;
    MyTimer* tmp = prev->next_;
    // 从双向链表中找到该定时器应该放置的位置
    // 即遍历一遍双向链表找到对应的位置
    while (tmp != nullptr) {
        if (timer->expire_ < tmp->expire_) {
            prev->next_ = timer;
            timer->next_ = tmp;
            tmp->prev_ = timer;
            timer->prev_ = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next_;
    }

    // 遍历完发现，目标定时器需要放到尾结点处
    if (tmp != nullptr) {
        prev->next_ = timer;
        timer->prev_ = prev;
        timer->next_ = nullptr;
        tail_ = timer;
    }
}