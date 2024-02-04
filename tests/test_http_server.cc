#include "../windgent/http/http_server.h"
#include "../windgent/http/http_session.h"
#include "../windgent/http/servlet.h"
#include "../windgent/log.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

void test() {
    windgent::Address::ptr addr = windgent::Address::getAnyAddrFromHost("0.0.0.0:8020");
    windgent::http::HttpServer::ptr http_server(new windgent::http::HttpServer);
    while(!http_server->bind(addr)) {
        sleep(2);
    }
    auto sd = http_server->getDispatcher();
    sd->addServlet("/windgent/xxx", [](windgent::http::HttpRequest::ptr req
                                      ,windgent::http::HttpResponse::ptr rsp
                                      ,windgent::http::HttpSession::ptr session){
        rsp->setBody(req->toString());
        return 0;
    });
    sd->addGlobServlet("/windgent/*", [](windgent::http::HttpRequest::ptr req
                                      ,windgent::http::HttpResponse::ptr rsp
                                      ,windgent::http::HttpSession::ptr session){
        rsp->setBody("Glob:\r\n" + req->toString());
        return 0;
    });
    http_server->start();
}

int main() {
    windgent::IOManager iom(2);
    iom.schedule(test);

    return 0;
}