#include "../windgent/http/http_server.h"
#include "../windgent/log.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

void test() {
    windgent::Address::ptr addr = windgent::Address::getAnyAddrFromHost("0.0.0.0:8020");
    windgent::http::HttpServer::ptr http_server(new windgent::http::HttpServer);
    while(!http_server->bind(addr)) {
        sleep(2);
    }

    http_server->start();
}

int main() {
    windgent::IOManager iom(2);
    iom.schedule(test);

    return 0;
}