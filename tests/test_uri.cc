#include "../windgent/uri.h"

void test() {
    windgent::Uri::ptr uri = windgent::Uri::Create("http://www.baidu.com/test/中文/uri?id=100&name=windgent#fragment中文");
    std::cout << uri->toString() << std::endl;
    auto addr = uri->getAddrFromUri();
    std::cout << *addr << std::endl;
}

int main() {
    test();

    return 0;
}