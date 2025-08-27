#pragma once

#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <ctime>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

namespace order_engine {
namespace network {

class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

/**
 * @brief TCP连接封装类
 * 
 * 管理单个TCP连接的生命周期，提供：
 * - 非阻塞数据收发
 * - 缓冲区管理
 * - 连接状态跟踪
 * - 心跳检测
 */
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    enum State {
        kConnecting,
        kConnected,
        kDisconnecting,
        kDisconnected
    };

    using MessageCallback = std::function<void(const TcpConnectionPtr&, const std::string&)>;
    using CloseCallback = std::function<void(const TcpConnectionPtr&)>;

    TcpConnection(int sockfd, const struct sockaddr_in& peer_addr);
    ~TcpConnection();

    // 连接管理
    void establishConnection();
    void closeConnection();
    
    // 数据收发
    ssize_t send(const std::string& data);
    ssize_t send(const char* data, size_t len);
    void handleRead();
    void handleWrite();
    
    // 状态查询
    bool isConnected() const { return state_ == kConnected; }
    State getState() const { return state_; }
    int getSocket() const { return sockfd_; }
    std::string getPeerAddress() const;
    
    // 回调设置
    void setMessageCallback(const MessageCallback& cb) { message_callback_ = cb; }
    void setCloseCallback(const CloseCallback& cb) { close_callback_ = cb; }
    
    // 心跳检测
    void updateLastActiveTime();
    bool isTimeout(int timeout_seconds) const;
    
    // Channel管理（简化处理）
    void setChannel(std::unique_ptr<class Channel> channel) { channel_ = std::move(channel); }

private:
    void setState(State state) { state_ = state; }
    void handleError();
    
    int sockfd_;
    struct sockaddr_in peer_addr_;
    State state_;
    
    // 缓冲区
    std::string input_buffer_;
    std::string output_buffer_;
    
    // 回调函数
    MessageCallback message_callback_;
    CloseCallback close_callback_;
    
    // 时间戳
    std::atomic<time_t> last_active_time_;
    
    // Channel管理（简化处理）
    std::unique_ptr<class Channel> channel_;
    
    static const size_t kBufferSize = 65536;
};

} // namespace network
} // namespace order_engine
