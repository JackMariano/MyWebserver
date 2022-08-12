#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>
using namespace std;

Log::Log()
{
    m_count = 0;
    m_is_async = false;
}

Log::~Log()
{
    if (m_fp != NULL)
    {
        fclose(m_fp);
    }
}
//异步需要设置阻塞队列的长度，同步不需要设置
bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    //如果设置了max_queue_size,则设置为异步
    if (max_queue_size >= 1)
    {
        m_is_async = true;
        m_log_queue = new block_queue<string>(max_queue_size);//阻塞队列
        pthread_t tid;
        //flush_log_thread为回调函数,这里表示创建线程异步写日志
        pthread_create(&tid, NULL, flush_log_thread, NULL);
    }
    //是否关闭日志系统
    m_close_log = close_log;
    //日志缓冲区大小
    m_log_buf_size = log_buf_size;
    //申请一个日志缓冲区
    m_buf = new char[m_log_buf_size];
    //填充零
    memset(m_buf, '\0', m_log_buf_size);
    //最大行数
    m_split_lines = split_lines;
    //获取当前时间
    time_t t = time(NULL);
    //使用timer的值来填充tm结构体
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    //从后往前找到第一个/的位置。
    const char *p = strrchr(file_name, '/');
    char log_full_name[300] = {0};
    //相当于自定义日志名
    //若输入的文件名没有/，则直接将时间+文件名作为日志名。
    if (p == NULL)
    {
        snprintf(log_full_name, 299, "%d_%02d_%02d_%s", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    }
    else
    {
        //将/位置向后移动一个位置，然后复制到logname中
        //p-file_name+1是文件所在路径文件夹的长度
        //dirname相当于.
        strcpy(log_name, p + 1);//操作日志文件名
        strncpy(dir_name, file_name, p - file_name + 1);//操作日志文件目录
        //后面的参数和format有关
        snprintf(log_full_name, 299, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    m_today = my_tm.tm_mday;
    
    m_fp = fopen(log_full_name, "a");
    if (m_fp == NULL)
    {
        return false;
    }

    return true;
}

void Log::write_log(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;
    char s[16] = {0};
    switch (level)
    {
    case 0:
        strcpy(s, "[debug]:");
        break;
    case 1:
        strcpy(s, "[info]:");
        break;
    case 2:
        strcpy(s, "[warn]:");
        break;
    case 3:
        strcpy(s, "[erro]:");
        break;
    default:
        strcpy(s, "[info]:");
        break;
    }
    //写入一个log，对m_count++, m_split_lines最大行数
    m_mutex.lock();
    //跟新现有行数
    m_count++;
    //日志不是今天或写入的，日志行数是最大行的倍数
    //m_split_lines为最大行数
    //重新打开m_fp
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) //everyday log
    {
        
        char new_log[300] = {0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16] = {0};
       //格式化日志名中的时间部分
        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
       //如果时间不是今天，则创建今天的日志，更新m_today和m_count
        if (m_today != my_tm.tm_mday)
        {
            snprintf(new_log, 299, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        }
        else
        {
            //超过了最大行，在之前的日志名基础上加后缀，m_count/m_split_lines
            snprintf(new_log, 299, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }
        m_fp = fopen(new_log, "a");
    }
 
    m_mutex.unlock();

    va_list valst;
    //对valst进行初始化，让它指向可变参数表里面的地一个参数。也就是省略号之前的参数。
    va_start(valst, format);

    string log_str;
    m_mutex.lock();
    //写入内容格式：时间+内容
    //写入的具体时间内容格式，snprintf成功返回写字符的总数，其中不包括结尾的null字符
    int n = snprintf(m_buf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                     my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
    //按size大小输出到字符串str中，但是变量使用va_list调用所替代
    int m = vsnprintf(m_buf + n, m_log_buf_size - 1, format, valst);
    m_buf[n + m] = '\n';
    m_buf[n + m + 1] = '\0';
    log_str = m_buf;

    m_mutex.unlock();
    //若m_is_async为true表示异步，默认为同步
    //若同步则将日志信息加入阻塞队列，同步则加锁向文件中写
    if (m_is_async && !m_log_queue->full())
    {
        m_log_queue->push(log_str);
    }
    else
    {
        m_mutex.lock();
        fputs(log_str.c_str(), m_fp);
        m_mutex.unlock();
    }

    va_end(valst);
}

void Log::flush(void)
{
    m_mutex.lock();
    //强制刷新写入流缓冲区
    fflush(m_fp);
    m_mutex.unlock();
}
