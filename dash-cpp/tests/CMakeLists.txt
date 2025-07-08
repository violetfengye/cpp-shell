# 测试目录的 CMakeLists.txt

# 查找 Google Test
find_package(GTest QUIET)

if(NOT GTEST_FOUND)
    # 如果没有找到 Google Test，则下载并构建它
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.11.0
    )
    FetchContent_MakeAvailable(googletest)
endif()

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
    gtest
    gtest_main
    # 链接到项目的库
    dash-lib
)

# 包含目录
target_include_directories(dash-cpp-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${GTEST_INCLUDE_DIRS}
)

# 添加测试
add_test(NAME dash-cpp-tests COMMAND dash-cpp-tests) 