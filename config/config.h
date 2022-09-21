#ifndef CONFIG_H
#define CONFIG_H
#include <unistd.h>
#include <string>
using std::string;
class Config {
   public:
    Config();
    void parse_arg(int argc, char* const* argv);

    ~Config(){};

   public:
    /*一些常量*/
    static constexpr int iLTLT = 0;
    static constexpr int iLTET = 1;
    static constexpr int iETLT = 2;
    static constexpr int iETET = 3;

    static constexpr int iNOTLINGER = 0;
    static constexpr int iLINGER = 1;

    static constexpr int iProactor = 0;
    static constexpr int iReactor = 1;

    /*数据库相关参数*/
    string sdatabase_user;
    string sdatabase_password;
    int idatabase_port;
    int idatabase_conn_thread_num;

    /*网络连接相关参数*/
    int inet_listen_port;
    int inet_triger_mode;
    int inet_actor_mode;
    int inet_opt_linger;

    /*日志相关参数*/
    int ilog_mode;

    /*线程池相关参数*/
    int ithread_num;
};
#endif
