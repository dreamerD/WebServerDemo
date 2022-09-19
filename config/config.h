#ifndef CONFIG_H
#define CONFIG_H
#include <string>
using std::string;
class Config {
   public:
    Config(const char* argv[]);
    ~Config();

   public:
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
