#include "../windgent/windgent.h"
#include "../windgent/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

windgent::Logger::ptr g_logger = LOG_ROOT();

int sockfd = 0;

void test_fiber() {
    LOG_INFO(g_logger) << "test_fiber sock= " << sockfd;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);

    if(!connect(sockfd, (const sockaddr*)&addr, sizeof(addr))) {
        ;
    } else if(errno == EINPROGRESS) {
        LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        windgent::IOManager::GetThis()->addEvent(sockfd, windgent::IOManager::READ, [](){
            LOG_INFO(g_logger) << "read callback";
        });
        windgent::IOManager::GetThis()->addEvent(sockfd, windgent::IOManager::WRITE, [](){
            LOG_INFO(g_logger) << "write callback";
            // close(sockfd);
            windgent::IOManager::GetThis()->cancelEvent(sockfd, windgent::IOManager::READ);
            close(sockfd);
        });
    } else {
        LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test_iomanager() {
    // std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;  //1 4
    windgent::IOManager iom;
    iom.schedule(&test_fiber);
}

windgent::Timer::ptr s_timer;
void test_timer() {
    windgent::IOManager iom(1, false, "name");
    s_timer = iom.addTimer(1000, [](){
        static int i = 0;
        LOG_INFO(g_logger) << "hello timer i=" << i;
        if(++i == 3) {
            s_timer->reset(2000, true);
            // s_timer->cancel();
        }
    }, true);
}

int main() {
    test_iomanager();
    // test_timer();
    return 0;
}