#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class sem
{
public:
    sem()
    {
        //初始化一个未命名的信号量，value参数指定信号的初始值
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();//抛出异常
        }
    }
    sem(int num)//有参构造
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);
    }
    //以原子操作将信号量的值减1.如果信号量的值为零，则sem_wait会被阻塞。
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    //以原子操作将信号两的值+1,当信号量大于0时，其他正在调用sem_wait的值会被唤醒
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;//m_sem就是信号量的值
};
//互斥锁
class locker
{
public:
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    //获取互斥锁的指针
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};
//条件变量，用于在线程之间同步共享数据的值
class cond
{
public:
    cond()
    {
        //初始化条件变量
        if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            //pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }
    bool wait(pthread_mutex_t *m_mutex)
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        //等待目标变量，为了保证cond_wait操作的原子性，应该加锁解锁
        //后面的m_mutex是用于保护条件变量的互斥锁
        ret = pthread_cond_wait(&m_cond, m_mutex);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    //挂起之后有一段等待时间
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t)
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    //用于唤醒一个等待目标条件变量的线程，具体唤醒那一个要看优先级和调度机制。
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }
    //用广播的方式唤醒所有等待目标条件变量的线程
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    //static pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};
#endif
