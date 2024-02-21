#include "../windgent/windgent.h"
#include "../windgent/iomanager.h"
#include "../windgent/hook.h"
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
    inet_pton(AF_INET, "112.80.248.75", &addr.sin_addr.s_addr);

    if(!connect(sockfd, (const sockaddr*)&addr, sizeof(addr))) {
        ;
    } else if(errno == EINPROGRESS) {
        LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        windgent::IOManager::GetThis()->addEvent(sockfd, windgent::IOManager::READ, [](){
            LOG_INFO(g_logger) << "read callback";
            char temp[1000];
            int rt = read(sockfd, temp, 1000);
            if (rt >= 0) {
                std::string ans(temp, rt);
                LOG_INFO(g_logger) << "read:["<< ans << "]";
            } else {
                LOG_INFO(g_logger) << "read rt = " << rt;
            }
        });

        windgent::IOManager::GetThis()->addEvent(sockfd, windgent::IOManager::WRITE, [](){
            LOG_INFO(g_logger) << "write callback";
            int rt = write(sockfd, "GET / HTTP/1.1\r\ncontent-length: 0\r\n\r\n",38);
            LOG_INFO(g_logger) << "write rt = " << rt;
            // windgent::IOManager::GetThis()->cancelEvent(sockfd, windgent::IOManager::READ);
            // close(sockfd);
        });
    } else {
        LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test_iomanager() {
    // std::cout << "EPOLLIN=" << EPOLLIN << " EPOLLOUT=" << EPOLLOUT << std::endl;  //1 4
    windgent::IOManager iom(2, true, "IOM");
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
    g_logger->setLevel(windgent::LogLevel::INFO);
    test_iomanager();
    // test_timer();
    return 0;
}

//执行流程：
//1. 在test01()中创建了IO协程调度器，并且会start()启动1个线程（901）执行任务run()，然后在出函数时析构执行stop()让当前线程（900）去执行任务run()。
//2. 当线程901在run()中拿到test_fiber执行cb_fiber协程，它创建socket，与百度建立连接，并且注册了读事件和写事件，此时test_fiber执行完毕，回到run()中，继续执行idle()。
//3. 在idle()中epoll_wait()监听到写事件，所以将写事件的回调函数加入到任务队列中，然后交出自己的执行权，回到run()中。
//4. 线程901拿到了写任务，执行写操作，然后继续陷入idle()中。
//5. 在idle()中，当消息回来时，epoll_wait()监听到了读事件，此时将读事件的回调函数加入到任务队列中，然后交出自己的执行权，回到run()中。
//6. 线程900拿到了读任务，执行写操作，然后继续陷入idle()中，但此时所有任务已经结束了，达到结束条件。与此同时，由于已经没有事件能够唤醒epoll_wait()，所以线程901在idle()中睡了3秒，等线程900处理完读任务后达到了停止条件，也结束了。
//7. 此时任务都已经执行完毕了，达到了停止条件，idle()结束，线程901结束任务，线程900回到stop()中，继续完成析构函数，此时test01()真正的结束了。