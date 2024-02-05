#ifndef __HTTP_CONNECTION_H__
#define __HTTP_CONNECTION_H__

#include "../socket_stream.h"
#include "http.h"

namespace windgent {
namespace http {

//客户端使用
class HttpConnection : public SocketStream {
public:
    typedef std::shared_ptr<HttpConnection> ptr;
    HttpConnection(Socket::ptr socket, bool owner = true);

    //流程：接收m_socket上发来的响应报文 --> 通过HttpResponseParser::execute执行解析响应报文的所有回调函数(on_response_http_field...)来填充m_data的所有内容(状态行、首部、消息体...)
    HttpResponse::ptr recvResponse();
    int sendRequest (HttpRequest::ptr req);
};

}
}

#endif