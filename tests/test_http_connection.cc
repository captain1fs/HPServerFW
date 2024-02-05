#include "../windgent/http/http_connection.h"
#include "../windgent/log.h"
#include "../windgent/iomanager.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

void test() {
    windgent::Address::ptr addr = windgent::Address::getAnyAddrFromHost("www.baidu.com:80");
    if(!addr) {
        LOG_ERROR(g_logger) << "get addr error";
        return;
    }
    windgent::Socket::ptr sock = windgent::Socket::createTCP(addr);
    if(!sock->connect(addr)) {
        LOG_ERROR(g_logger) << "connect " << *addr << " failed!";
        return;
    }
    windgent::http::HttpConnection::ptr conn(new windgent::http::HttpConnection(sock));
    windgent::http::HttpRequest::ptr req(new windgent::http::HttpRequest);
    req->setPath("/s");
    req->setQuery("wd=主题教育总结会议在京召开");
    req->setHeader("host", "www.baidu.com");
    LOG_INFO(g_logger) << "req: " << std::endl << *req;

    conn->sendRequest(req);
    auto rsp = conn->recvResponse();
    if(!rsp) {
        LOG_ERROR(g_logger) << "recv response error";
        return;
    }
    LOG_INFO(g_logger) << "rsp: " << std::endl << *rsp;
}

int main() {
    windgent::IOManager iom(2);
    iom.schedule(test);

    return 0;
}