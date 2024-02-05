#include "./http_connection.h"
#include "./http_parser.h"
#include "../log.h"

namespace windgent {
namespace http {

static windgent::Logger::ptr g_logger = LOG_NAME("system");

HttpConnection::HttpConnection(Socket::ptr socket, bool owner)
    :SocketStream(socket, owner) {
}

HttpResponse::ptr HttpConnection::recvResponse() {
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t buffer_size = HttpResponseParser::getHttpResponseBufferSize();
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
        size_t nparse = parser->execute(data, len, false);     //解析数据并从data中移除
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
    } while(true);
    //已经填充完状态行和首部，开始填充消息体
    auto& client_parser = parser->getParser();
    std::string body;
    if(client_parser.chunked) {
       int len = last_size;
        do {
            bool begin = true;
            do {
                if(!begin || len == 0) {
                    int rt = read(data + len, buffer_size - len);
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }
                    len += rt;
                }
                data[len] = '\0';
                size_t nparse = parser->execute(data, len, true);
                if(parser->hasError()) {
                    close();
                    return nullptr;
                }
                len -= nparse;
                if(len == (int)buffer_size) {
                    close();
                    return nullptr;
                }
                begin = false;
            } while(!parser->isFinished());
            //len -= 2;
            
            LOG_DEBUG(g_logger) << "content_len=" << client_parser.content_len;
            if(client_parser.content_len + 2 <= len) {
                body.append(data, client_parser.content_len);
                memmove(data, data + client_parser.content_len + 2
                        , len - client_parser.content_len - 2);
                len -= client_parser.content_len + 2;
            } else {
                body.append(data, len);
                int left = client_parser.content_len - len + 2;
                while(left > 0) {
                    int rt = read(data, left > (int)buffer_size ? (int)buffer_size : left);
                    if(rt <= 0) {
                        close();
                        return nullptr;
                    }
                    body.append(data, rt);
                    left -= rt;
                }
                body.resize(body.size() - 2);
                len = 0;
            }
        } while(!client_parser.chunks_done);
        parser->getData()->setBody(body);
    } else {
        uint64_t length = parser->getContentLength();
        if(length > 0) {
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
    }

    return parser->getData();
}

int HttpConnection::sendRequest (HttpRequest::ptr req) {
    std::stringstream ss;
    ss << *req;
    std::string data = ss.str();
    return writeFixedSize(data.c_str(), data.size());
}

}
}