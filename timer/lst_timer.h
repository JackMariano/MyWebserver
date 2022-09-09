#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unordered_map>
#include <time.h>
#include "../log/log.h"

#include "../lock/locker.h"
class heap_timer;

struct client_data//用户数据结构
{
    sockaddr_in address;//客户端地址
    int sockfd;//套接字        
    heap_timer *timer;//定时器节点
};
class heap_timer//定时器的节点
{
public:
    heap_timer(){}; 

public:
    time_t expire;//任务的超时时间，这里使用绝对时间
    
    void (* cb_func)(client_data *);//任务回调函数的指针，定义了一个变量cb_func，这个变量是一个指针，指向返回值为空，参数都是client_data*
    //的函数的指针
    client_data *user_data;//用户数据
};

class time_heap//基于最小堆实现的定时器
{
public:
    time_heap();
    ~time_heap();

    void add_timer(heap_timer *timer);//添加定时器
    void percolate_down(int hole);//下沉操作，定时器应用领域不需要用到上浮
    void resize();//模仿vector的扩容机制
    heap_timer* top();
    void pop_timer();
    void del_timer(heap_timer *timer);//删除定时器
    void tick();//脉搏函数
    bool empty() const;
    void adjust_timer(heap_timer* timer);
    void swap_timer(int i, int j);
private:
    void add_timer(heap_timer *timer, heap_timer *lst_head);//用于在指定位置直接插入定时器，外界无法访问
    heap_timer** array;
    unordered_map<heap_timer*, int> hash;
    int capacity;
    int cur_size;
};

class Utils//统一事件源
{
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot);

    //对文件描述符设置非阻塞
    int setnonblocking(int fd);

    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    //信号处理函数
    static void sig_handler(int sig);

    //设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    time_heap m_timer_lst;
    static int u_epollfd;
    //超时信号的时间
    int m_TIMESLOT;
};

void cb_func(client_data *user_data);

#endif
