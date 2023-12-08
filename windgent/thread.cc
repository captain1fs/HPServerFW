#include "thread.h"
#include "log.h"
#include "util.h"

namespace windgent {

//线程局部变量，每个线程都有自己的一份拷贝
static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOWN";

static windgent::Logger::ptr g_logger = LOG_NAME("system");

Thread* Thread::GetThis() {
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if(t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

void* Thread::run(void* arg) {
    Thread* thd = (Thread*)arg;
    t_thread = thd;
    t_thread_name = thd->m_name;
    thd->m_id = windgent::GetThreadId();
    pthread_setname_np(pthread_self(), thd->m_name.substr(0,15).c_str());

    std::function<void()> cb;
    cb.swap(thd->m_cb);

    cb();

    // thd->m_sem.notify();
    // thd->m_cond.signal();
    return 0;
}

Thread::Thread(std::function<void()> cb, std::string name)
    :m_cb(cb), m_name(name) {
    if(name.empty()) {
        m_name = "UNKNOWN";
    }
    int retVal = pthread_create(&m_threadId, nullptr, &Thread::run, this);
    if(retVal) {
        LOG_ERROR(g_logger) << "pthread_create failed, retVal= " << retVal << ", name = " << m_name << std::endl;
            throw std::logic_error("pthread_create error");
    }
    // m_sem.wait();
    // m_cond.wait();
}

Thread::~Thread() {
    if(m_threadId) {
        pthread_detach(m_threadId);
    }
}

void Thread::join() {
    if(m_threadId) {
        int retVal = pthread_join(m_threadId, nullptr);
        if(retVal) {
            LOG_ERROR(g_logger) << "pthread_join failed, retVal= " << retVal << ", name = " << m_name << std::endl;
            throw std::logic_error("pthread_join error");
        }
    }
    m_threadId = 0;
}

}