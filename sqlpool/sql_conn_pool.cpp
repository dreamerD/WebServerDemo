#include "sql_conn_pool.h"

MySQLConnPool::MySQLConnPool() {
    cur_conn_num_ = 0;
    free_conn_num_ = 0;
}

MySQLConnPool* MySQLConnPool::GetInstance() {
    static MySQLConnPool connPool;
    return &connPool;
}

void MySQLConnPool::init(string url, string user, string password, string database_name, int port, int maxconn, bool is_closed_log) {
    url_ = url;
    user_ = user;
    password_ = password_;
    database_name_ = database_name;
    is_closed_log_ = is_closed_log;
    max_conn_num_ = maxconn;
    for (int i = 0; i < max_conn_num_; i++) {
        MYSQL* conn = nullptr;
        conn = mysql_init(conn);
        if (conn == nullptr) {
            LOG_ERROR("MySQL Error");
            exit(1);
        }
        conn = mysql_real_connect(conn, url_.c_str(), user_.c_str(), password_.c_str(), database_name_.c_str(), port_, nullptr, 0);
        if (conn == nullptr) {
            LOG_ERROR("MySQL Error");
            exit(1);
        }
        conn_list_.push_back(conn);
        free_conn_num_++;
    }
    mysem_ = MySem(free_conn_num_);
}

MYSQL* MySQLConnPool::GetConnection() {
    MYSQL* conn = nullptr;
    mysem_.Wait();
    mymutex_.Lock();
    conn = conn_list_.front();
    conn_list_.pop_front();

    --free_conn_num_;
    ++cur_conn_num_;

    mymutex_.Unlock();
    return conn;
}

bool MySQLConnPool::ReleaseConnection(MYSQL* conn) {
    if (conn == nullptr) {
        return false;
    }

    mymutex_.Lock();
    conn_list_.push_back(conn);
    ++free_conn_num_;
    --cur_conn_num_;
    mysem_.Post();
    mymutex_.Unlock();
}

void MySQLConnPool::DestroyPool() {
    mymutex_.Lock();
    if (conn_list_.size() > 0) {
        for (auto it = conn_list_.begin(); it != conn_list_.end(); ++it) {
            MYSQL* con = *it;
            mysql_close(con);
        }
        cur_conn_num_ = 0;
        free_conn_num_ = 0;
        conn_list_.clear();
    }
    mymutex_.Unlock();
}

int MySQLConnPool::GetFreeConn() {
    return free_conn_num_;
}

MySQLConnPool::~MySQLConnPool() {
    DestroyPool();
}

ConnectionRAII::ConnectionRAII(MySQLConnPool* connPool) {
    connRAII_ = connPool->GetConnection();
    poolRAII_ = connPool;
}

ConnectionRAII::~ConnectionRAII() {
    poolRAII_->ReleaseConnection(connRAII_);
}