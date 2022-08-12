#include "data.h"

void data::init(){
    cout << 1 << endl;
}

void data::adjust_timer(util_timer *timer)
{
    time_t cur = time(NULL);
    timer->expire = cur + 3;
    utils.m_timer_lst.adjust_timer(timer);
}

void data::deal_timer(util_timer *timer, int sockfd)
{
    timer->cb_func(&users_timer[sockfd]);
    if (timer)
    {
        utils.m_timer_lst.del_timer(timer);
    }
}

data::data()
{
}
data::~data()
{
    delete[] users;
    delete[] users_timer;
}
void data::test(int i) {
    cout << i << endl;
}