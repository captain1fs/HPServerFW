#include "../windgent/log.h"
#include "../windgent/tcp_server.h"
#include "../windgent/iomanager.h"
#include "../windgent/bytearray.h"
#include "../windgent/address.h"

windgent::Logger::ptr g_logger = LOG_ROOT();

class EchoServer : public windgent::TcpServer {
public:
    typedef std::shared_ptr<EchoServer> ptr;
    EchoServer(int type):m_type(type) { }
protected:
    void handleClient(windgent::Socket::ptr client) {
        LOG_INFO(g_logger) << "Welcome client: " << *client;
        windgent::ByteArray::ptr ba(new windgent::ByteArray);
        while(true) {
            ba->clear();
            std::vector<iovec> iovs;
            ba->getWriteBuffers(iovs, 1024);
            int ret = client->recv(&iovs[0], iovs.size());
            if(0 == ret) {
                LOG_INFO(g_logger) << "client closed";
                break;
            } else if(ret < 0) {
                LOG_ERROR(g_logger) << "client error, ret = " << ret 
                                    << ", errno = " << ", errstr = " << strerror(errno);
                break;
            }
            
            ba->setPosition(ba->getPosition() + ret);
            ba->setPosition(0);
            if(1 == m_type) {
                // LOG_INFO(g_logger) << ba->toString();
                std::cout << ba->toString();
            } else {
                // LOG_INFO(g_logger) << ba->toHexString();
                std::cout << ba->toHexString();
            }
            std::cout.flush();
        }
    }
private:
    int m_type = 0;     //数据格式：二进制 or 文本
};

int type = 1;
void run() {
    EchoServer::ptr es(new EchoServer(type));
    auto addr = windgent::Address::getAnyAddrFromHost("0.0.0.0:8020");
    while(!es->bind(addr)) {
        sleep(2);
    }
    es->start();
}

int main(int argc, char** argv) {
    if(argc < 2) {
        LOG_ERROR(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 0;
    }
    if(!strcmp(argv[1], "-b")) {
        type = 2;
    }
    windgent::IOManager iom(2);
    iom.schedule(run);
    return 0;
}