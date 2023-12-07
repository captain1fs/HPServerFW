#include "../windgent/windgent.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

int cnt = 0;
windgent::RWMutex g_rw_mutex;
windgent::Mutex g_mutex;

void func1() {
    LOG_INFO(g_logger) << "name: " << windgent::Thread::GetName()
                       << " this.name: " << windgent::Thread::GetThis()->getName()
                       << " id: " << windgent::GetThreadId()
                       << " this.id: "<< windgent::Thread::GetThis()->getId();
    // sleep(1);
    for(int i = 0;i < 1000000; ++i){
        // windgent::RWMutex::WrLock lk(g_rw_mutex);
        // windgent::Mutex::Lock lk(g_mutex);
        ++cnt;
    }
}

void func2() {

}

void test_thread() {
    LOG_INFO(g_logger) << "thread test begin";
    std::vector<windgent::Thread::ptr> thrs;
    for(int i = 0;i < 5; ++i) {
        windgent::Thread::ptr thr(new windgent::Thread(&func1, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }

    for(int i = 0;i < 5; ++i) {
        thrs[i]->join();
    }
    LOG_INFO(g_logger) << "thread test end";
    LOG_INFO(g_logger) << "cnt = " << cnt;
}


int main() {

    test_thread();

    return 0;
}