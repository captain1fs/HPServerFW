#include "./socket_stream.h"
#include <vector>

namespace windgent {

SocketStream::SocketStream(Socket::ptr socket, bool owner)
    :m_socket(socket), m_owner(owner) {
}

SocketStream::~SocketStream() {
    if(m_owner && m_socket) {
        m_socket->close();
    }
}

bool SocketStream::isConnected() const {
    return m_socket->isConnected();
}

int SocketStream::read(void* buffer, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    return m_socket->recv(buffer, length);
}

int SocketStream::read(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs, length);
    int ret = m_socket->recv(&iovs[0], iovs.size());
    if(ret > 0) {
        ba->setPosition(ba->getPosition() + ret);
    }
    return ret;
}

int SocketStream::write(const void* buffer, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    return m_socket->send(buffer, length);
}

int SocketStream::write(ByteArray::ptr ba, size_t length) {
    if(!isConnected()) {
        return -1;
    }
    std::vector<iovec> iovs;
    ba->getWriteBuffers(iovs, length);
    int ret = m_socket->send(&iovs[0], iovs.size());
    if(ret > 0) {
        ba->setPosition(ba->getPosition() + ret);
    }
    return ret;
}

void SocketStream::close() {
    if(m_socket) {
        m_socket->close();
    }
}

}