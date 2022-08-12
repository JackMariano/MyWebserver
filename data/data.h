#ifndef DATA_H
#define DATA_H      

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "../http/http_conn.h"
        //最小超时单位
class data {
public:
    data();
    ~data();
public:
    http_conn *users;
    client_data *users_timer;
    Utils utils;
public:
    void test(int i);
    void init();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_timer *timer);
    void deal_timer(util_timer*timer, int sockfd);
};


#endif