#ifndef LOG_H
#define LOG_H
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <fstream>
#include <iostream>
#include <string>
#include "../lock/locker.h"
#include "../log_queue/log_queue.h"
using std::string;
class Log {
   public:
    static Log* GetInstance() {
        static Log instance;
        return &instance;
    }
    static void* FlushLog(void* args) {
        Log::GetInstance()->AsyncWriteLog();
    }
    bool Init(const string& log_name, bool is_closed, int max_line_nummber = 5000000, int max_queue_size = 10);
    void WriteLog(int level, const char* format, ...);

   private:
    Log();
    ~Log();
    void AsyncWriteLog();

   private:
    string log_name_;          // log文件名
    int max_line_nummber_;     // 日志最大行数
    long long count_;          // 日志行数记录
    string date_;              // 记录当前时间
    LogQueue<string>* queue_;  // 日志阻塞队列
    std::fstream iofs_;        //文件类对象
    bool is_closed_;           // 关闭
};
#define LOG_DEBUG(format, ...)                                  \
    if (0 == is_closed_log_) {                                  \
        Log::GetInstance()->WriteLog(0, format, ##__VA_ARGS__); \
    }
#define LOG_INFO(format, ...)                                   \
    if (0 == is_closed_log_) {                                  \
        Log::GetInstance()->WriteLog(1, format, ##__VA_ARGS__); \
    }
#define LOG_WARN(format, ...)                                   \
    if (0 == is_closed_log_) {                                  \
        Log::GetInstance()->WriteLog(2, format, ##__VA_ARGS__); \
    }
#define LOG_ERROR(format, ...)                                  \
    if (0 == is_closed_log_) {                                  \
        Log::GetInstance()->WriteLog(3, format, ##__VA_ARGS__); \
    }
#endif