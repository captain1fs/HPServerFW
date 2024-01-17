#include "./timer.h"
#include "./util.h"

namespace windgent {

bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs){
    if(!lhs && !rhs) return false;
    if(!lhs) return true;
    if(!rhs) return false;
    if(lhs->m_next < rhs->m_next) return true;
    if(lhs->m_next > rhs->m_next) return false;
    return lhs.get() < rhs.get();
}

bool Timer::cancel() {
    TimerManager::RWMutexType::WrLock lock(m_manager->m_mutex);
    if(m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh() {
    TimerManager::RWMutexType::WrLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }

    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    m_next = windgent::GetCurrentMS() + m_ms;       //刷新执行时间
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
    if(ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WrLock lock(m_manager->m_mutex);
    if(!m_cb) {
        return false;
    }

    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if(from_now) {
        start = windgent::GetCurrentMS();
    } else {
        start = m_next - m_ms;
    }
    //重置时间间隔和执行时刻
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}

Timer::Timer(uint64_t ms, std::function<void()> cb, bool isRecur, TimerManager* manager)
    : m_isRecur(isRecur), m_ms(ms), m_cb(cb), m_manager(manager) {
    m_next = windgent::GetCurrentMS() + m_ms;
}

Timer::Timer(uint64_t next):m_next(next) { }

TimerManager::TimerManager() {
    m_previouseTime = windgent::GetCurrentMS();
}

TimerManager::~TimerManager() { }

void TimerManager::addTimer(Timer::ptr timer, RWMutexType::WrLock& lock) {
    auto it = m_timers.insert(timer).first;
    bool at_front = it == m_timers.begin() && !m_tickled;
    if(at_front) {
        m_tickled = true;
    }
    lock.unlock();
    //如果插在首部，主动唤醒epoll_wait来执行此定时器
    if(at_front) {
        onTimerInsertedAtFront();
    }
}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool isRecur) {
    Timer::ptr timer(new Timer(ms, cb, isRecur, this));
    RWMutexType::WrLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

static void Ontimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = weak_cond.lock();
    if(tmp) {
        cb();
    }
}

Timer::ptr TimerManager::addCondTimer(uint64_t ms, std::function<void()> cb
                                    , std::weak_ptr<void> weak_cond, bool isRecur) {
    return addTimer(ms, std::bind(&Ontimer, weak_cond, cb), isRecur);
}

uint64_t TimerManager::getTimeOfNextTimer() {
    RWMutexType::RdLock lock(m_mutex);
    if(m_timers.empty()) {
        return ~0ull;
    }
    m_tickled = false;
    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = windgent::GetCurrentMS();
    if(now_ms >= next->m_next) {
        return 0;
    } else {
        return next->m_next - now_ms;
    }
}

bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    if(now_ms < m_previouseTime && now_ms < (m_previouseTime - 60 * 60 * 1000)) {
        rollover = true;
    }
    m_previouseTime = now_ms;
    return rollover;
}

void TimerManager::listExpiredCbs(std::vector<std::function<void()> >& cbs) {
    uint64_t now_ms = windgent::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::RdLock locK(m_mutex);
        if(m_timers.empty()) {
            return;
        }
    }
    RWMutexType::WrLock lock(m_mutex);
    if(m_timers.empty()) {
            return;
    }
    //如果服务器时间延后，则当前所有的定时器都已超时
    bool rollover = detectClockRollover(now_ms);
    if(!rollover && ((*m_timers.begin())->m_next > now_ms)) {
        return;
    }
    Timer::ptr now_timer(new Timer(now_ms));
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    //找到执行时刻<=now_ms的所有定时器，这些定时器已经超时，需要手动触发执行它们
    while(it != m_timers.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);   //删除超时的定时器
    cbs.reserve(expired.size());

    for(auto& timer : expired) {
        cbs.push_back(timer->m_cb);
        if(timer->m_isRecur) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            timer->m_cb = nullptr;
        }
    }
}

bool TimerManager::hasTimer() {
    RWMutexType::RdLock lock(m_mutex);
    return !m_timers.empty();
}

}   