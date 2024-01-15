#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <iostream>
#include <memory>
#include <atomic>
#include <vector>
#include <string>
#include <list>
#include "fiber.h"
#include "thread.h"
#include "mutex.h"


namespace windgent {

//协程调度器类，内部有一个线程池和一个待执行的协程（任务）队列，调度即M个协程可在N个线程之间切换
class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() const { return m_name; }

    static Scheduler* GetThis();
    static Fiber* GetMainFiber();

    void start();
    void stop();

    //协程调度:可以指定协程在某个线程中执行
    template<class FiberOrcb>
    void schedule(FiberOrcb fc, int thd = -1) {
        std::cout << "--------- Scheduler::schedule() ---------" << std::endl;
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mtx);
            need_tickle = scheduleNoLock(fc, thd);
        }
        if(need_tickle) {
            std::cout << "--------- need_tickle ---------" << std::endl;
            tickle();
        }
    }
    //批量协程调度
    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mtx);
            while(begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if(need_tickle) {
            tickle();
        }
    }

    void switchTo(int thread);
    std::ostream& dump(std::ostream& os);
protected:
    //通知协程调度器有任务到来
    virtual void tickle();
    //协程调度函数
    void run();
    virtual bool stopping();
    virtual void idle();    //协程无任务可调度时执行idle协程

    void setThis();
    bool hasIdleThreads() { return m_idleThreadCount > 0; }
private:

    template<class FiberOrcb>
    bool scheduleNoLock(FiberOrcb fc, int thd) {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thd);
        if(ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }
private:
    //线程要执行的任务：可以是一个协程或者一个std::function
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int threadId;

        //协程任务
        FiberAndThread(Fiber::ptr f, int thr):fiber(f), threadId(thr) {

        }
        FiberAndThread(Fiber::ptr* f, int thr) :threadId(thr) {
            fiber.swap(*f);
        }

        //一般任务
        FiberAndThread(std::function<void()> f, int thr) :cb(f), threadId(thr) {

        }
        FiberAndThread(std::function<void()>* f, int thr) : threadId(thr) {
            cb.swap(*f);
        }

        FiberAndThread() :threadId(-1) {

        }

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            threadId = -1;
        }
    };
private:
    MutexType m_mtx;
    std::vector<Thread::ptr> m_threads;     //工作线程
    std::list<FiberAndThread> m_fibers;     //待执行的协程（任务）队列
    std::string m_name;

    Fiber::ptr m_rootFiber;                 //use_caller为true时有效, 调度协程
protected:
    std::vector<int> m_threadIds;   //线程id数组
    size_t m_threadCount = 0;       //总线程数
    std::atomic<size_t> m_activeThreadCount = {0};        //活跃线程数
    std::atomic<size_t> m_idleThreadCount = {0};          //空闲线程数
    bool m_stopping = true;
    bool m_autostop = false;
    int m_rootThread = 0;       //主线程id（user_caller）
};

}

#endif