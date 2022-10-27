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

#include "../lock/locker.h"
#include "../log/log.h"
#include "../sqlpool/sql_conn_pool.h"
using std::string;
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
    void Process();
    bool Read_once();
    bool Write();
    sockaddr_in* get_address() {
        return &address_;
    }
    //初始化数据库读取线程
    void Initmysql();

   private:
    void init();
    //从read_buf读取，并处理请求报文
    HTTP_CODE process_read();
    //向write_buf写入响应报文数据
    bool process_write(HTTP_CODE ret);
    //主状态机解析报文中的请求行数据
    HTTP_CODE parse_request_line(char* text);
    //主状态机解析报文中的请求头数据
    HTTP_CODE parse_headers(char* text);
    //主状态机解析报文中的请求内容
    HTTP_CODE parse_content(char* text);
    //生成响应报文
    HTTP_CODE do_request();
    // start_line是已经解析的字符
    // get_line用于将指针向后偏移，指向未处理的字符
    char* get_line() { return read_buf_ + start_line_; };
    //从状态机读取一行，分析是请求报文的哪一部分
    LINE_STATUS parse_line();
    void unmap();
    //根据响应报文格式，生成对应8个部分，以下函数均由do_request调用
    bool add_response(const char* format, ...);
    bool add_content(const char* content);
    bool add_status_line(int status, const char* title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();

   public:
    int state_;
    static int user_count_;
    static int epollfd_;
    int improv_;
    int timer_flag_;
    MySQLConnPool* connPool_;

   private:
    int sockfd_;
    sockaddr_in address_;
    //存储读取的请求报文数据
    char read_buf_[READ_BUFFER_SIZE];
    //缓冲区中read_buf中数据的最后一个字节的下一个位置
    int read_idx_;
    // read_buf读取的位置checked_idx
    int checked_idx_;
    // read_buf中已经解析的字符个数
    int start_line_;
    //存储发出的响应报文数据
    char write_buf_[WRITE_BUFFER_SIZE];
    //指示buffer中的长度
    int write_idx_;
    //主状态机的状态
    CHECK_STATE check_state_;
    //请求方法
    METHOD method_;
    //以下为解析请求报文中对应的6个变量
    //存储读取文件的名称
    char real_file_[FILENAME_LEN];
    char* url_;
    char* version_;
    char* host_;
    int content_length_;
    bool linger_;
    //读取服务器上的文件地址
    char* file_address_;
    struct stat file_stat_;
    // io向量机制iovec
    struct iovec iv_[2];
    int iv_count_;
    int cgi_;       //是否启用的POST
    char* string_;  //存储请求头数据
                    //剩余发送字节数
    int bytes_to_send_;
    //已发送字节数
    int bytes_have_send_;
    char* doc_root_;

    int TRIGMode_;       //触发模式
    int is_closed_log_;  //是否开启日志

    char sql_user_[100];
    char sql_passwd_[100];
    char sql_name_[100];
};
#endif