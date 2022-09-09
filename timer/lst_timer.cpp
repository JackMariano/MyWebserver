#include "lst_timer.h"
#include "../http/http_conn.h"

time_heap::time_heap()
{
    cur_size = 0;
    capacity = 10000;//初始容量设置为10000
    array = new heap_timer*[capacity];
    for(int i = 0; i < capacity; i++) {
        array[i] = NULL;
    }
}
time_heap::~time_heap()
{
    for(int i = 0; i < cur_size; i++) {
        delete array[i];
    }
    delete[] array;
}
//添加定时器
void time_heap::add_timer(heap_timer *timer)
{
    if (!timer)
    {
        return;
    }
    if(cur_size >= capacity) //如果当前堆数组容量不够就进行扩容
    {
        resize();
    }
    int hole = cur_size++;
    int parent = 0;
    for(; hole > 0; hole = parent) {
        parent = (hole - 1)/2;
        if(array[parent]->expire <= timer->expire) {
            break;
        }
        array[hole] = array[parent];
    }
    hash[timer] = hole;
    array[hole] = timer;
}
//删除定时器
void time_heap::del_timer(heap_timer *timer)
{
    if (!timer)
    {
        return;
    }
    timer->cb_func = NULL;//延时删除
}

heap_timer* time_heap::top() {
    if(empty()) {
        return NULL;
    }
    return array[0];
}

void time_heap::pop_timer() {
    if(empty()) {
        return;
    }
    if(array[0]) {
        hash.erase(array[0]);
        delete array[0];
        cur_size--;
        hash[array[cur_size]] = 0;
        array[0] = array[cur_size];
        percolate_down(0);
    }
}

void time_heap::adjust_timer(heap_timer* timer) {
    percolate_down(hash[timer]);
}


void time_heap::swap_timer(int i, int j) {
    std::swap(array[i], array[j]);
    hash[array[i]] = j;
    hash[array[j]] = i;
}

//下沉操作，父节点与子节点比较
void time_heap::percolate_down(int hole) {
    heap_timer* temp = array[hole];
    int child = 0;
    for(; (hole*2+1) <= (cur_size-1); hole = child) {
        child = hole*2+1;
        if((child < (cur_size-1))&&(array[child+1]->expire < array[child]->expire)) {
            child++;
        }
        if(array[child]->expire < temp->expire) {
            swap_timer(child, hole);
        } else {
            break;
        }
    } 
}
void time_heap::resize() {
    heap_timer** temp = new heap_timer*[2*capacity];
    for(int i = 0; i < 2*capacity; i++) {
        temp[i] = NULL;
    }
    capacity = 2*capacity;
    for(int i = 0; i < cur_size; ++i) {
        temp[i] = array[i];
    }
    delete[] array;
    array = temp;
}
//SIGALARM信号每次触发就在其信号处理函数中执行一次tick函数，以处理链表上到期的任务
void time_heap::tick()
{
    heap_timer* tmp = array[0];
    time_t cur = time(NULL);
    while(!empty()) {
        if(!tmp) {
            break;
        }
        if(tmp->expire > cur) {
            break;
        }
        if(array[0]->cb_func) {
            array[0]->cb_func(array[0]->user_data);
        }
        pop_timer();
        tmp = array[0];
    }
}
bool time_heap::empty() const{
    return cur_size == 0;
}
//Utils用来统一事件源
void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    //事件对应的文件描述符
    event.data.fd = fd;
    //设计触发模式是水平触发LT还是边沿触发ET
    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;
    //设置一个socket连接在任一时刻都只能被一个线程处理
    if (one_shot)
        event.events |= EPOLLONESHOT;
    //设置描述符为非阻塞
    setnonblocking(fd);
    //添加事件
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    //将信号值写入管道，通知主循环，通过epoll来实现统一事件源
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号的处理函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    //重新调用被该信号终止的系统调用
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer_lst.tick();
    alarm(m_TIMESLOT);//这相当于是过一段时间就启动一次tick()
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
//回调函数
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}
