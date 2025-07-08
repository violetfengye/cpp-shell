# 测试目录的 CMakeLists.txt

# 查找本地安装的 Google Test
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)  # GTest通常需要线程库

# 测试源文件
file(GLOB_RECURSE TEST_SOURCES 
    "*.cpp"
)

# 如果没有测试源文件，创建一个简单的测试
if(NOT TEST_SOURCES)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_test.cpp "
        #include <gtest/gtest.h>
        TEST(DummyTest, AlwaysPass) {
            EXPECT_TRUE(true);
        }
        int main(int argc, char **argv) {
            ::testing::InitGoogleTest(&argc, argv);
            return RUN_ALL_TESTS();
        }
    ")
    set(TEST_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/dummy_test.cpp)
endif()

# 添加测试可执行文件
add_executable(dash-cpp-tests ${TEST_SOURCES})

# 链接 Google Test 和项目库
target_link_libraries(dash-cpp-tests PRIVATE
    ${GTEST_BOTH_LIBRARIES}  # 包含gtest和gtest_main
    ${CMAKE_THREAD_LIBS_INIT}  # 线程库
    # 链接到项目的库
    dash-lib
)

# 包含目录
target_include_directories(dash-cpp-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${GTEST_INCLUDE_DIRS}  # GTest头文件路径
)

# 添加测试
add_test(NAME dash-cpp-tests COMMAND dash-cpp-tests) 