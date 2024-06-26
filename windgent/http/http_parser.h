#ifndef __HTTP_PARSER_H__
#define __HTTP_PARSER_H__

#include "./http.h"
#include "./http11_parser.h"
#include "./httpclient_parser.h"

namespace windgent {
namespace http {

class HttpRequestParser {
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser();
    //解析请求报文，返回已经解析的长度，并移除已经解析的数据
    size_t execute(char* data, size_t len);
    int isFinished();
    int hasError();

    const http_parser& getParser() const { return m_parser; }
    HttpRequest::ptr getData() const { return m_data; }
    uint64_t getContentLength();
    void setError(int v) { m_error = v; }

    static uint64_t getHttpRequestBufferSize();
    static uint64_t getHttpRequestMaxBodySize();
private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    /// 1000: invalid method
    /// 1001: invalid version
    /// 1002: invalid field
    int m_error;
};

class HttpResponseParser {
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    HttpResponseParser();

    size_t execute(char* data, size_t len, bool chunk);
    int isFinished();
    int hasError();

    void setError(int v) { m_error = v; }
    HttpResponse::ptr getData() const { return m_data; }
    uint64_t getContentLength();
    const httpclient_parser& getParser() const { return m_parser; }

    static uint64_t getHttpResponseBufferSize();
    static uint64_t getHttpResponseMaxBodySize();
private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    /// 1001: invalid version
    /// 1002: invalid field
    int m_error;
};

}
}


#endif