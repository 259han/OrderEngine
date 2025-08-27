#include "network/tcp_server.h"
#include "common/logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cassert>

namespace order_engine {
namespace network {

TcpServer::TcpServer(const std::string& ip, uint16_t port, int thread_num)
    : ip_(ip)
    , port_(port)
    , listen_fd_(-1)
    , thread_num_(thread_num <= 0 ? std::thread::hardware_concurrency() : thread_num)
    , running_(false)
    , next_reactor_(0) {
    
    LOG_INFO("TcpServer created, bind: {}:{}, threads: {}", ip_, port_, thread_num_);
}

TcpServer::~TcpServer() {
    stop();
    LOG_INFO("TcpServer destroyed");
}

bool TcpServer::start() {
    if (running_.load()) {
        LOG_WARN("TcpServer already running");
        return true;
    }
    
    // 创建监听socket
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (listen_fd_ < 0) {
        LOG_ERROR("Create listen socket failed, errno: {}", errno);
        return false;
    }
    
    // 设置socket选项
    int on = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
    
    // 绑定地址
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    
    if (ip_ == "0.0.0.0") {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (::inet_pton(AF_INET, ip_.c_str(), &server_addr.sin_addr) <= 0) {
            LOG_ERROR("Invalid IP address: {}", ip_);
            ::close(listen_fd_);
            return false;
        }
    }
    
    if (::bind(listen_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Bind address {}:{} failed, errno: {}", ip_, port_, errno);
        ::close(listen_fd_);
        return false;
    }
    
    // 开始监听
    if (::listen(listen_fd_, SOMAXCONN) < 0) {
        LOG_ERROR("Listen failed, errno: {}", errno);
        ::close(listen_fd_);
        return false;
    }
    
    LOG_INFO("Listen on {}:{}, fd: {}", ip_, port_, listen_fd_);
    
    // 创建主Reactor
    main_reactor_ = std::make_unique<Reactor>();
    
    // 创建从Reactor线程池
    sub_reactors_.reserve(thread_num_);
    reactor_threads_.reserve(thread_num_);
    
    for (int i = 0; i < thread_num_; ++i) {
        auto reactor = std::make_unique<Reactor>();
        auto* reactor_ptr = reactor.get();
        sub_reactors_.push_back(std::move(reactor));
        
        reactor_threads_.emplace_back([reactor_ptr]() {
            LOG_DEBUG("Sub reactor thread started");
            reactor_ptr->loop();
            LOG_DEBUG("Sub reactor thread stopped");
        });
    }
    
    // 在主Reactor中处理新连接
    accept_channel_ = std::make_unique<Channel>(main_reactor_.get(), listen_fd_);
    accept_channel_->setReadCallback([this]() { acceptConnection(); });
    accept_channel_->enableReading();
    
    running_.store(true);
    
    // 启动主Reactor线程
    main_thread_ = std::thread([this]() {
        LOG_DEBUG("Main reactor thread started");
        main_reactor_->loop();
        LOG_DEBUG("Main reactor thread stopped");
    });
    
    LOG_INFO("TcpServer started successfully");
    return true;
}

void TcpServer::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    LOG_INFO("TcpServer stopping...");
    
    // 停止接受新连接
    if (accept_channel_) {
        accept_channel_->disableAll();
        accept_channel_->remove();
        accept_channel_.reset();
    }
    
    // 关闭监听socket
    if (listen_fd_ >= 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
    
    // 停止主Reactor
    if (main_reactor_) {
        main_reactor_->quit();
    }
    
    if (main_thread_.joinable()) {
        main_thread_.join();
    }
    
    // 停止从Reactor
    for (auto& reactor : sub_reactors_) {
        reactor->quit();
    }
    
    for (auto& thread : reactor_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // 关闭所有连接
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        for (auto& pair : connections_) {
            pair.second->closeConnection();
        }
        connections_.clear();
    }
    
    LOG_INFO("TcpServer stopped");
}

void TcpServer::acceptConnection() {
    struct sockaddr_in peer_addr;
    socklen_t addrlen = sizeof(peer_addr);
    
    while (true) {
        std::memset(&peer_addr, 0, sizeof(peer_addr));
        int connfd = ::accept4(listen_fd_, (struct sockaddr*)&peer_addr, 
                              &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
        
        if (connfd >= 0) {
            handleNewConnection(connfd, peer_addr);
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 没有更多连接
            } else if (errno == EINTR) {
                continue; // 被信号中断，继续
            } else {
                LOG_ERROR("Accept connection failed, errno: {}", errno);
                break;
            }
        }
    }
}

void TcpServer::handleNewConnection(int connfd, const struct sockaddr_in& peer_addr) {
    // 选择一个从Reactor（轮询方式）
    int reactor_index = next_reactor_.fetch_add(1) % thread_num_;
    Reactor* reactor = sub_reactors_[reactor_index].get();
    
    // 创建TcpConnection
    auto conn = std::make_shared<TcpConnection>(connfd, peer_addr);
    
    // 设置回调函数
    conn->setMessageCallback(message_callback_);
    conn->setCloseCallback([this](const TcpConnectionPtr& conn) {
        removeConnection(conn);
        if (connection_callback_) {
            connection_callback_(conn);
        }
    });
    
    // 在选定的Reactor中管理连接
    reactor->runInLoop([this, conn, reactor]() {
        // 创建Channel并注册到Reactor
        auto channel = std::make_unique<Channel>(reactor, conn->getSocket());
        
        channel->setReadCallback([conn]() { conn->handleRead(); });
        channel->setWriteCallback([conn]() { conn->handleWrite(); });
        channel->setCloseCallback([conn]() { conn->closeConnection(); });
        channel->setErrorCallback([conn]() { conn->closeConnection(); });
        
        channel->enableReading();
        
        // 保存Channel到连接中（这里简化处理）
        conn->setChannel(std::move(channel));
        
        // 建立连接
        conn->establishConnection();
        
        // 通知连接建立
        if (connection_callback_) {
            connection_callback_(conn);
        }
    });
    
    // 添加到连接管理
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_[connfd] = conn;
    }
    
    LOG_DEBUG("New connection accepted, fd: {}, peer: {}, total: {}", 
              connfd, conn->getPeerAddress(), getConnectionCount());
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
    int fd = conn->getSocket();
    
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.erase(fd);
    }
    
    LOG_DEBUG("Connection removed, fd: {}, peer: {}, total: {}", 
              fd, conn->getPeerAddress(), getConnectionCount());
}

int TcpServer::getConnectionCount() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return static_cast<int>(connections_.size());
}

void TcpServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    LOG_DEBUG("Broadcasting message to {} connections", connections_.size());
    
    for (auto& pair : connections_) {
        auto& conn = pair.second;
        if (conn->isConnected()) {
            conn->send(message);
        }
    }
}

} // namespace network
} // namespace order_engine
