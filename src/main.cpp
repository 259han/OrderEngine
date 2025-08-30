#include <iostream>
#include <signal.h>
#include <memory>
#include "common/logger.h"
#include "common/config.h"
#include "network/tcp_server.h"
#include "services/order_service.h"
#include "services/inventory_service.h"
// #include "database/connection_pool.h"  // TODO: 待实现
// #include "cache/cache_manager.h"       // TODO: 待实现  
// #include "message/kafka_producer.h"    // TODO: 待实现

using namespace order_engine;

class OrderEngineApplication {
public:
    OrderEngineApplication() : running_(true) {}
    
    bool initialize(const std::string& config_file) {
        // 初始化日志系统
        if (!common::Logger::getInstance().initialize(config_file)) {
            std::cerr << "Failed to initialize logger" << std::endl;
            return false;
        }
        
        LOG_INFO("Starting OrderEngine Application...");
        
        // 初始化配置
        config_ = std::make_shared<common::Config>();
        if (!config_->load(config_file)) {
            LOG_ERROR_FMT("Failed to load config file: {}", config_file);
            return false;
        }
        
        // TODO: 初始化数据库连接池 (暂时跳过)
        LOG_INFO("Database connection pool initialization skipped in Phase 1");
        
        // TODO: 初始化缓存管理器 (暂时跳过)
        LOG_INFO("Cache manager initialization skipped in Phase 1");
        
        // TODO: 初始化Kafka生产者 (暂时跳过)
        LOG_INFO("Kafka producer initialization skipped in Phase 1");
        
        // TODO: 初始化业务服务 (暂时跳过)
        LOG_INFO("Business services initialization skipped in Phase 1");
        
        // 初始化TCP服务器
        std::string server_ip = config_->getString("server.ip", "0.0.0.0");
        int server_port = config_->getInt("server.port", 8080);
        int thread_num = config_->getInt("server.thread_num", 4);
        
        tcp_server_ = std::make_shared<network::TcpServer>(server_ip, server_port, thread_num);
        
        // 设置消息处理回调
        tcp_server_->setMessageCallback([this](const network::TcpConnectionPtr& conn, const std::string& message) {
            this->handleMessage(conn, message);
        });
        
        tcp_server_->setConnectionCallback([this](const network::TcpConnectionPtr& conn) {
            this->handleConnection(conn);
        });
        
        LOG_INFO("OrderEngine Application initialized successfully");
        return true;
    }
    
    void run() {
        // 注册信号处理
        signal(SIGINT, [](int) { 
            std::cout << "\nReceived SIGINT, shutting down..." << std::endl;
        });
        
        signal(SIGTERM, [](int) { 
            std::cout << "\nReceived SIGTERM, shutting down..." << std::endl;
        });
        
        // 启动TCP服务器
        if (!tcp_server_->start()) {
            LOG_ERROR("Failed to start TCP server");
            return;
        }
        
        LOG_INFO("OrderEngine Application started successfully");
        
        // 主循环
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // 打印统计信息
            if (++stats_counter_ % 60 == 0) {  // 每分钟打印一次
                printStats();
            }
        }
        
        LOG_INFO("OrderEngine Application shutting down...");
        shutdown();
    }
    
    void stop() {
        running_ = false;
    }

private:
    void handleMessage(const network::TcpConnectionPtr& conn, const std::string& message) {
        LOG_DEBUG_FMT2("Received message from {}: {}", conn->getPeerAddress(), message);
        
        // 这里应该解析协议消息并路由到相应的服务
        // 简化示例：直接回显
        conn->send("Echo: " + message);
    }
    
    void handleConnection(const network::TcpConnectionPtr& conn) {
        if (conn->isConnected()) {
            LOG_INFO_FMT("New connection from: {}", conn->getPeerAddress());
        } else {
            LOG_INFO_FMT("Connection closed: {}", conn->getPeerAddress());
        }
    }
    
    void printStats() {
        LOG_INFO("=== OrderEngine Statistics ===");
        LOG_INFO_FMT_INT("Active connections: {}", tcp_server_->getConnectionCount());
        // TODO: 添加业务统计 (Phase 2)
        LOG_INFO("==============================");
    }
    
    void shutdown() {
        if (tcp_server_) {
            tcp_server_->stop();
        }
        
        // TODO: 关闭业务服务 (Phase 2)
        
        common::Logger::getInstance().shutdown();
    }
    
    std::atomic<bool> running_;
    int stats_counter_ = 0;
    
    // 核心组件
    std::shared_ptr<common::Config> config_;
    std::shared_ptr<network::TcpServer> tcp_server_;
    // TODO: 添加数据库、缓存、消息队列组件 (Phase 2)
    // TODO: 添加业务服务组件 (Phase 2)
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }
    
    try {
        OrderEngineApplication app;
        
        if (!app.initialize(argv[1])) {
            std::cerr << "Failed to initialize application" << std::endl;
            return 1;
        }
        
        app.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
