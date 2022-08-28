#include "config.h"

Config::Config(){
    //端口号,默认9006
    PORT = 9006;

    //数据库连接池数量,默认8
    sql_num = 8;

    //线程池内的线程数量,默认8
    thread_num = 8;
}