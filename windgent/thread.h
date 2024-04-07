#ifndef __THREAD_H__
#define __THREAD_H__

#include <thread>
#include <functional>
#include <memory>
#include "mutex.h"

namespace windgent {

//POSIX线程的封装
class Thread : NonCopyable {
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, std::string name);
    ~Thread();

    pid_t getId() const { return m_id; }
    const std::string& getName() const { return m_name; }

    void join();

    static Thread* GetThis();
    static const std::string& GetName();
    static void SetName(const std::string& name);
private:
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    static void* run(void* arg);
private:
    pid_t m_id = -1;            //linux的线程id，在日志中输出和ps显示的一样，便于调试
    pthread_t m_threadId = 0;   //pthreda库的线程id
    std::function<void()> m_cb;
    std::string m_name;

    Semaphore m_sem;
    // Cond m_cond;
};

}

#endif