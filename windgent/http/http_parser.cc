#include "./http_parser.h"
#include "../log.h"
#include "../config.h"

namespace windgent {
namespace http {

static windgent::Logger::ptr g_logger = LOG_NAME("system");
static windgent::ConfigVar<uint64_t>::ptr g_http_request_buffer_size 
    = windgent::ConfigMgr::Lookup<uint64_t>("http.request.buffer_size" , 4 * 1024ull, "http request buffer size");
static windgent::ConfigVar<uint64_t>::ptr g_http_request_max_body_size 
    = windgent::ConfigMgr::Lookup<uint64_t>("http.request.max_body_size", 64 * 1024 * 1024ull, "http request max body size");

//在main函数执行之前初始化
static uint64_t s_http_request_buffer_size = 0;
static uint64_t s_http_request_max_body_size = 0;
struct _RequestSizeIniter {
    _RequestSizeIniter() {
        s_http_request_buffer_size = g_http_request_buffer_size->getVal();
        s_http_request_max_body_size = g_http_request_max_body_size->getVal();

        g_http_request_buffer_size->addListener([](const uint64_t& old_val, const uint64_t& new_val){
            s_http_request_buffer_size = new_val;
        });
        g_http_request_max_body_size->addListener([](const uint64_t& old_val, const uint64_t& new_val){
            s_http_request_max_body_size = new_val;
        });
    }
};
static _RequestSizeIniter _Initer;

void on_request_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen) {
    if(flen == 0) {
        LOG_WARN(g_logger) << "Invalid http request field length = 0";
        return;
    }
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}

void on_request_method(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    HttpMethod m = Chars2HttpMethod(at);
    if(m == HttpMethod::INVALID_METHOD) {
        LOG_WARN(g_logger) << "Invalid request method: " << std::string(at, length);
        parser->setError(1000);
        return;
    }
    parser->getData()->setMethod(m);
}

void on_request_uri(void *data, const char *at, size_t length) {
    // HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
}

void on_request_fragment(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setFragment(std::string(at, length));
}

void on_request_path(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setPath(std::string(at, length));
}

void on_request_query(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setQuery(std::string(at, length));
}

void on_request_version(void *data, const char *at, size_t length) {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    uint8_t v = 0;
    if(strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if(strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        LOG_WARN(g_logger) << "Invalid http request version: " << std::string(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(v);
}

void on_request_http_done(void *data, const char *at, size_t length) {
    // HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
}

HttpRequestParser::HttpRequestParser():m_error(0) {
    m_data.reset(new windgent::http::HttpRequest);
    http_parser_init(&m_parser);
    m_parser.http_field = on_request_http_field;
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query;
    m_parser.http_version = on_request_version;
    m_parser.header_done = on_request_http_done;
    m_parser.data = this;
}

size_t HttpRequestParser::execute(char* data, size_t len) {
    size_t offset = http_parser_execute(&m_parser, data, len, 0);
    memmove(data, data + offset, (len-offset));
    return offset;
}

int HttpRequestParser::isFinished() {
    return http_parser_is_finished(&m_parser);
}

int HttpRequestParser::hasError() {
    return m_error || http_parser_has_error(&m_parser);
}

uint64_t HttpRequestParser::getContentLength() {
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

void on_response_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen) {
    if(flen == 0) {
        LOG_WARN(g_logger) << "Invalid http response field length = 0";
        return;
    }
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    parser->getData()->setHeader(std::string(field, flen), std::string(value, vlen));
}

void on_response_reason(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    parser->getData()->setReason(std::string(at, length));
}

void on_response_status(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    HttpStatus s = (HttpStatus)(atoi(at));
    parser->getData()->setStatus(s);
}

void on_response_chunk_size(void *data, const char *at, size_t length) {
}

void on_response_version(void *data, const char *at, size_t length) {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    uint8_t v = 0;
    if(strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if(strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        LOG_WARN(g_logger) << "Invalid http response version: " << std::string(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(v);
}

void on_response_header_done(void *data, const char *at, size_t length) {
}

void on_response_last_chunk(void *data, const char *at, size_t length) {
}

HttpResponseParser::HttpResponseParser():m_error(0) {
    m_data.reset(new windgent::http::HttpResponse);
    httpclient_parser_init(&m_parser);
    m_parser.http_field = on_response_http_field;
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code = on_response_status;
    m_parser.chunk_size = on_response_chunk_size;
    m_parser.http_version = on_response_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    m_parser.data = this;
}

size_t HttpResponseParser::execute(char* data, size_t len, bool chunk) {
    if(chunk) {
        httpclient_parser_init(&m_parser);
    }
    size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);
    memmove(data, data + offset, (len-offset));
    return offset;
}

int HttpResponseParser::isFinished() {
    return httpclient_parser_is_finished(&m_parser);
}

int HttpResponseParser::hasError() {
    return m_error || httpclient_parser_has_error(&m_parser);
}

uint64_t HttpResponseParser::getContentLength() {
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}

}
}