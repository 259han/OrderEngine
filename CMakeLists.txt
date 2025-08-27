cmake_minimum_required(VERSION 3.16)
project(OrderEngine VERSION 1.0.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -g")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找第三方库
find_package(Boost REQUIRED COMPONENTS system filesystem thread log)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBHIREDIS REQUIRED hiredis)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(MySQL REQUIRED)
find_package(GTest REQUIRED)
find_package(spdlog REQUIRED)

# 包含头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${LIBHIREDIS_INCLUDE_DIRS})

# 添加子目录
add_subdirectory(src)
add_subdirectory(tests)

# 设置链接库
set(COMMON_LIBS
    ${Boost_LIBRARIES}
    ${LIBHIREDIS_LIBRARIES}
    protobuf::libprotobuf
    gRPC::grpc++
    mysqlclient
    pthread
    spdlog::spdlog
)

# 主执行程序
add_executable(order_engine
    src/main.cpp
)

target_link_libraries(order_engine
    order_engine_core
    ${COMMON_LIBS}
)

# 安装配置
install(TARGETS order_engine DESTINATION bin)
install(DIRECTORY config/ DESTINATION etc/order_engine)
