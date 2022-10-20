#ifndef SQL_CONN_POOL_H
#define SQL_CONN_POOL_H
#include <mysql/mysql.h>
#include <list>
#include <string>
#include "../lock/locker.h"
#include "../log/log.h"
using std::list;
using std::string;
class MySQLConnPool {
   public:
    MYSQL* GetConnection();
    bool ReleaseConnection(MYSQL* conn);
    int GetFreeConn();
    void DestroyPool();

    static MySQLConnPool* GetInstance();
    void init(string url, string user, string password, string database_name, int port, int maxconn, bool is_closed_log);

   private:
    MySQLConnPool();
    ~MySQLConnPool();

    int max_conn_num_;   // 最大连接数
    int cur_conn_num_;   // 当前连接数
    int free_conn_num_;  // 空闲的连接数
    MyMutex mymutex_;
    MySem mysem_;
    list<MYSQL*> conn_list_;  // 连接池
   public:
    string url_;
    int port_;
    string user_;
    string password_;
    string database_name_;
    bool is_closed_log_;
};

class ConnectionRAII {
   public:
    ConnectionRAII(MySQLConnPool* connpool);
    ~ConnectionRAII();

   private:
    MYSQL* connRAII_;
    MySQLConnPool* poolRAII_;
};
#endif