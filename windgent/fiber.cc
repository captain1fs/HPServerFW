#include <atomic>
#include "./fiber.h"
#include "./macro.h"
#include "./log.h"
#include "./config.h"
#include "./scheduler.h"

namespace windgent {

static windgent::Logger::ptr g_logger = LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id {0};    //协程id
static std::atomic<uint64_t> s_fiber_count {0};    //协程数量

static windgent::ConfigVar<uint32_t>::ptr g_fiber_config = windgent::ConfigMgr::Lookup<uint32_t>("fiber.stacksize", 128 * 1024, "fiber stack size");

static thread_local Fiber* t_fiber = nullptr;   //当前正在执行的协程
static thread_local Fiber::ptr t_threadFiber = nullptr;   //主协程

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }
    static void Dealloc(void* ptr, size_t size) {
        if(!ptr) {
            free(ptr);
        }
    }
};
using StackAllocator = MallocStackAllocator;

Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);
    if(getcontext(&m_ctx)) {
        ASSERT2(false, "getcontext");
    }
    ++s_fiber_count;

    LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

//创建协程
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller) 
    :m_id(++s_fiber_id), m_cb(cb) {
    m_stacksize = stacksize ? stacksize : g_fiber_config->getVal();
    m_stack = StackAllocator::Alloc(m_stacksize);
    ++s_fiber_count;

    if(getcontext(&m_ctx)) {
        ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    if(!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }
    LOG_DEBUG(g_logger) << "Fiber::Fiber(cb, sz) id=" << m_id;
}

Fiber::~Fiber() {
    --s_fiber_count;
    if(m_stack) {
        ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    }else {
        //初始协程没有cb
        ASSERT(!m_cb);
        ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if(cur == this) {
            SetThis(nullptr);
        }
    }

    LOG_DEBUG(g_logger) << "Fiber::~Fiber() id=" << m_id;
}

//重置协程函数和状态
void Fiber::reset(std::function<void()> cb) {
    ASSERT(m_stack);
    ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
    m_cb = cb;
    if(getcontext(&m_ctx)) {
        ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

//保存主协程上下文，切换到当前协程执行
void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
    if(swapcontext(&(t_threadFiber->m_ctx), &m_ctx)) {
        ASSERT2(false, "swapcontext");
    }
}

//切换当前协程到后台执行，恢复主协程上下文
void Fiber::back() {
    SetThis(t_threadFiber.get());
    // m_state = TERM;
    if(swapcontext(&m_ctx, &(t_threadFiber->m_ctx))) {
        ASSERT2(false, "swapcontext");
    }
}

//切换到当前协程执行
void Fiber::swapIn() {
    SetThis(this);
    ASSERT(m_state != EXEC);
    m_state = EXEC;
    if(swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx)) {
        ASSERT2(false, "swapcontext");
    }
}

//切换当前协程到后台
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());
    // m_state = TERM;
    if(swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx))) {
        ASSERT2(false, "swapcontext");
    }
}

//设置当前执行的协程
void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

//返回当前协程
Fiber::ptr Fiber::GetThis() {
    if(t_fiber) {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

//切换协程到后台，并设为READY状态
void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    ASSERT(cur->m_state == EXEC);
    cur->m_state = READY;
    cur->swapOut();
}

//切换协程到后台，并设为HOLD状态
void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    ASSERT(cur->m_state == EXEC);
    cur->m_state = HOLD;
    cur->swapOut();
}

//总协程数量
uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}
//返回协程id
uint64_t Fiber::GetFiberId() {
    if(t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

//上下文入口函数
void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis();
    ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& e) {
        cur->m_state = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber::MainFunc Exception: " << e.what() << ", fiber id= " << cur->getId() << std::endl << windgent::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber::MainFunc Exception" << ", fiber id= " << cur->getId() << std::endl << windgent::BacktraceToString();
    }

    //减少一次this的引用计数，使得能够正确析构
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();

    ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc() {
    Fiber::ptr cur = GetThis();
    ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& e) {
        cur->m_state = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber::MainFunc Exception: " << e.what() << ", fiber id= " << cur->getId() << std::endl << windgent::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        LOG_ERROR(g_logger) << "Fiber::MainFunc Exception" << ", fiber id= " << cur->getId() << std::endl << windgent::BacktraceToString();
    }

    //减少一次this的引用计数，使得能够正确析构
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();

    ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

}