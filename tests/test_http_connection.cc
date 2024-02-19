#include "../windgent/http/http_connection.h"
#include "../windgent/log.h"
#include "../windgent/iomanager.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

void test_pool() {
    windgent::http::HttpConnectionPool::ptr pool(new windgent::http::HttpConnectionPool("www.baidu.com", "", 80, 10, 1000 * 30, 20));
    windgent::IOManager::GetThis()->addTimer(1000, [pool]() {
        auto r = pool->doGet("/", 300);
        LOG_INFO(g_logger) << r->toString();
    }, true);
}

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
    // LOG_INFO(g_logger) << "rsp: " << std::endl << *rsp;

    LOG_INFO(g_logger) << "--------------------------------------------------------";
    windgent::http::HttpConnectionPool::ptr pool(new windgent::http::HttpConnectionPool("www.baid0u.com", "", 80, 10, 1000 * 30, 5));
    windgent::IOManager::GetThis()->addTimer(1000, [pool]() {
        auto r = pool->doGet("/", 300);
        LOG_INFO(g_logger) << r->toString();
    }, true);
}

void test2() {
    auto res = windgent::http::HttpConnection::doGet("http://www.baidu.com/s?wd=ragel&rsv_spt=1&rsv_iqid=0xf5e8015f0004506f&issp=1&f=8&rsv_bp=1&rsv_idx=2&ie=utf-8&rqlang=cn&tn=15007414_20_dg&rsv_enter=1&rsv_dl=tb&oq=%25E7%25B1%25BB%25E5%2586%2585%25E5%25AD%2598%25E5%25AF%25B9%25E9%25BD%2590&rsv_btype=t&inputT=2537&rsv_t=df76ORYIrmrAZWmMzGEmB5PP%2F%2BjgPN86n35gAwporgZ68KuSe3C3LYxPDgGXXooRChQPNIE&rsv_pq=b34334040004cb73&rsv_sug3=29&rsv_sug1=28&rsv_sug7=100&rsv_sug2=0&rsv_sug4=2537", 5000);

    LOG_INFO(g_logger) << "result= " << res->result << ", response= " << *(res->response) << ", error= " << res->error;
    LOG_INFO(g_logger) << "--------------------------------------------------------";
    test_pool();
}

int main() {
    windgent::IOManager iom(2);
    iom.schedule(test);

    return 0;
}