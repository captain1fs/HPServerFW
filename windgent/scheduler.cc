#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"

namespace windgent {

windgent::Logger::ptr g_logger = LOG_NAME("system");

//当前线程的协程调度器，同⼀个调度器下的所有线程指同同⼀个调度器实例
static thread_local Scheduler* t_scheduler = nullptr;               
//当前线程的调度协程，每个线程都独有⼀份，包括caller线程，这个加上前⾯协程模块的t_fiber和t_thread_fiber，每个线程总共可以记录三个协程的上下⽂信息。
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) :m_name(name) {
    ASSERT(threads > 0);

    //是否把创建Scheduler的线程纳入Schuduler监管，如果是，则
    if(use_caller) {
        windgent::Fiber::GetThis();     //初始化线程的主协程
        --threads;                      //当前线程被纳入监管，则可创建线程数减一

        ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        //由于是把已有的线程纳入监管，所以需要为此线程创建一个子协程作为该线程的主协程并与run绑定。
        //caller线程的调度协程不会被调度器调度，只会在调度器stop时主动call，且当它停止时，应该返回caller线程的主协程
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

//调度器的停止行为要分两种情况讨论，首先是use_caller为false的情况，这种情况下，由于没有使用caller线程进行调度，那么只需要简单地等各个调度线程的调度协程退出就行了。
//如果use_caller为true，表示caller线程也要参于调度，这时，调度器初始化时记录的属于caller线程的调度协程就要起作用了，在调度器停止前，应该让这个caller线程的调度协程也运行一次，让caller线程完成调度工作后再退出。如果调度器只使用了caller线程进行调度，那么所有的调度任务要在调度器停止时才会被调度。
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
    //若当前执行run的线程不是user_caller线程，设置线程的主协程为调度协程
    if(windgent::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }
    //空闲协程，当队列中无任务时，线程执行此协程
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while(true) {
        ft.reset();
        bool tickle_me = false;     //是否tickle其他线程进⾏任务调度
        bool is_active = false;
        {
            MutexType::Lock locK(m_mtx);
            auto it = m_fibers.begin();
            while(it != m_fibers.end()) {
                //指定了调度线程，但不是在当前线程上调度，标记⼀下需要通知其他线程（指定的那个线程）进⾏调度，然后跳过这个任务，继续下⼀个
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
                // 当前调度线程找到⼀个任务，准备开始调度，将其从任务队列中剔除，活跃线程数加1
                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            // 当前线程拿完⼀个任务后，发现任务队列还有剩余，那么tickle⼀下其他线程
            tickle_me |= it != m_fibers.end();          
        }

        if(tickle_me) {
            tickle();
        }

        //协程任务，协程切换都是在任务协程和线程主协程直接的切换
        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)) {
            //resume协程，resume返回时，协程要么执⾏完了，要么半路yield了，总之这个任务就算完成了，活跃线程数减⼀
            ft.fiber->swapIn();
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
            // 如果调度器没有调度任务，那么idle协程会不停地swapIn/swapOut，不会结束，如果idle协程结束了，那⼀定是调度器停⽌了
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