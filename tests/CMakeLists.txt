# 测试文件
set(TEST_SOURCES
    test_main.cpp
    test_logger.cpp
    test_config.cpp
    test_reactor.cpp
    test_tcp_server.cpp
)

# 创建测试可执行文件
add_executable(order_engine_test ${TEST_SOURCES})

target_link_libraries(order_engine_test
    order_engine_core
    ${COMMON_LIBS}
    ${GTEST_LIBS}
)
