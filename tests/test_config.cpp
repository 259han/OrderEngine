#include <gtest/gtest.h>
#include "common/config.h"
#include <fstream>
#include <cstdio>

using namespace order_engine::common;

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试配置文件
        test_config_file_ = "test_config.ini";
        createTestConfigFile();
        config_ = std::make_unique<Config>();
    }
    
    void TearDown() override {
        // 删除测试配置文件
        std::remove(test_config_file_.c_str());
    }
    
    void createTestConfigFile() {
        std::ofstream file(test_config_file_);
        file << "# Test configuration file\n";
        file << "\n";
        file << "[server]\n";
        file << "ip = 127.0.0.1\n";
        file << "port = 8080\n";
        file << "thread_num = 4\n";
        file << "debug = true\n";
        file << "timeout = 30.5\n";
        file << "\n";
        file << "[database]\n";
        file << "host = localhost\n";
        file << "port = 3306\n";
        file << "max_connections = 50\n";
        file << "auto_reconnect = false\n";
        file << "\n";
        file << "# Global settings\n";
        file << "app_name = OrderEngine\n";
        file << "version = 1.0.0\n";
        file.close();
    }
    
    std::string test_config_file_;
    std::unique_ptr<Config> config_;
};

TEST_F(ConfigTest, LoadConfig) {
    EXPECT_TRUE(config_->load(test_config_file_));
}

TEST_F(ConfigTest, StringValues) {
    ASSERT_TRUE(config_->load(test_config_file_));
    
    EXPECT_EQ(config_->getString("server.ip"), "127.0.0.1");
    EXPECT_EQ(config_->getString("database.host"), "localhost");
    EXPECT_EQ(config_->getString("app_name"), "OrderEngine");
    EXPECT_EQ(config_->getString("version"), "1.0.0");
    
    // 测试默认值
    EXPECT_EQ(config_->getString("nonexistent", "default"), "default");
}

TEST_F(ConfigTest, IntValues) {
    ASSERT_TRUE(config_->load(test_config_file_));
    
    EXPECT_EQ(config_->getInt("server.port"), 8080);
    EXPECT_EQ(config_->getInt("server.thread_num"), 4);
    EXPECT_EQ(config_->getInt("database.port"), 3306);
    EXPECT_EQ(config_->getInt("database.max_connections"), 50);
    
    // 测试默认值
    EXPECT_EQ(config_->getInt("nonexistent", 42), 42);
}

TEST_F(ConfigTest, DoubleValues) {
    ASSERT_TRUE(config_->load(test_config_file_));
    
    EXPECT_DOUBLE_EQ(config_->getDouble("server.timeout"), 30.5);
    
    // 测试默认值
    EXPECT_DOUBLE_EQ(config_->getDouble("nonexistent", 3.14), 3.14);
}

TEST_F(ConfigTest, BoolValues) {
    ASSERT_TRUE(config_->load(test_config_file_));
    
    EXPECT_TRUE(config_->getBool("server.debug"));
    EXPECT_FALSE(config_->getBool("database.auto_reconnect"));
    
    // 测试默认值
    EXPECT_TRUE(config_->getBool("nonexistent", true));
    EXPECT_FALSE(config_->getBool("nonexistent", false));
}

TEST_F(ConfigTest, HasKey) {
    ASSERT_TRUE(config_->load(test_config_file_));
    
    EXPECT_TRUE(config_->hasKey("server.ip"));
    EXPECT_TRUE(config_->hasKey("database.host"));
    EXPECT_TRUE(config_->hasKey("app_name"));
    EXPECT_FALSE(config_->hasKey("nonexistent"));
}

TEST_F(ConfigTest, SetValues) {
    ASSERT_TRUE(config_->load(test_config_file_));
    
    // 设置新值
    config_->setString("new_string", "test_value");
    config_->setInt("new_int", 123);
    config_->setDouble("new_double", 45.67);
    config_->setBool("new_bool", true);
    
    // 验证设置的值
    EXPECT_EQ(config_->getString("new_string"), "test_value");
    EXPECT_EQ(config_->getInt("new_int"), 123);
    EXPECT_DOUBLE_EQ(config_->getDouble("new_double"), 45.67);
    EXPECT_TRUE(config_->getBool("new_bool"));
}

TEST_F(ConfigTest, GetAllConfigs) {
    ASSERT_TRUE(config_->load(test_config_file_));
    
    auto all_configs = config_->getAllConfigs();
    
    EXPECT_GT(all_configs.size(), 0);
    EXPECT_NE(all_configs.find("server.ip"), all_configs.end());
    EXPECT_NE(all_configs.find("database.host"), all_configs.end());
}
