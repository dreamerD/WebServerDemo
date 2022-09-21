#include "config.h"
/*改进，重写参数解析类*/
Config::Config()
    : inet_listen_port(9006),
      inet_triger_mode(iLTLT),
      inet_opt_linger(iNOTLINGER),
      idatabase_conn_thread_num(8),
      ithread_num(8),
      inet_actor_mode(iProactor) {
}

void Config::parse_arg(int argc, char* const* argv) {
    int opt;
    const char* str = "p:l:m:o:s:t:c:a:";
    while ((opt = getopt(argc, argv, str)) != -1) {
        switch (opt) {
            case 'p': {
                inet_listen_port = atoi(optarg);
                break;
            }
            case 'l': {
                ilog_mode = atoi(optarg);
                break;
            }
            case 'm': {
                inet_triger_mode = atoi(optarg);
                break;
            }
            case 'o': {
                inet_opt_linger = atoi(optarg);
                break;
            }
            case 's': {
                idatabase_conn_thread_num = atoi(optarg);
                break;
            }
            case 't': {
                ithread_num = atoi(optarg);
                break;
            }
            case 'a': {
                inet_actor_mode = atoi(optarg);
                break;
            }
            default:
                break;
        }
    }
}