#include "network/tcp_connection.h"
#include "network/reactor.h"
#include "common/logger.h"
#include <cassert>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

namespace order_engine {
namespace network {

TcpConnection::TcpConnection(int sockfd, const struct sockaddr_in& peer_addr)
    : sockfd_(sockfd)
    , peer_addr_(peer_addr)
    , state_(kConnecting)
    , last_active_time_(time(nullptr)) {
    
    LOG_DEBUG("TcpConnection created");
    
    // 设置socket选项
#ifdef _WIN32
    BOOL on = TRUE;
    setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, (const char*)&on, sizeof(on));
    setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, (const char*)&on, sizeof(on));
#else
    int on = 1;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
#endif
}

TcpConnection::~TcpConnection() {
    LOG_DEBUG("TcpConnection destroyed");
    
    if (sockfd_ >= 0) {
#ifdef _WIN32
        closesocket(sockfd_);
#else
        ::close(sockfd_);
#endif
    }
}

void TcpConnection::establishConnection() {
    setState(kConnected);
    updateLastActiveTime();
    LOG_INFO("Connection established, fd: {}, peer: {}", sockfd_, getPeerAddress());
}

void TcpConnection::closeConnection() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        
        if (close_callback_) {
            close_callback_(shared_from_this());
        }
        
        setState(kDisconnected);
        LOG_INFO("Connection closed, fd: {}, peer: {}", sockfd_, getPeerAddress());
    }
}

ssize_t TcpConnection::send(const std::string& data) {
    return send(data.c_str(), data.size());
}

ssize_t TcpConnection::send(const char* data, size_t len) {
    if (state_ != kConnected) {
        LOG_WARN("Connection not connected, cannot send data, fd: {}", sockfd_);
        return -1;
    }
    
    updateLastActiveTime();
    
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool fault_error = false;
    
    // 如果输出缓冲区为空，尝试直接发送
    if (output_buffer_.empty()) {
        nwrote = ::write(sockfd_, data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0) {
                LOG_TRACE("Send data directly, fd: {}, bytes: {}", sockfd_, nwrote);
                return nwrote;
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_ERROR("Send data failed, fd: {}, errno: {}", sockfd_, errno);
                if (errno == EPIPE || errno == ECONNRESET) {
                    fault_error = true;
                }
            }
        }
    }
    
    assert(remaining <= len);
    
    if (!fault_error && remaining > 0) {
        // 将剩余数据加入输出缓冲区
        output_buffer_.append(data + nwrote, remaining);
        LOG_TRACE("Add to output buffer, fd: {}, bytes: {}, buffer_size: {}", 
                  sockfd_, remaining, output_buffer_.size());
    }
    
    return fault_error ? -1 : static_cast<ssize_t>(len);
}

void TcpConnection::handleRead() {
    updateLastActiveTime();
    
    char buffer[kBufferSize];
    ssize_t n = ::read(sockfd_, buffer, sizeof(buffer));
    
    if (n > 0) {
        input_buffer_.append(buffer, n);
        LOG_TRACE("Read data, fd: {}, bytes: {}, buffer_size: {}", 
                  sockfd_, n, input_buffer_.size());
        
        // 处理接收到的数据
        if (message_callback_) {
            message_callback_(shared_from_this(), input_buffer_);
            input_buffer_.clear(); // 简化处理，清空缓冲区
        }
    } else if (n == 0) {
        LOG_INFO("Connection closed by peer, fd: {}, peer: {}", 
                 sockfd_, getPeerAddress());
        closeConnection();
    } else {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            LOG_ERROR("Read data failed, fd: {}, errno: {}", sockfd_, errno);
            handleError();
        }
    }
}

void TcpConnection::handleWrite() {
    updateLastActiveTime();
    
    if (output_buffer_.empty()) {
        LOG_WARN("Output buffer is empty, why handleWrite called? fd: {}", sockfd_);
        return;
    }
    
    ssize_t n = ::write(sockfd_, output_buffer_.data(), output_buffer_.size());
    
    if (n > 0) {
        output_buffer_.erase(0, n);
        LOG_TRACE("Write data, fd: {}, bytes: {}, remaining: {}", 
                  sockfd_, n, output_buffer_.size());
        
        if (output_buffer_.empty()) {
            // 输出缓冲区已清空，可以禁用写事件
            LOG_TRACE("Output buffer cleared, fd: {}", sockfd_);
        }
    } else {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            LOG_ERROR("Write data failed, fd: {}, errno: {}", sockfd_, errno);
            handleError();
        }
    }
}

std::string TcpConnection::getPeerAddress() const {
    char buf[64];
    snprintf(buf, sizeof(buf), "%s:%d", 
             inet_ntoa(peer_addr_.sin_addr), ntohs(peer_addr_.sin_port));
    return std::string(buf);
}

void TcpConnection::updateLastActiveTime() {
    last_active_time_.store(time(nullptr));
}

bool TcpConnection::isTimeout(int timeout_seconds) const {
    time_t now = time(nullptr);
    time_t last_active = last_active_time_.load();
    return (now - last_active) > timeout_seconds;
}

void TcpConnection::handleError() {
    int err = 0;
    socklen_t optlen = sizeof(err);
    
    if (::getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &err, &optlen) < 0) {
        err = errno;
    }
    
    LOG_ERROR("Connection error, fd: {}, peer: {}, error: {}", 
              sockfd_, getPeerAddress(), err);
    
    closeConnection();
}

} // namespace network
} // namespace order_engine
