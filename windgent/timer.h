#ifndef __TIMER_H__
#define __TIMER_H__

#include "./mutex.h"

#include <iostream>
#include <memory>
#include <functional>
#include <set>
#include <vector>

namespace windgent {

class TimerManager;
//定时器类
class Timer : public std::enable_shared_from_this<Timer> {
    friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;
    //取消定时器
    bool cancel();
    //刷新定时器的执行时刻
    bool refresh();
    //重置定时器
    bool reset(uint64_t ms, bool from_now);
private:
    Timer(uint64_t ms, std::function<void()> cb, bool isRecur, TimerManager* manager);
    Timer(uint64_t next);   //为了便于根据m_next查找
private:
    bool m_isRecur = false;             //是否是循环定时器
    uint64_t m_ms = 0;                  //执行周期
    uint64_t m_next = 0;                //精确的执行时间，用于什么时候超时
    std::function<void()> m_cb;         //回调函数
    TimerManager* m_manager = nullptr;  //定时器所属的TimeManager
private:
    struct Comparator {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs);
    };
};

//定时器管理类，使用set来管理
class TimerManager {
    friend class Timer;
public:
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();

    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool isRecur = false);
    //添加条件定时器，满足条件weak_cond时才会执行
    Timer::ptr addCondTimer(uint64_t ms, std::function<void()> cb
                            , std::weak_ptr<void> weak_cond, bool isRecur = false);

    //获取到下一定时器执行要等待的时间
    uint64_t getTimeOfNextTimer();
    //获取所有超时的定时器的回调函数，从而创建出协程来schedule
    void listExpiredCbs(std::vector<std::function<void()> >& cbs);
    //是否有定时器任务
    bool hasTimer();
protected:
    //如果有新的定时器插入到首部时，表示这个定时器任务很快就会执行，此时应主动将IOManager从epoll_wait中唤醒来执行此任务
    //因为epoll_wait等待的时间TIMEOUT可能太长
    virtual void onTimerInsertedAtFront() = 0;
    //添加定时器，供上层调用
    void addTimer(Timer::ptr timer, RWMutexType::WrLock& lock);
private:
    //检测服务器时间是否延后
    bool detectClockRollover(uint64_t now_ms);
private:
    RWMutexType m_mutex;
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    //是否触发onTimerInsertedAtFront,因为可能发生频繁修改
    bool m_tickled = false;
    uint64_t m_previouseTime = 0;
};

}

#endif