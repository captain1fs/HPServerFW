#ifndef __FIBER_H__
#define __FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>

namespace windgent {

class Scheduler;

//使用非对称协程的设计思路，通过主协程创建新协程，主协程由swapIn()让出执行权执行子协程的任务，子协程可以通过YieldToHold()让出执行权继续执行主协程的任务，不能在子协程之间做相互的转化，这样会导致回不到main函数的上下文。这里使用了两个线程局部变量保存当前协程和主协程，切换协程时调用swapcontext，若两个变量都保存子协程，则无法回到原来的主协程中。

class Fiber : public std::enable_shared_from_this<Fiber> {
    friend class Scheduler;
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
    //构造子协程，参数use_caller表示是否将调用线程加入线程池
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    //重置协程状态和⼊⼝函数，复⽤栈空间，不重新创建栈
    void reset(std::function<void()> cb);

    //只有被调度器执行的协程才会调用swapIn和swapOut。因为这些子协程只和调度协程切换
    //切换到当前协程执行
    void swapIn();
    //切换当前协程到后台执行
    void swapOut();

    //主协程执行次函数，切换到当前协程执行
    void call();
    //当前协程执行此函数，切换到主协程执行
    void back();

    uint64_t getId() const { return m_id; }
    State getState() const { return m_state; }

    //设置当前运行的协程
    static void SetThis(Fiber* f);
    //返回当前执行的协程，如果当前线程还未创建协程，则创建线程的第⼀个协程，且该协程为当前线程的主协程，其他协程都通过这个协程来调度，也就是说，其他协程结束时,都要切回到主协程，由主协程重新选择新的协程进⾏resume
    static Fiber::ptr GetThis();
    //切换协程到后台，并设为READY状态
    static void YieldToReady();
    //切换协程到后台，并设为HOLD状态
    static void YieldToHold();
    //总协程数量
    static uint64_t TotalFibers();
    static uint64_t GetFiberId();

    //上下文入口函数，在每个协程的独立栈空间上执行。
    //在用户传入的协程入口函数上进行了一次封装，这个封装类似于线程模块的对线程入口函数的封装。通过封装协程入口函数，可以实现协程在结束自动执行yield的操作
    static void MainFunc();

    static void CallerMainFunc();
private:
    //只⽤于创建线程的第⼀个协程，也就是线程主函数对应的协程，这个协程只能由GetThis()⽅法调⽤，所以定义成私有⽅法
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