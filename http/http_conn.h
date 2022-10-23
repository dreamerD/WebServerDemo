#ifndef HTTP_CONN_H
#define HTTP_CONN_H
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <map>

class HttpConn {
   public:
    // 读取文件长度上限
    static const int FILENAME_LEN = 200;
    // 读缓存大小
    static const int READ_BUFFER_SIZE = 2048;
    // 写缓存大小
    static const int WRITE_BUFFER_SIZE = 2048;
    // HTTP方法名
    enum METHOD {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    //主状态机状态，检查请求报文中元素
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    // HTTP状态码
    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    //从状态机的状态，文本解析是否成功
    enum LINE_STATUS {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

   public:
    HttpConn() {}
    ~HttpConn() {}

   public:
    void Init(int sockfd, const sockaddr_in& addr, char*, int, int, string user, string passwd, string sqlname);
    void CloseConn(bool real_close = true);
    void process();

   public:
    int state_;
    static int user_count_;
    static int epollfd_;
    int improv_;
    int timer_flag_;
};
#endif