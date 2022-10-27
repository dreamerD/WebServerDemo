#include "http_conn.h"
#include <mysql/mysql.h>
#include <fstream>
#include "../utils/utils.h"
//定义http响应的一些状态信息
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file form this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem serving the request file.\n";

MyMutex mutex_;
std::map<string, string> users_map;
int HttpConn::user_count_ = 0;
int HttpConn::epollfd_ = -1;

void HttpConn::Initmysql() {
    ConnectionRAII mysqlconnraii(connPool_);
    MYSQL* mysql = mysqlconnraii.GetConnection();
    //在user表中检索username，passwd数据，浏览器端输入
    if (mysql_query(mysql, "SELECT username,passwd FROM user")) {
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }
    //从表中检索完整的结果集
    MYSQL_RES* result = mysql_store_result(mysql);
    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        string temp1(row[0]);
        string temp2(row[1]);
        users_map[temp1] = temp2;
    }
}
// 子线程不关闭连接

// 初始化连接，初始化套接字地址
void HttpConn::Init(int sockfd, const sockaddr_in& addr, char* root, int TRIGMode, int close_log, string user, string passwd, string sqlname) {
    sockfd_ = sockfd;
    address_ = addr;
    doc_root_ = root;
    TRIGMode_ = TRIGMode;
    is_closed_log_ = close_log;

    strcpy(sql_user_, user.c_str());
    strcpy(sql_passwd_, passwd.c_str());
    strcpy(sql_name_, sqlname.c_str());
    // 原代码这里有问题
    Utils::Addfd(epollfd_, sockfd, true, TRIGMode_);
    user_count_++;
    init();
}

void HttpConn::init() {
    // mysql_ = NULL;
    bytes_to_send_ = 0;
    bytes_have_send_ = 0;
    check_state_ = CHECK_STATE_REQUESTLINE;
    linger_ = false;
    method_ = GET;
    url_ = 0;
    version_ = 0;
    content_length_ = 0;
    host_ = 0;
    start_line_ = 0;
    checked_idx_ = 0;
    read_idx_ = 0;
    write_idx_ = 0;
    cgi_ = 0;
    state_ = 0;
    timer_flag_ = 0;
    improv_ = 0;

    memset(read_buf_, '\0', READ_BUFFER_SIZE);
    memset(write_buf_, '\0', WRITE_BUFFER_SIZE);
    memset(real_file_, '\0', FILENAME_LEN);
}

bool HttpConn::Read_once() {
    if (read_idx_ >= READ_BUFFER_SIZE) {
        return false;
    }
    int bytes_read = 0;

    // ET读数据
    while (true) {
        // https://blog.csdn.net/liurunjiang/article/details/79743381
        bytes_read = recv(sockfd_, read_buf_ + read_idx_, READ_BUFFER_SIZE - read_idx_, 0);
        if (bytes_read == -1) {
            //非阻塞ET模式下，需要一次性将数据读完，无错
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            return false;
        } else if (bytes_read == 0) {
            return false;
        }
        read_idx_ += bytes_read;
    }
    return true;
}

//功能逻辑单元
HttpConn::HTTP_CODE HttpConn::do_request() {
    strcpy(real_file_, doc_root_);
    int len = strlen(doc_root_);
    // printf("url:%s\n", url);
    const char* p = strrchr(url_, '/');

    //处理cgi
    if (cgi_ == 1 && (*(p + 1) == '2' || *(p + 1) == '3')) {
        char* url_real = (char*)malloc(sizeof(char) * 200);
        strcpy(url_real, "/");
        strcat(url_real, url_ + 2);
        strncpy(real_file_ + len, url_real, FILENAME_LEN - len - 1);
        free(url_real);

        //将用户名和密码提取出来
        // eg:user=123&passwd=123
        char name[100], password[100];
        int i;
        for (i = 5; string_[i] != '&'; ++i)
            name[i - 5] = string_[i];
        name[i - 5] = '\0';

        int j = 0;
        for (i = i + 10; string_[i] != '\0'; ++i, ++j)
            password[j] = string_[i];
        password[j] = '\0';

        if (*(p + 1) == '3') {
            //如果是注册，先检测数据库中是否有重名的
            //没有重名的，进行增加数据
            char* sql_insert = (char*)malloc(sizeof(char) * 200);
            strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
            strcat(sql_insert, "'");
            strcat(sql_insert, name);
            strcat(sql_insert, "', '");
            strcat(sql_insert, password);
            strcat(sql_insert, "')");

            if (users_map.find(name) == users_map.end()) {
                mutex_.Lock();
                ConnectionRAII mysqlconnraii(connPool_);
                MYSQL* mysql = mysqlconnraii.GetConnection();
                int res = mysql_query(mysql, sql_insert);
                users_map.insert(std::pair<string, string>(name, password));
                mutex_.Unlock();

                if (!res)
                    strcpy(url_, "/log.html");
                else
                    strcpy(url_, "/registerError.html");
            } else
                strcpy(url_, "/registerError.html");
        }
        //如果是登录，直接判断
        //若浏览器端输入的用户名和密码在表中可以查找到，返回1，否则返回0
        else if (*(p + 1) == '2') {
            if (users_map.find(name) != users_map.end() && users_map[name] == password)
                strcpy(url_, "/welcome.html");
            else
                strcpy(url_, "/logError.html");
        }
    }
    //如果请求资源为/0，表示跳转注册界面
    if (*(p + 1) == '0') {
        char* url_real = (char*)malloc(sizeof(char) * 200);
        strcpy(url_real, "/register.html");
        strncpy(real_file_ + len, url_real, strlen(url_real));

        free(url_real);
    }
    //如果请求资源为/1，表示跳转登录界面
    else if (*(p + 1) == '1') {
        char* url_real = (char*)malloc(sizeof(char) * 200);
        strcpy(url_real, "/log.html");
        strncpy(real_file_ + len, url_real, strlen(url_real));

        free(url_real);
    }
    //如果请求资源为/5，表示跳转pic
    else if (*(p + 1) == '5') {
        char* url_real = (char*)malloc(sizeof(char) * 200);
        strcpy(url_real, "/picture.html");
        strncpy(real_file_ + len, url_real, strlen(url_real));

        free(url_real);
    }
    //如果请求资源为/6，表示跳转video
    else if (*(p + 1) == '6') {
        char* url_real = (char*)malloc(sizeof(char) * 200);
        strcpy(url_real, "/video.html");
        strncpy(real_file_ + len, url_real, strlen(url_real));

        free(url_real);
    }
    //如果请求资源为/7，表示跳转weixin
    else if (*(p + 1) == '7') {
        char* url_real = (char*)malloc(sizeof(char) * 200);
        strcpy(url_real, "/fans.html");
        strncpy(real_file_ + len, url_real, strlen(url_real));
        free(url_real);
    } else
        //如果以上均不符合，即不是登录和注册，直接将url与网站目录拼接
        //这里的情况是welcome界面，请求服务器上的一个图片
        strncpy(real_file_ + len, url_, FILENAME_LEN - len - 1);
    //通过stat获取请求资源文件信息，成功则将信息更新到file_stat结构体
    //失败返回NO_RESOURCE状态，表示资源不存在
    if (stat(real_file_, &file_stat_) < 0)
        return NO_RESOURCE;

    //判断文件的权限，是否可读，不可读则返回FORBIDDEN_REQUEST状态
    if (!(file_stat_.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    //判断文件类型，如果是目录，则返回BAD_REQUEST，表示请求报文有误
    if (S_ISDIR(file_stat_.st_mode))
        return BAD_REQUEST;

    //以只读方式获取文件描述符，通过mmap将该文件映射到内存中
    int fd = open(real_file_, O_RDONLY);
    file_address_ = (char*)mmap(0, file_stat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    //避免文件描述符的浪费和占用
    close(fd);

    //表示请求文件存在，且可以访问
    return FILE_REQUEST;
}

// 判断http请求是否被完整读入
HttpConn::HTTP_CODE HttpConn::parse_content(char* text) {
    if (read_idx_ >= (content_length_ + checked_idx_)) {
        text[content_length_] = '\0';
        // POST请求中最后为输入的用户名和密码
        string_ = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}
//解析http请求行，获得请求方法，目标url及http版本号
HttpConn::HTTP_CODE HttpConn::parse_request_line(char* text) {
    //在HTTP报文中，请求行用来说明请求类型
    //要访问的资源以及所使用的HTTP版本，其中各个部分之间通过\t或空格分隔。
    //请求行中最先含有空格和\t任一字符的位置并返回

    // strpbrk在源字符串（s1）中找出最先含有搜索字符串（s2）
    //中任一字符的位置并返回，若找不到则返回空指针
    url_ = strpbrk(text, " \t");
    //如果没有空格或\t，则报文格式有误
    if (!url_) {
        return BAD_REQUEST;
    }
    //将该位置改为\0，用于将前面数据取出
    //(已读取的数据不再会匹配到\t）
    *url_++ = '\0';

    //取出数据，并通过与GET和POST比较，以确定请求方式
    char* method = text;
    if (strcasecmp(method, "GET") == 0)
        method_ = GET;
    else if (strcasecmp(method, "POST") == 0) {
        method_ = POST;
        cgi_ = 1;
    } else {
        return BAD_REQUEST;
    }
    // url此时跳过了第一个空格或\t字符，但不知道之后是否还有
    //将url向后偏移，通过查找
    //继续跳过空格和\t字符，指向请求资源的第一个字符

    // strspn:检索字符串 str1 中第一个不在字符串 str2 中出现的字符下标
    //即跳过匹配的字符串片段
    url_ += strspn(url_, " \t");

    //相同逻辑，判断HTTP版本号
    version_ = strpbrk(url_, " \t");
    if (!version_)
        return BAD_REQUEST;
    *version_++ = '\0';
    version_ += strspn(version_, " \t");
    //仅支持HTTP/1.1
    if (strcasecmp(version_, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }

    //对请求资源前7个字符进行判断
    //这里主要是有些报文的请求资源中会带有http://
    //这里需要对这种情况进行单独处理
    if (strncasecmp(url_, "http://", 7) == 0) {
        url_ += 7;
        url_ = strchr(url_, '/');
    }

    //同样增加https情况
    if (strncasecmp(url_, "https://", 8) == 0) {
        url_ += 8;
        url_ = strchr(url_, '/');
    }

    //一般的不会带有上述两种符号，直接是单独的/或/后面带访问资源
    if (!url_ || url_[0] != '/') {
        return BAD_REQUEST;
    }
    //当url为/时，显示欢迎界面
    if (strlen(url_) == 1) {
        strcat(url_, "judge.html");
    }
    //请求行处理完毕，将主状态机转移处理请求头
    check_state_ = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//从状态机，用于分析出一行内容
//返回值为行的读取状态，有LINE_OK,LINE_BAD,LINE_OPEN
HttpConn::LINE_STATUS HttpConn::parse_line() {
    char temp;
    for (; checked_idx_ < read_idx_; ++checked_idx_) {
        temp = read_buf_[checked_idx_];
        if (temp == '\r') {
            if ((checked_idx_ + 1) == read_idx_)
                return LINE_OPEN;
            else if (read_buf_[checked_idx_ + 1] == '\n') {
                read_buf_[checked_idx_++] = '\0';
                read_buf_[checked_idx_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        } else if (temp == '\n') {
            if (checked_idx_ > 1 && read_buf_[checked_idx_ - 1] == '\r') {
                read_buf_[checked_idx_ - 1] = '\0';
                read_buf_[checked_idx_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

HttpConn::HTTP_CODE HttpConn::parse_headers(char* text) {
    //判断是空行还是请求头
    if (text[0] == '\0') {
        //判断是GET还是POST请求
        //! 0 is POST
        if (content_length_ != 0) {
            // POST需要跳转到消息体处理状态
            check_state_ = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        //==0 is GET
        return GET_REQUEST;
    }
    //解析请求头部connection字段
    else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;

        //跳过空格和\t字符
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            //如果是长连接，则将linger标志设置为true
            linger_ = true;
        }
    }
    //解析请求头部Content-length字段
    else if (strncasecmp(text, "Content-length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        content_length_ = atol(text);
    }
    //解析请求头部Host字段
    else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        host_ = text;
    } else {
        LOG_INFO("oop!unknow header: %s", text);
    }
    return NO_REQUEST;
}

HttpConn::HTTP_CODE HttpConn::process_read() {
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = 0;

    while ((check_state_ == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = parse_line()) == LINE_OK)) {
        text = get_line();
        start_line_ = checked_idx_;
        LOG_INFO("%s", text);
        switch (check_state_) {
            case CHECK_STATE_REQUESTLINE: {
                ret = parse_request_line(text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER: {
                ret = parse_headers(text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                } else if (ret == GET_REQUEST) {
                    return do_request();
                }
                break;
            }
            case CHECK_STATE_CONTENT: {
                ret = parse_content(text);
                if (ret == GET_REQUEST)
                    return do_request();
                line_status = LINE_OPEN;
                break;
            }
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

bool HttpConn::add_response(const char* format, ...) {
    // 写入内容超出write_buf_大小则报错
    if (write_idx_ >= WRITE_BUFFER_SIZE) {
        return false;
    }
    va_list arg_list;
    // 将变量arg_list初始化为传入参数
    va_start(arg_list, format);
    // 将数据format从可变参数列表写入缓冲区，返回写入数据的长度
    int len = vsnprintf(write_buf_ + write_idx_, WRITE_BUFFER_SIZE - 1 - write_idx_, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - write_idx_)) {
        va_end(arg_list);
        return false;
    }
    write_idx_ += len;
    va_end(arg_list);
    LOG_INFO("request:%s", write_buf_);
    return true;
}

// 添加状态行
bool HttpConn::add_status_line(int status, const char* title) {
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

// 添加Content-length
bool HttpConn::add_content_length(int content_len) {
    return add_response("Content-Length:%d\r\n", content_len);
}

//添加连接状态，通知浏览器端是保持连接还是关闭
bool HttpConn::add_linger() {
    return add_response("Connection:%s\r\n", linger_ ? "keep-alive" : "close");
}

//添加空行
bool HttpConn::add_blank_line() {
    return add_response("%s", "\r\n");
}
//添加文本content
bool HttpConn::add_content(const char* content) {
    return add_response("%s", content);
}
// 添加消息报头，添加文本长度、连接状态和空行
bool HttpConn::add_headers(int content_len) {
    return add_content_length(content_len) && add_linger() && add_blank_line();
}
//生成响应报文
bool HttpConn::process_write(HTTP_CODE ret) {
    switch (ret) {
        case INTERNAL_ERROR: {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if (!add_content(error_500_form))
                return false;
            break;
        }
        case BAD_REQUEST: {
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if (!add_content(error_404_form))
                return false;
            break;
        }
        case FORBIDDEN_REQUEST: {
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if (!add_content(error_403_form))
                return false;
            break;
        }
        case FILE_REQUEST: {
            add_status_line(200, ok_200_title);
            if (file_stat_.st_size != 0) {
                add_headers(file_stat_.st_size);
                iv_[0].iov_base = write_buf_;
                iv_[0].iov_len = write_idx_;
                iv_[1].iov_base = file_address_;
                iv_[1].iov_len = file_stat_.st_size;
                iv_count_ = 2;
                bytes_to_send_ = write_idx_ + file_stat_.st_size;
                return true;
            } else {
                const char* ok_string = "<html><body></body></html>";
                add_headers(strlen(ok_string));
                if (!add_content(ok_string))
                    return false;
            }
        }
        default:
            return false;
    }
    iv_[0].iov_base = write_buf_;
    iv_[0].iov_len = write_idx_;
    iv_count_ = 1;
    bytes_to_send_ = write_idx_;
    return true;
}

//取消内存映射
void HttpConn::unmap() {
    if (file_address_ != nullptr) {
        munmap(file_address_, file_stat_.st_size);
        file_address_ = nullptr;
    }
}
// 写入数据
bool HttpConn::Write() {
    int temp = 0;
    while (true) {
        temp = writev(sockfd_, iv_, iv_count_);
        if (temp < 0) {
            //缓冲区满
            if (errno == EAGAIN) {
                Utils::Modfd(epollfd_, sockfd_, EPOLLOUT, TRIGMode_);
                return true;
            }

            unmap();
            return false;
        }

        bytes_have_send_ += temp;
        bytes_to_send_ -= temp;

        //第一个iovec头部信息的数据已发送完，发送第二个iovec数据
        if (bytes_have_send_ >= iv_[0].iov_len) {
            //不再继续发送头部信息
            iv_[0].iov_len = 0;
            iv_[1].iov_base = file_address_ + (bytes_have_send_ - write_idx_);
            iv_[1].iov_len = bytes_to_send_;
        } else {
            iv_[0].iov_base = write_buf_ + bytes_have_send_;
            iv_[0].iov_len = iv_[0].iov_len - bytes_have_send_;
        }

        //判断条件，数据已全部发送完
        if (bytes_to_send_ <= 0) {
            //如果发送失败，但不是缓冲区问题，取消映射
            unmap();
            //重新注册写事件
            Utils::Modfd(epollfd_, sockfd_, EPOLLIN, TRIGMode_);

            //浏览器的请求为长连接
            if (linger_) {
                //重新初始化HTTP对象**
                init();
                return true;
            } else {
                return false;
            }
        }
    }
}
//处理http报文请求与报文响应
//根据read/write的buffer进行报文的解析和响应
void HttpConn::Process() {
    // NO_REQUEST，表示请求不完整，需要继续接收请求数据
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST) {
        //注册并监听读事件
        Utils::Modfd(epollfd_, sockfd_, EPOLLIN, TRIGMode_);
        return;
    }
    //调用process_write完成报文响应
    bool write_ret = process_write(read_ret);
    // if (!write_ret) {
    //     close_conn();
    // }
    //注册并监听写事件
    if (write_ret) {
        Utils::Modfd(epollfd_, sockfd_, EPOLLOUT, TRIGMode_);
    }
}
