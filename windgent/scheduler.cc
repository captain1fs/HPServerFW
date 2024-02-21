#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"

namespace windgent {

windgent::Logger::ptr g_logger = LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;               //当前线程的协程调度器
static thread_local Fiber* t_scheduler_fiber = nullptr;             //当前线程的主协程

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) :m_name(name) {
    ASSERT(threads > 0);

    //是否把创建Scheduler的线程纳入Schuduler监管，如果是，则
    if(use_caller) {
        windgent::Fiber::GetThis();     //返回/创建一个主协程
        --threads;          //当前线程被纳入监管，则可创建线程数减一

        ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        //由于是把已有的线程纳入监管，所以需要为此线程创建一个子协程作为该线程的主协程并与run绑定
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        windgent::Thread::SetName(m_name);
        t_scheduler_fiber = m_rootFiber.get();

        m_rootThread = windgent::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    ASSERT(m_stopping);
    if(GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

void Scheduler::setThis() {
    t_scheduler = this;
}

Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mtx);
    if(!m_stopping) {
        return;
    }
    m_stopping = false;
    ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0;i < m_threadCount; ++i) {
        //而对于新的线程，直接将run作为其执行函数，线程立即开始执行run
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();
    LOG_INFO(g_logger) << "start";
}

void Scheduler::stop() {
    LOG_INFO(g_logger) << "stop";
    m_autostop = true;
    // 使用use_caller,并且只有一个线程，并且主协程的状态为结束或者初始化
    if(m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT)) {
        LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if(stopping()) {
            return;
        }
    }
    //use_caller线程，当前调度器应和t_scheduler相同，因为构造函数中设置了
    if(m_rootThread != -1) {
        ASSERT(GetThis() == this);
    } else {
        //非use_caller线程，此时t_scheduler=nullptr
        ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }
    //如果使用了use_caller，需要多tickle一下
    if(m_rootFiber) {
        tickle();
    }
    //使用use_caller，只要没达到停止条件，让线程主协程去执行run。因为use_caller线程不像其他线程一开始就绑定run并执行，
    //而必须通过它自己的主协程来执行
    if(m_rootFiber) {
        LOG_DEBUG(g_logger) << "before m_rootFiber->call()";
        if(!stopping()) {
            LOG_DEBUG(g_logger) << "m_rootFiber->call()";
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mtx);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }
}

void Scheduler::run() {
    LOG_INFO(g_logger) << "run";
    set_hook_enable(true);
    setThis();
    //若当前执行run的线程不是user_caller线程，设置当前协程为线程的主协程
    if(windgent::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }
    //空闲协程，当队列中无任务时，线程执行此协程
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while(true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock locK(m_mtx);
            auto it = m_fibers.begin();
            while(it != m_fibers.end()) {
                //如果当前任务指定的线程id不是当前线程，跳过
                if(it->threadId != -1 && it->threadId != windgent::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }
                ASSERT(it->fiber || it->cb);
                //协程已经处于执行态
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            tickle_me |= it != m_fibers.end();
        }

        if(tickle_me) {
            tickle();
        }

        //协程任务，协程切换都是在任务协程和线程主协程直接的切换
        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)) {
            ft.fiber->swapIn();
            //到此步时，协程已经执行完成，活跃线程数-1
            --m_activeThreadCount;

            if(ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if(ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT) {
                //没有执行完毕
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        } else if(ft.cb) {      //一般任务
            if(cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }

            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;

            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if(cb_fiber->getState() == Fiber::TERM || cb_fiber->getState() == Fiber::EXCEPT) {       //执行完毕或者出现异常
                cb_fiber->reset(nullptr);
            } else {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {        //空闲时
             if(is_active) {
                --m_activeThreadCount;
                continue;
            }
            
            if(idle_fiber->getState() == Fiber::TERM) {
                LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

void Scheduler::tickle() {
    LOG_DEBUG(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mtx);
    return m_stopping && m_autostop && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        windgent::Fiber::YieldToHold();
    }
}

void Scheduler::switchTo(int thread) {
    ASSERT(Scheduler::GetThis() != nullptr);
    if(this == Scheduler::GetThis()) {
        //线程id=-1或者就是当前线程，则无需切换
        if(thread == -1 || thread == windgent::GetThreadId()) {
            return;
        }
    }
    schedule(Fiber::GetThis(), thread);
    Fiber::YieldToHold();
}

std::ostream& Scheduler::dump(std::ostream& os) {
    os << "[Scheduler name=" << m_name
       << " size=" << m_threadCount
       << " active_count=" << m_activeThreadCount
       << " idle_count=" << m_idleThreadCount
       << " stopping=" << m_stopping
       << " ]" << std::endl << "    ";

    for(size_t i = 0; i < m_threadIds.size(); ++i) {
        if(i) {
            os << ", ";
        }
        os << m_threadIds[i];
    }
    return os;
}

}