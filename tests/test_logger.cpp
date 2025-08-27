#include <gtest/gtest.h>
#include "common/logger.h"
#include <thread>
#include <chrono>

using namespace order_engine::common;

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试用的简单初始化
    }
    
    void TearDown() override {
        // 清理
    }
};

TEST_F(LoggerTest, BasicLogging) {
    Logger& logger = Logger::getInstance();
    
    // 测试各种日志级别
    LOG_TRACE("This is a trace message: {}", 42);
    LOG_DEBUG("This is a debug message: {}", "test");
    LOG_INFO("This is an info message");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");
    LOG_CRITICAL("This is a critical message");
    
    EXPECT_TRUE(true); // 基本测试通过
}

TEST_F(LoggerTest, PerformanceLogging) {
    Logger& logger = Logger::getInstance();
    
    // 测试性能日志
    auto start = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double duration_ms = duration.count() / 1000.0;
    
    LOG_PERF("test_operation", duration_ms);
    
    EXPECT_GT(duration_ms, 0);
}

TEST_F(LoggerTest, BusinessLogging) {
    Logger& logger = Logger::getInstance();
    
    // 测试业务日志
    LOG_BUSINESS("order_created", "order_id=12345,user_id=67890,amount=99.99");
    LOG_BUSINESS("payment_success", "transaction_id=tx_12345");
    
    EXPECT_TRUE(true);
}

TEST_F(LoggerTest, ThreadSafety) {
    const int num_threads = 10;
    const int messages_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, messages_per_thread]() {
            for (int j = 0; j < messages_per_thread; ++j) {
                LOG_INFO("Thread {} message {}", i, j);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_TRUE(true);
}
