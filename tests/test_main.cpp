#include <gtest/gtest.h>
#include "common/logger.h"

int main(int argc, char** argv) {
    // 初始化日志系统
    order_engine::common::Logger::getInstance().initialize("config/server.conf");
    
    // 初始化测试框架
    ::testing::InitGoogleTest(&argc, argv);
    
    // 运行测试
    int result = RUN_ALL_TESTS();
    
    // 关闭日志系统
    order_engine::common::Logger::getInstance().shutdown();
    
    return result;
}
