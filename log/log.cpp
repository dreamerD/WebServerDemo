#include "log.h"
#include <pthread.h>
#include <iomanip>
#include <sstream>
Log::Log() {
    count_ = 0;  // 日志记录数为0
}

bool Log::Init(const string& file_name, bool is_closed, int max_line_nummber, int max_queue_size) {
    queue_ = new LogQueue<string>(max_queue_size);
    pthread_t tid;
    pthread_create(&tid, NULL, FlushLog, NULL);

    is_closed_ = is_closed;
    max_line_nummber_ = max_line_nummber;
    log_name_ = file_name;
    time_t t = time(NULL);
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    date_ = std::to_string(my_tm.tm_year + 1900) + "_" + (my_tm.tm_mon + 1 < 10 ? "0" : "") + std ::to_string(my_tm.tm_mon + 1) + "_" + (my_tm.tm_mday < 10 ? "0" : "") + std::to_string(my_tm.tm_mday);
    string full_log_name = date_ + "_" + log_name_;
    iofs_.open(full_log_name, std::ios::in | std::ios::out | std::ios::app);
    if (!iofs_.is_open()) {
        return false;
    }
    return true;
}

void Log::WriteLog(int level, const char* format, ...) {
    struct timeval now = {0, 0};
    time_t t = now.tv_sec;
    struct tm* sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    string str;
    switch (level) {
        case 0:
            str += "[debug]:";
            break;
        case 1:
            str += "[info]:";
            break;
        case 2:
            str += "[warn]:";
            break;
        case 3:
            str += "[erro]:";
            break;
        default:
            str += "[info]:";
            break;
    }
    va_list valst;
    va_start(valst, format);
    string new_log = std::to_string(my_tm.tm_year + 1900) + "_" + (my_tm.tm_mon + 1 < 10 ? "0" : "") + std ::to_string(my_tm.tm_mon + 1) + "_" + (my_tm.tm_mday < 10 ? "0" : "") + std::to_string(my_tm.tm_mday) + " ";
    new_log += (my_tm.tm_hour < 10 ? "0" : "") + std::to_string(my_tm.tm_hour) + ":" + (my_tm.tm_min < 10 ? "0" : "") + ":" + std::to_string(my_tm.tm_min) + ":" + (my_tm.tm_sec < 10 ? "0" : "") + std::to_string(my_tm.tm_sec) + ":";
    std::stringstream ss;
    ss << std::showpoint << std::setprecision(6) << now.tv_usec;
    new_log += ss.str() + " ";
    new_log += str;
    char buffer[4096] = {'\0'};
    vsnprintf(buffer, 4095, format, valst);
    new_log += buffer;
    queue_->Push(new_log);
    va_end(valst);
}

void Log::AsyncWriteLog() {
    string single_log;
    //从阻塞队列中取出一个日志string，写入文件
    while (true) {
        queue_->Pop(single_log);
        if (single_log.substr(0, single_log.find(" ") + 1) != date_ || count_ % max_line_nummber_ == 0) {
            iofs_.close();
            if (count_ % max_line_nummber_ == 0) {
                iofs_.open(date_ + "_" + log_name_ + "_" + std::to_string(count_ / max_line_nummber_));
                assert(iofs_.is_open());
                count_ = 0;
            } else {
                date_ = single_log.substr(0, single_log.find(" ") + 1);
                iofs_.open(date_ + "_" + log_name_ + "_" + std::to_string(count_ / max_line_nummber_));
                assert(iofs_.is_open());
                count_ = 0;
            }
        }
        iofs_ << single_log << std::endl;
        count_++;
    }
}