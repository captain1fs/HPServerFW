#include "../windgent/iomanager.h"
#include "../windgent/hook.h"
#include "../windgent/log.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

//两个协程分别sleep 2s和3s，正常5s才能打印两个日志。如果能在3s内打印两个日志，则说明这两个协程都没有独占线程的时间
void test_sleep() {
    windgent::IOManager iom(1);
    iom.schedule([](){
        sleep(2);
        LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([](){
        sleep(3);
        LOG_INFO(g_logger) << "sleep 3";
    });
    LOG_INFO(g_logger) << "test_sleep";
}

int main(int argc, char** argv) {
    test_sleep();

    return 0;
}