#include "../windgent/socket.h"
#include "../windgent/address.h"
#include "../windgent/log.h"
#include "../windgent/iomanager.h"

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

void test2() {
    windgent::IPAddress::ptr addr = windgent::Address::getAnyIPAddrFromHost("www.baidu.com:80");
    if(addr) {
        LOG_INFO(g_logger) << "get address: " << addr->toString();
    } else {
        LOG_ERROR(g_logger) << "get address fail";
        return;
    }

    windgent::Socket::ptr sock = windgent::Socket::createTCP(addr);
    if(!sock->connect(addr)) {
        LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }

    uint64_t ts = windgent::GetCurrentUS();
    for(size_t i = 0; i < 10000000000ul; ++i) {
        if(int err = sock->getError()) {
            LOG_INFO(g_logger) << "err=" << err << " errstr=" << strerror(err);
            break;
        }

        //struct tcp_info tcp_info;
        //if(!sock->getOption(IPPROTO_TCP, TCP_INFO, tcp_info)) {
        //    SYLAR_LOG_INFO(g_logger) << "err";
        //    break;
        //}
        //if(tcp_info.tcpi_state != TCP_ESTABLISHED) {
        //    SYLAR_LOG_INFO(g_logger)
        //            << " state=" << (int)tcp_info.tcpi_state;
        //    break;
        //}
        static int batch = 10000000;
        if(i && (i % batch) == 0) {
            uint64_t ts2 = windgent::GetCurrentUS();
            LOG_INFO(g_logger) << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
            ts = ts2;
        }
    }
}

int main() {
    // test_socket();
    windgent::IOManager iom;
    iom.schedule(&test_socket);

    return 0;
}