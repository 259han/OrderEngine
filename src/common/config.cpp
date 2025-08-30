#include "common/config.h"
#include "common/logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace order_engine {
namespace common {

bool Config::load(const std::string& config_file) {
    config_file_ = config_file;
    
    std::ifstream file(config_file);
    if (!file.is_open()) {
        LOG_ERROR_FMT("Failed to open config file: {}", config_file);
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return parseINI(buffer.str());
}

bool Config::reload() {
    if (config_file_.empty()) {
        LOG_ERROR("No config file specified for reload");
        return false;
    }
    
    return load(config_file_);
}

bool Config::parseINI(const std::string& content) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    configs_.clear();
    
    std::istringstream stream(content);
    std::string line;
    std::string section;
    
    while (std::getline(stream, line)) {
        // 去除首尾空格
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        // 跳过空行和注释
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // 处理节
        if (line[0] == '[' && line.back() == ']') {
            section = line.substr(1, line.length() - 2);
            continue;
        }
        
        // 处理键值对
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除键值的空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // 组合完整键名
            std::string full_key = section.empty() ? key : section + "." + key;
            configs_[full_key] = value;
        }
    }
    
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "Loaded %zu config items from: %s", configs_.size(), config_file_.c_str());
    LOG_INFO(buffer);
    return true;
}

std::string Config::getString(const std::string& key, const std::string& default_value) const {
    std::string value = getValue(key);
    return value.empty() ? default_value : value;
}

int Config::getInt(const std::string& key, int default_value) const {
    std::string value = getValue(key);
    if (value.empty()) {
        return default_value;
    }
    
    try {
        return std::stoi(value);
    } catch (const std::exception& e) {
        char buffer[200];
        snprintf(buffer, sizeof(buffer), "Failed to parse int config: %s = %s, using default: %d", 
                 key.c_str(), value.c_str(), default_value);
        LOG_WARN(buffer);
        return default_value;
    }
}

double Config::getDouble(const std::string& key, double default_value) const {
    std::string value = getValue(key);
    if (value.empty()) {
        return default_value;
    }
    
    try {
        return std::stod(value);
    } catch (const std::exception& e) {
        char buffer[200];
        snprintf(buffer, sizeof(buffer), "Failed to parse double config: %s = %s, using default: %f", 
                 key.c_str(), value.c_str(), default_value);
        LOG_WARN(buffer);
        return default_value;
    }
}

bool Config::getBool(const std::string& key, bool default_value) const {
    std::string value = getValue(key);
    if (value.empty()) {
        return default_value;
    }
    
    std::transform(value.begin(), value.end(), value.begin(), ::tolower);
    
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return true;
    } else if (value == "false" || value == "0" || value == "no" || value == "off") {
        return false;
    } else {
        char buffer[200];
        snprintf(buffer, sizeof(buffer), "Failed to parse bool config: %s = %s, using default: %s", 
                 key.c_str(), value.c_str(), default_value ? "true" : "false");
        LOG_WARN(buffer);
        return default_value;
    }
}

void Config::setString(const std::string& key, const std::string& value) {
    setValue(key, value);
}

void Config::setInt(const std::string& key, int value) {
    setValue(key, std::to_string(value));
}

void Config::setDouble(const std::string& key, double value) {
    setValue(key, std::to_string(value));
}

void Config::setBool(const std::string& key, bool value) {
    setValue(key, value ? "true" : "false");
}

bool Config::hasKey(const std::string& key) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return configs_.find(key) != configs_.end();
}

std::unordered_map<std::string, std::string> Config::getAllConfigs() const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    return configs_;
}

std::string Config::getValue(const std::string& key) const {
    std::lock_guard<std::mutex> lock(config_mutex_);
    auto it = configs_.find(key);
    return (it != configs_.end()) ? it->second : "";
}

void Config::setValue(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(config_mutex_);
    configs_[key] = value;
}

} // namespace common
} // namespace order_engine
