#include "../windgent/windgent.h"

static windgent::Logger::ptr g_logger = LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    LOG_INFO(g_logger) << "------- test in fiber s_count=" << s_count;

    windgent::set_hook_enable(false);
    sleep(1);
    if(--s_count >= 0) {
        windgent::Scheduler::GetThis()->schedule(&test_fiber);  //随机到某个协程
        // windgent::Scheduler::GetThis()->schedule(&test_fiber, windgent::GetThreadId());  //指定到一个协程
    }
}

int main(int argc, char** argv) {
    g_logger->setLevel(windgent::LogLevel::INFO);
    windgent::Thread::SetName("main");
    LOG_INFO(g_logger) << "main start";
    windgent::Scheduler sc(2, false, "test");
    sc.start();
    sleep(2);
    LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    LOG_INFO(g_logger) << "main over";

    return 0;
}