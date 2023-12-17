#include "../windgent/windgent.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

void test_assert() {
    // LOG_INFO(g_logger) << windgent::BacktraceToString(10);
    ASSERT2(0 == 1, "abf");
}

int main() {
    
    test_assert();
    return 0;
}