#ifndef __FIBER_H__
#define __FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>

namespace windgent {

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;
    enum State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };
public:
    Fiber(std::function<void()> cb, size_t stacksize = 0);
    ~Fiber();

    //重置协程函数和状态
    void reset(std::function<void()> cb);
    //切换到当前协程执行
    void swapIn();
    //切换当前协程到后台执行
    void swapOut();

    uint64_t getId() const { return m_id; }

    //设置当前协程
    static void SetThis(Fiber* f);
    //返回当前协程
    static Fiber::ptr GetThis();
    //切换协程到后台，并设为READY状态
    static void YieldToReady();
    //切换协程到后台，并设为HOLD状态
    static void YieldToHold();
    //总协程数量
    static uint64_t TotalFibers();
    static uint64_t GetFiberId();

    //上下文入口函数，在每个协程的独立栈空间上执行
    static void MainFunc();
private:
    //只用来设置初始协程，即当前线程状态
    Fiber();

private:
    uint64_t m_id = 0;              //协程id
    uint32_t m_stacksize = 0;       //所用栈大小
    State m_state = INIT;           //协程状态

    ucontext_t m_ctx;               //上下文信息
    void* m_stack = nullptr;        //栈空间

    std::function<void()> m_cb;     //执行函数
};

}

#endif