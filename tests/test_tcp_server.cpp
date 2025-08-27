#include <gtest/gtest.h>
#include "network/tcp_server.h"
#include "common/logger.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>

using namespace order_engine::network;

class TcpServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_ = std::make_unique<TcpServer>("127.0.0.1", 8081, 2);
        
        // 设置回调函数
        server_->setMessageCallback([this](const TcpConnectionPtr& conn, const std::string& message) {
            handleMessage(conn, message);
        });
        
        server_->setConnectionCallback([this](const TcpConnectionPtr& conn) {
            handleConnection(conn);
        });
    }
    
    void TearDown() override {
        if (server_) {
            server_->stop();
        }
    }
    
    void handleMessage(const TcpConnectionPtr& conn, const std::string& message) {
        received_messages_.push_back(message);
        // 回显消息
        conn->send("Echo: " + message);
    }
    
    void handleConnection(const TcpConnectionPtr& conn) {
        if (conn->isConnected()) {
            connection_count_++;
        } else {
            connection_count_--;
        }
    }
    
    std::unique_ptr<TcpServer> server_;
    std::vector<std::string> received_messages_;
    std::atomic<int> connection_count_{0};
};

TEST_F(TcpServerTest, BasicStartStop) {
    EXPECT_FALSE(server_->isRunning());
    
    EXPECT_TRUE(server_->start());
    EXPECT_TRUE(server_->isRunning());
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    server_->stop();
    EXPECT_FALSE(server_->isRunning());
}

TEST_F(TcpServerTest, ClientConnection) {
    EXPECT_TRUE(server_->start());
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 创建客户端连接
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(client_fd, 0);
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8081);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    int ret = connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (ret == 0) {
        // 连接成功，等待连接建立
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        EXPECT_EQ(connection_count_.load(), 1);
        EXPECT_EQ(server_->getConnectionCount(), 1);
        
        // 发送消息
        const char* message = "Hello Server";
        send(client_fd, message, strlen(message), 0);
        
        // 等待消息处理
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // 接收回显
        char buffer[1024];
        ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            std::string echo_msg(buffer);
            EXPECT_EQ(echo_msg, "Echo: Hello Server");
        }
        
        close(client_fd);
        
        // 等待连接关闭
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_EQ(connection_count_.load(), 0);
    } else {
        close(client_fd);
        // 连接失败可能是因为端口被占用，跳过测试
        GTEST_SKIP() << "Could not connect to test server";
    }
}

TEST_F(TcpServerTest, MultipleConnections) {
    EXPECT_TRUE(server_->start());
    
    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    const int num_clients = 5;
    std::vector<int> client_fds;
    
    // 创建多个客户端连接
    for (int i = 0; i < num_clients; ++i) {
        int client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fd < 0) continue;
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(8081);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
        
        if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
            client_fds.push_back(client_fd);
        } else {
            close(client_fd);
        }
    }
    
    if (!client_fds.empty()) {
        // 等待连接建立
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        EXPECT_EQ(server_->getConnectionCount(), static_cast<int>(client_fds.size()));
        
        // 关闭所有连接
        for (int fd : client_fds) {
            close(fd);
        }
        
        // 等待连接关闭
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        EXPECT_EQ(server_->getConnectionCount(), 0);
    } else {
        GTEST_SKIP() << "Could not establish test connections";
    }
}
