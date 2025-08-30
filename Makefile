# OrderEngine Makefile
# 高性能C++电商订单系统构建脚本

# 编译器和编译选项
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -g -fPIC -march=native
LDFLAGS = -pthread

# Debug 和 Release 配置
DEBUG_FLAGS = -O0 -g3 -DDEBUG -fsanitize=address
RELEASE_FLAGS = -O3 -DNDEBUG -DORDER_ENGINE_RELEASE

# 目录配置
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin
LIB_DIR = lib
TEST_DIR = tests
CONFIG_DIR = config
SCRIPT_DIR = scripts

# 第三方库 (Phase 1: 仅包含必需的库)
SPDLOG_LIBS = -lspdlog
GTEST_LIBS = -lgtest -lgtest_main

# Phase 1 只需要的库
ALL_LIBS = $(SPDLOG_LIBS)

# TODO: Phase 2 添加其他库
# BOOST_LIBS = -lboost_system -lboost_filesystem -lboost_thread -lboost_log
# MYSQL_LIBS = -lmysqlclient
# REDIS_LIBS = -lhiredis
# KAFKA_LIBS = -lrdkafka
# PROTOBUF_LIBS = -lprotobuf
# GRPC_LIBS = -lgrpc++
# JSON_LIBS = -ljsoncpp

# 包含路径
INCLUDES = -I$(INCLUDE_DIR) -I/usr/include/mysql -I/usr/local/include

# 源文件 (Phase 1: 仅包含已实现的模块)
CORE_SOURCES = $(wildcard $(SRC_DIR)/common/*.cpp) \
               $(wildcard $(SRC_DIR)/network/*.cpp)

# TODO: Phase 2 添加其他模块
# $(wildcard $(SRC_DIR)/database/*.cpp) \
# $(wildcard $(SRC_DIR)/cache/*.cpp) \
# $(wildcard $(SRC_DIR)/message/*.cpp) \
# $(wildcard $(SRC_DIR)/services/*.cpp) \
# $(wildcard $(SRC_DIR)/protocol/*.cpp) \
# $(wildcard $(SRC_DIR)/utils/*.cpp)

MAIN_SOURCE = $(SRC_DIR)/main.cpp
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.cpp)

# 目标文件
CORE_OBJECTS = $(CORE_SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
MAIN_OBJECT = $(MAIN_SOURCE:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.cpp=$(BUILD_DIR)/test_%.o)

# 目标程序
TARGET = $(BIN_DIR)/order_engine
CORE_LIB = $(LIB_DIR)/liborder_engine_core.a
TEST_TARGET = $(BIN_DIR)/order_engine_test

.PHONY: all clean debug release test install uninstall deps check format docs

# 默认目标
all: $(TARGET)

# 创建目录
$(BUILD_DIR) $(BIN_DIR) $(LIB_DIR):
	@mkdir -p "$@"

# 创建子目录 (Phase 1)
$(BUILD_DIR)/common $(BUILD_DIR)/network: | $(BUILD_DIR)
	@mkdir -p "$@"

# TODO: Phase 2 添加其他目录
# $(BUILD_DIR)/database $(BUILD_DIR)/cache $(BUILD_DIR)/message \
# $(BUILD_DIR)/services $(BUILD_DIR)/protocol $(BUILD_DIR)/utils

# 编译核心库
$(CORE_LIB): $(CORE_OBJECTS) | $(LIB_DIR)
	@echo "Creating static library: $@"
	@ar rcs $@ $^

# 编译目标文件 (Phase 1)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)/common $(BUILD_DIR)/network
	@echo "Compiling: $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 编译测试目标文件
$(BUILD_DIR)/test_%.o: $(TEST_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Compiling test: $<"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 链接主程序
$(TARGET): $(MAIN_OBJECT) $(CORE_LIB) | $(BIN_DIR)
	@echo "Linking: $@"
	@$(CXX) $(LDFLAGS) $< -L$(LIB_DIR) -lorder_engine_core $(ALL_LIBS) -o $@

# 编译测试程序
$(TEST_TARGET): $(TEST_OBJECTS) $(CORE_LIB) | $(BIN_DIR)
	@echo "Linking test: $@"
	@$(CXX) $(LDFLAGS) $^ $(ALL_LIBS) $(GTEST_LIBS) -o $@

# Debug 版本
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

# Release 版本
release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

# 运行测试
test: $(TEST_TARGET)
	@echo "Running tests..."
	@./$(TEST_TARGET)

# 性能测试
perf-test: release
	@echo "Running performance tests..."
	@./$(BIN_DIR)/order_engine $(CONFIG_DIR)/perf_test.conf &
	@sleep 2
	@$(SCRIPT_DIR)/benchmark.sh

# 代码检查
check:
	@echo "Running static analysis..."
	@cppcheck --enable=all --std=c++17 $(SRC_DIR) $(INCLUDE_DIR)
	@clang-tidy $(CORE_SOURCES) $(MAIN_SOURCE) -- $(INCLUDES)

# 代码格式化
format:
	@echo "Formatting code..."
	@find $(SRC_DIR) $(INCLUDE_DIR) $(TEST_DIR) -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# 生成文档
docs:
	@echo "Generating documentation..."
	@doxygen Doxyfile

# 安装
install: release
	@echo "Installing OrderEngine..."
	@sudo cp $(TARGET) /usr/local/bin/
	@sudo mkdir -p /etc/order_engine
	@sudo cp $(CONFIG_DIR)/* /etc/order_engine/
	@sudo mkdir -p /var/log/order_engine
	@sudo chmod 755 /usr/local/bin/order_engine

# 卸载
uninstall:
	@echo "Uninstalling OrderEngine..."
	@sudo rm -f /usr/local/bin/order_engine
	@sudo rm -rf /etc/order_engine
	@sudo rm -rf /var/log/order_engine

# 检查依赖
deps:
	@echo "Checking dependencies..."
	@pkg-config --exists mysqlclient || echo "MySQL client library not found"
	@pkg-config --exists hiredis || echo "Redis client library not found"
	@pkg-config --exists librdkafka || echo "Kafka client library not found"
	@pkg-config --exists protobuf || echo "Protocol Buffers library not found"
	@ldconfig -p | grep boost || echo "Boost libraries not found"

# 安装依赖 (Ubuntu/Debian)
install-deps-ubuntu:
	@echo "Installing dependencies for Ubuntu/Debian..."
	@sudo apt-get update
	@sudo apt-get install -y build-essential cmake git
	@sudo apt-get install -y libboost-all-dev
	@sudo apt-get install -y libmysqlclient-dev
	@sudo apt-get install -y libhiredis-dev
	@sudo apt-get install -y librdkafka-dev
	@sudo apt-get install -y libprotobuf-dev protobuf-compiler
	@sudo apt-get install -y libgrpc++-dev
	@sudo apt-get install -y libspdlog-dev
	@sudo apt-get install -y libjsoncpp-dev
	@sudo apt-get install -y libgtest-dev
	@sudo apt-get install -y cppcheck clang-tidy clang-format
	@sudo apt-get install -y doxygen graphviz

# 安装依赖 (CentOS/RHEL)
install-deps-centos:
	@echo "Installing dependencies for CentOS/RHEL..."
	@sudo yum groupinstall -y "Development Tools"
	@sudo yum install -y cmake3 git
	@sudo yum install -y boost-devel
	@sudo yum install -y mysql-devel
	@sudo yum install -y hiredis-devel
	@sudo yum install -y librdkafka-devel
	@sudo yum install -y protobuf-devel protobuf-compiler
	@sudo yum install -y grpc-devel
	@sudo yum install -y spdlog-devel
	@sudo yum install -y jsoncpp-devel

# Docker 构建
docker-build:
	@echo "Building Docker image..."
	@docker build -t order_engine:latest .

# Docker 运行
docker-run:
	@echo "Running Docker container..."
	@docker run -d --name order_engine \
		-p 8080:8080 \
		-v $(PWD)/config:/etc/order_engine \
		-v $(PWD)/logs:/var/log/order_engine \
		order_engine:latest

# 清理
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR) $(LIB_DIR)
	@rm -f core.*

# 深度清理
distclean: clean
	@rm -rf logs/*
	@rm -f *.log

# 帮助信息
help:
	@echo "OrderEngine Build System"
	@echo "========================"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build the main application (default)"
	@echo "  debug        - Build debug version with AddressSanitizer"
	@echo "  release      - Build optimized release version"
	@echo "  test         - Build and run unit tests"
	@echo "  perf-test    - Run performance tests"
	@echo "  check        - Run static analysis tools"
	@echo "  format       - Format code using clang-format"
	@echo "  docs         - Generate documentation"
	@echo "  install      - Install to system"
	@echo "  uninstall    - Remove from system"
	@echo "  deps         - Check dependencies"
	@echo "  install-deps-ubuntu - Install dependencies on Ubuntu/Debian"
	@echo "  install-deps-centos - Install dependencies on CentOS/RHEL"
	@echo "  docker-build - Build Docker image"
	@echo "  docker-run   - Run Docker container"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Remove all generated files"
	@echo "  help         - Show this help message"

# 依赖关系
$(CORE_OBJECTS): $(wildcard $(INCLUDE_DIR)/*/*.h)
$(MAIN_OBJECT): $(wildcard $(INCLUDE_DIR)/*/*.h)
$(TEST_OBJECTS): $(wildcard $(INCLUDE_DIR)/*/*.h)
