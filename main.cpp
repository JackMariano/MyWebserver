#include "config/config.h"
//#include "./threadpool/threadpool.h"
//#include "./http/http_conn.h"
//#include "./data/data.h"
int main(int argc, char *argv[])
{
    //需要修改的数据库信息,登录名,密码,库名
    string user = "root";
    string passwd = "123456";
    string databasename = "yourdb";
    //命令行解析
    Config config;
    //config.parse_arg(argc, argv);
    //data data1;
    //data1.init();
    //data1.test(4);
    WebServer server;
    //初始化
    server.init(config.PORT, user, passwd, databasename, config.sql_num, config.thread_num);
    

    //日志
    server.log_write();
    //数据库
    server.sql_pool();
    //线程池
    server.thread_pool();
    //监听
    server.eventListen();
    //运行
    server.eventLoop();
    return 0;
}