#include "../windgent/http/http.h"
#include "../windgent/log.h"
#include "../windgent/socket.h"
#include "../windgent/address.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

void test_request() {
    windgent::IPAddress::ptr addr = windgent::IPAddress::getAnyIPAddrFromHost("www.baidu.com", AF_INET);
    if(addr) {
        LOG_INFO(g_logger) << "get address: " << addr->toString();
    }
    addr->setPort(80);
    
    windgent::Socket::ptr sock = windgent::Socket::createTCP(addr);
    if(!sock->connect(addr)) {
        LOG_ERROR(g_logger) << "connect " << addr->toString() << " failed!";
    } else {
        LOG_INFO(g_logger) << "connect " << addr->toString() << " success!";
    }

    windgent::http::HttpRequest::ptr req(new windgent::http::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    req->setBody("hello baidu");

    // req->dump(std::cout) << std::endl;

    // const char data[] = "GET / HTTP/1.0\r\n\r\n";
    int ret = sock->send(req->toString().c_str(), req->toString().size(), 0);
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

void test_response() {
    windgent::http::HttpResponse::ptr rsp(new windgent::http::HttpResponse);
    rsp->setHeader("xxx", "windgent");
    rsp->setBody("hello windgent");
    rsp->setClose(false);
    rsp->setStatus((windgent::http::HttpStatus)400);

    rsp->dump(std::cout) << std::endl;
}

int main() {
    // test_request();
    test_response();

    return 0;
}