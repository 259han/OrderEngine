#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace order_engine {
namespace common {

/**
 * @brief 配置管理器
 * 
 * 支持多种配置格式和动态配置更新
 * 线程安全的配置读取和更新
 */
class Config {
public:
    Config() = default;
    ~Config() = default;

    // 加载配置文件
    bool load(const std::string& config_file);
    
    // 重新加载配置
    bool reload();
    
    // 获取配置值
    std::string getString(const std::string& key, const std::string& default_value = "") const;
    int getInt(const std::string& key, int default_value = 0) const;
    double getDouble(const std::string& key, double default_value = 0.0) const;
    bool getBool(const std::string& key, bool default_value = false) const;
    
    // 设置配置值
    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setDouble(const std::string& key, double value);
    void setBool(const std::string& key, bool value);
    
    // 检查配置项是否存在
    bool hasKey(const std::string& key) const;
    
    // 获取所有配置
    std::unordered_map<std::string, std::string> getAllConfigs() const;

private:
    bool parseINI(const std::string& content);
    std::string getValue(const std::string& key) const;
    void setValue(const std::string& key, const std::string& value);
    
    mutable std::mutex config_mutex_;
    std::unordered_map<std::string, std::string> configs_;
    std::string config_file_;
};

} // namespace common
} // namespace order_engine
