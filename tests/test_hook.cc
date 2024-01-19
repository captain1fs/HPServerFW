#include "../windgent/iomanager.h"
#include "../windgent/hook.h"
#include "../windgent/log.h"

#include <sys/types.h>
// #include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

windgent::Logger::ptr g_logger = LOG_ROOT();

//两个协程分别sleep 2s和3s，正常5s才能打印两个日志。如果能在3s内打印两个日志，则说明这两个协程都没有独占线程的时间
void test_sleep() {
    windgent::IOManager iom(1);
    iom.schedule([](){
        sleep(2);
        LOG_INFO(g_logger) << "sleep 2";
    });

    iom.schedule([](){
        sleep(3);
        LOG_INFO(g_logger) << "sleep 3";
    });
    LOG_INFO(g_logger) << "test_sleep";
}

void test_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    LOG_INFO(g_logger) << "sockfd = " << sockfd;

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "39.156.66.18", &addr.sin_addr.s_addr);

    LOG_INFO(g_logger) << "begin connect";
    int ret = connect(sockfd, (const sockaddr*)&addr, sizeof(addr));
    LOG_INFO(g_logger) << "connect ret = " << ret << ", errno = " << errno;
    if(ret) {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    ret = send(sockfd, data, sizeof(data), 0);
    LOG_INFO(g_logger) << "send rt=" << ret << " errno=" << errno;
    if(ret <= 0) {
        return;
    }

    std::string buff;
    buff.resize(4096);
    ret = recv(sockfd, &buff[0], buff.size(), 0);
    LOG_INFO(g_logger) << "recv rt=" << ret << " errno=" << errno;
    if(ret <= 0) {
        return;
    }
    buff.resize(ret);
    LOG_INFO(g_logger) << buff;
}

int main(int argc, char** argv) {
    // test_sleep();
    // test_socket();

    windgent::IOManager iom;
    iom.schedule(test_socket);

    return 0;
}