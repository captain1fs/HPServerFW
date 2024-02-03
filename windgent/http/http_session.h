#ifndef __HTTP_SESSION_H__
#define __HTTP_SESSION_H__

#include "../socket_stream.h"
#include "http.h"

namespace windgent {
namespace http {

//服务端
class HttpSession : public SocketStream {
public:
    typedef std::shared_ptr<HttpSession> ptr;
    HttpSession(Socket::ptr socket, bool owner = true);

    //流程：接收m_socket上发来的请求报文 --> 通过HttpRequestParser::execute执行解析请求报文的所有回调函数(on_request_http_field...)来填充m_data的所有内容(请求行、首部、消息体...)
    HttpRequest::ptr recvRequest();
    int sendResponse (HttpResponse::ptr rsp);
};

}
}

#endif