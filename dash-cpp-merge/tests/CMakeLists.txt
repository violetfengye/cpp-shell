# Tests directory CMakeLists.txt

# Attempt to find GTest (if available)
find_package(GTest QUIET)
find_package(Threads REQUIRED)

# Check if there are any test files
file(GLOB_RECURSE TEST_SOURCES 
    "*.cpp"
)

# If no test sources and GTest is found, create a dummy test
if(NOT TEST_SOURCES AND GTEST_FOUND)
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
    
    # Add test executable
    add_executable(dash-tests ${TEST_SOURCES})
    
    # Link Google Test and project libraries
    target_link_libraries(dash-tests PRIVATE
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
    )
    
    # Include directories
    target_include_directories(dash-tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${GTEST_INCLUDE_DIRS}
    )
    
    # Add test
    add_test(NAME dash-tests COMMAND dash-tests)
else()
    # If no tests yet, just emit a message
    message(STATUS "No test sources found in tests directory. Skipping test configuration.")
endif() 