#include "./http_session.h"
#include "./http_parser.h"

namespace windgent {
namespace http {

HttpSession::HttpSession(Socket::ptr socket, bool owner)
    :SocketStream(socket, owner) {
}

HttpRequest::ptr HttpSession::recvRequest() {
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buffer_size = HttpRequestParser::getHttpRequestBufferSize();
    std::shared_ptr<char> buffer(new char[buffer_size], [](char* ptr){ delete[] ptr; });
    char* data = buffer.get();
    uint64_t last_size = 0;      //data中剩余的未解析完的数据大小

    do {
        int len = read(data + last_size, buffer_size - last_size);      //SockStream::read
        if(len <= 0) {              //error
            close();
            return nullptr;
        }
        len += last_size;
        size_t nparse = parser->execute(data, len);     //解析数据并从data中移除
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        last_size = len - nparse;
        if(last_size == buffer_size) {
            close();
            return nullptr;
        }
        if(parser->isFinished()) {
            break;
        }
    }while(true);
    //已经填充完请求行和首部，开始填充消息体
    uint64_t length = parser->getContentLength();
    if(length > 0) {
        std::string body;
        body.resize(length);
        int len = 0;
        if(length >= last_size) {
            memcpy(&body[0], data, last_size);
            len = last_size;
        } else {
            memcpy(&body[0], data, length);
            len = length;
        }
        length -= last_size;
        if(length > 0) {
            if(readFixedSize(&body[len], length) <= 0) {
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }

    return parser->getData();
}

int HttpSession::sendResponse (HttpResponse::ptr rsp) {
    std::stringstream ss;
    ss << *rsp;
    std::string data = ss.str();
    return writeFixedSize(data.c_str(), data.size());
}

}
}