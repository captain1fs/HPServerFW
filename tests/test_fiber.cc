#include "../windgent/windgent.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

void run_in_fiber() {
    LOG_INFO(g_logger) << "run in fiber begin";
    windgent::Fiber::YieldToHold();
    LOG_INFO(g_logger) << "run in fiber end";
    // windgent::Fiber::YieldToHold();
}

void test_fiber() {
    LOG_INFO(g_logger) << "main begin -1";
    {
        windgent::Fiber::GetThis();
        LOG_INFO(g_logger) << "main begin";
        windgent::Fiber::ptr fiber(new windgent::Fiber(run_in_fiber));
        fiber->swapIn();
        LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        LOG_INFO(g_logger) << "main end";
        // fiber->swapIn();
    }
    LOG_INFO(g_logger) << "main end 0";
}

int main(int argc, char** argv) {
    windgent::Thread::SetName("main");

    std::vector<windgent::Thread::ptr> thds;
    for(int i = 0;i < 1; ++i) {
        thds.push_back(windgent::Thread::ptr(new windgent::Thread(&test_fiber, "name_" + std::to_string(i))));
    }

    for(int i = 0;i < 1; ++i) {
        thds[i]->join();
    }
    
    return 0;
}