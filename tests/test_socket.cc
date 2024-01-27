#include "../windgent/socket.h"
#include "../windgent/address.h"
#include "../windgent/log.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

void test_socket() {
    windgent::IPAddress::ptr addr = windgent::IPAddress::getAnyIPAddrFromHost("www.baidu.com", AF_INET);
    if(addr) {
        LOG_INFO(g_logger) << "get address: " << addr->toString();
    }
    addr->setPort(80);
    
    windgent::Socket::ptr sock = windgent::Socket::createTCP(addr);
    if(!sock->connect(addr)) {
        LOG_ERROR(g_logger) << "connect " << addr->toString() << " failed!";
    } else {
        LOG_ERROR(g_logger) << "connect " << addr->toString() << " success!";
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    int ret = sock->send(data, sizeof(data), 0);
    LOG_INFO(g_logger) << "send rt=" << ret << " errno=" << errno;
    if(ret <= 0) {
        return;
    }

    std::string buff;
    buff.resize(4096);
    ret = sock->recv(&buff[0], buff.size(), 0);
    LOG_INFO(g_logger) << "recv rt=" << ret << " errno=" << errno;
    if(ret <= 0) {
        return;
    }
    buff.resize(ret);
    LOG_INFO(g_logger) << buff;
}

int main() {
    test_socket();

    return 0;
}