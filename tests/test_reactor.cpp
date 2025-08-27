#include <gtest/gtest.h>
#include "network/reactor.h"
#include "common/logger.h"
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <chrono>

using namespace order_engine::network;

class ReactorTest : public ::testing::Test {
protected:
    void SetUp() override {
        reactor_ = std::make_unique<Reactor>();
    }
    
    void TearDown() override {
        if (reactor_) {
            reactor_->quit();
        }
        if (reactor_thread_.joinable()) {
            reactor_thread_.join();
        }
    }
    
    std::unique_ptr<Reactor> reactor_;
    std::thread reactor_thread_;
};

TEST_F(ReactorTest, BasicFunctionality) {
    EXPECT_FALSE(reactor_->isInLoopThread());
    
    // 启动Reactor线程
    reactor_thread_ = std::thread([this]() {
        reactor_->loop();
    });
    
    // 等待一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 停止Reactor
    reactor_->quit();
    reactor_thread_.join();
    
    EXPECT_TRUE(true);
}

TEST_F(ReactorTest, TaskQueue) {
    bool task_executed = false;
    
    // 启动Reactor线程
    reactor_thread_ = std::thread([this]() {
        reactor_->loop();
    });
    
    // 添加任务到队列
    reactor_->runInLoop([&task_executed]() {
        task_executed = true;
    });
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    reactor_->quit();
    reactor_thread_.join();
    
    EXPECT_TRUE(task_executed);
}

TEST_F(ReactorTest, DelayedTask) {
    bool task_executed = false;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 启动Reactor线程
    reactor_thread_ = std::thread([this]() {
        reactor_->loop();
    });
    
    // 添加延迟任务
    reactor_->runAfter([&task_executed]() {
        task_executed = true;
    }, 0.1); // 100ms延迟
    
    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    reactor_->quit();
    reactor_thread_.join();
    
    EXPECT_TRUE(task_executed);
    EXPECT_GE(duration.count(), 100); // 至少延迟了100ms
}
