#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include <memory>
#include "../tcp_server.h"
#include "http_session.h"

namespace windgent {
namespace http {

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;

    HttpServer(bool keep_alive = false, IOManager* worker = IOManager::GetThis()
              ,IOManager* accept_worker = IOManager::GetThis());
protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    bool m_isKeepAlive;
};

}
}

#endif