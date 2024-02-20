#include "../windgent/http/http_server.h"
#include "../windgent/log.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

void test() {
    g_logger->setLevel(windgent::LogLevel::INFO);
    windgent::Address::ptr addr = windgent::Address::getAnyIPAddrFromHost("0.0.0.0:8020");
    if(!addr) {
        LOG_ERROR(g_logger) << "Get addr failed";
        return;
    }
    windgent::http::HttpServer::ptr http_server(new windgent::http::HttpServer(true));
    while(!http_server->bind(addr)) {
        LOG_ERROR(g_logger) << "Bind " << *addr << " failed";
        sleep(1);
    }
    http_server->start();
}

int main() {
    windgent::IOManager iom(2);
    iom.schedule(test);
    return 0;
}