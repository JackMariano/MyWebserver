#include "threadpool.h"
//#include "../webserver.h"

//构造函数
template <typename T>
threadpool<T>::threadpool(int actor_model, connection_pool *connPool, int thread_number, int max_requests) : m_actor_model(actor_model),m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL),m_connPool(connPool)
{
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    //创建描述线程池的数组
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads)
        throw std::exception();
    for (int i = 0; i < thread_number; ++i)
    {
        //循环创建线程，并将工作线程按照要求进行运行
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }
        //设置为脱离线程，这样就不用单独对线程进行回收
        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
}
//通过list容器创建请求队列，向队列中添加时，通过互斥锁保证线程安全，添加完成后通过信号量提醒有任务要处理，最后注意线程同步
template <typename T>
bool threadpool<T>::append(T *request, int state)
{
    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    request->m_state = state;
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    //信号量通知有任务要处理
    m_queuestat.post();
    return true;
}
template <typename T>
bool threadpool<T>::append_p(T *request)
{
    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}
//线程处理函数，内部访问静态私有成员函数run，完成线程处理要求
template <typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    
    return pool;
}
//主要实现，工作线程从请求队列中取出某个任务进行处理，注意线程同步
template <typename T>
void threadpool<T>::run()
{
    while (true)
    {
        //信号量等待
        m_queuestat.wait();
        //请求队列加锁
        m_queuelocker.lock();
        if (m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }
        //取出链表头部的任务
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        //对请求队列操作完成，解锁。
        m_queuelocker.unlock();
        if (!request)
            continue;
        if (1 == m_actor_model)
        {
            //reactor模式
            if (0 == request->m_state)//读事件
            {
                if (request->read_once())//读取数据成功
                {
                    //request->improv = 1;
                    connectionRAII mysqlcon(&request->mysql, m_connPool);
                    request->process();
                }
                else//读取数据失败，主线程会负责关闭这个连接。
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
            else
            {
                if (request->write())//写事件
                {
                    request->improv = 1;
                }
                else
                {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
        else
        {
            //从连接池中取出一个数据库连接，这个链接的地址是在任务的mysql中
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }
    }
}
