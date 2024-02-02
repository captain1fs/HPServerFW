#include "../windgent/tcp_server.h"
#include "../windgent/iomanager.h"
#include "../windgent/log.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

void run() {
    auto addr = windgent::Address::getAnyAddrFromHost("0.0.0.0:8037");
    // auto addr2 = windgent::UnixAddress::ptr(new windgent::UnixAddress("/tmp/unix_addr"));
    // LOG_INFO(g_logger) << *addr2;

    std::vector<windgent::Address::ptr> addrs;
    addrs.push_back(addr);
    // addrs.push_back(addr2);

    windgent::TcpServer::ptr tcp_server(new windgent::TcpServer);
    std::vector<windgent::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
    // tcp_server->stop();
}

int main() {
    windgent::IOManager iom(2);
    iom.schedule(run);

    return 0;
}