#pragma once

#include <memory>
#include <functional>
#include <atomic>
#include <thread>
#include <vector>
#include "tcp_connection.h"
#include "reactor.h"

namespace order_engine {
namespace network {

/**
 * @brief 高性能TCP服务器
 * 
 * 基于Reactor模式实现，支持：
 * - 主从Reactor架构
 * - 多线程事件处理
 * - 非阻塞I/O
 * - 连接池管理
 * - 负载均衡
 */
class TcpServer {
public:
    using MessageCallback = std::function<void(const TcpConnectionPtr&, const std::string&)>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;

    TcpServer(const std::string& ip, uint16_t port, int thread_num = 0);
    ~TcpServer();

    // 启动和停止服务器
    bool start();
    void stop();

    // 设置回调函数
    void setMessageCallback(const MessageCallback& cb) { message_callback_ = cb; }
    void setConnectionCallback(const ConnectionCallback& cb) { connection_callback_ = cb; }

    // 服务器状态
    bool isRunning() const { return running_.load(); }
    int getConnectionCount() const;

    // 广播消息
    void broadcast(const std::string& message);

private:
    void acceptConnection();
    void handleNewConnection(int connfd, const struct sockaddr_in& peer_addr);
    
    std::string ip_;
    uint16_t port_;
    int listen_fd_;
    
    // Reactor线程池
    std::unique_ptr<Reactor> main_reactor_;
    std::vector<std::unique_ptr<Reactor>> sub_reactors_;
    std::vector<std::thread> reactor_threads_;
    
    int thread_num_;
    std::atomic<bool> running_;
    std::atomic<int> next_reactor_;
    
    // 回调函数
    MessageCallback message_callback_;
    ConnectionCallback connection_callback_;
    
    // 连接管理
    std::unordered_map<int, TcpConnectionPtr> connections_;
    mutable std::mutex connections_mutex_;
    
    // 接受连接的Channel
    std::unique_ptr<class Channel> accept_channel_;
    std::thread main_thread_;
    
    void removeConnection(const TcpConnectionPtr& conn);
};

} // namespace network
} // namespace order_engine
