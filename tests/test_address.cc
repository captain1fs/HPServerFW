#include "../windgent/address.h"
#include "../windgent/log.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

void test() {
    std::vector<windgent::Address::ptr> addrs;

    bool ret = windgent::Address::getAllAddrFromHost(addrs, "www.baidu.com:ftp");
    if(!ret) {
        LOG_ERROR(g_logger) << "getAllAddrFromHost failed!";
        return;
    }
    for(size_t i = 0;i < addrs.size(); ++i) {
        LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<windgent::Address::ptr, uint32_t> > res;
    bool ret = windgent::Address::getInterfaceAddress(res);
    if(!ret) {
        LOG_ERROR(g_logger) << "getInterfaceAddress failed!";
        return;
    }
    for(auto& i : res) {
        LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() << " - " << i.second.second;
    }
}

void test_ipv4() {
    auto addr = windgent::IPAddress::Create("www.baidu.com");
    if(addr) {
        LOG_INFO(g_logger) << addr->toString();
    }
}

int main() {
    // test();
    // test_iface();
    test_ipv4();

    return 0;
}