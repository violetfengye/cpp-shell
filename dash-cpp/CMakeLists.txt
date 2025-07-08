cmake_minimum_required(VERSION 3.10)
project(dash-cpp VERSION 0.1.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置 CMake 模块路径
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 设置编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

# 选项
option(BUILD_TESTS "Build tests" ON)
option(USE_READLINE "Use readline library" ON)

# 依赖检查
if(USE_READLINE)
    find_package(Readline REQUIRED)
    if(READLINE_FOUND)
        add_definitions(-DREADLINE_ENABLED)
        message(STATUS "Found Readline: ${READLINE_INCLUDE_DIR}")
    else()
        message(WARNING "Readline not found, disabling Readline support")
        set(USE_READLINE OFF)
    endif()
endif()

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if(USE_READLINE AND READLINE_FOUND)
    include_directories(${READLINE_INCLUDE_DIR})
endif()

# 源文件
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    src/builtins/bg_command.cpp
    src/builtins/cd_command.cpp
    src/builtins/echo_command.cpp
    src/builtins/exit_command.cpp
    src/builtins/fg_command.cpp
    src/builtins/jobs_command.cpp
    src/builtins/pwd_command.cpp
    src/builtins/help_command.cpp
    src/builtins/debug_command.cpp
    src/builtins/source_command.cpp
    src/core/debug.cpp
)

# 头文件
file(GLOB_RECURSE HEADERS 
    "include/*.h"
    "src/core/debug.h"
)

# 分离主函数文件和库文件
file(GLOB MAIN_SOURCE "src/main.cpp")
list(REMOVE_ITEM SOURCES ${MAIN_SOURCE})

# 创建库
add_library(dash-lib STATIC ${SOURCES} ${HEADERS})
if(USE_READLINE AND READLINE_FOUND)
    target_link_libraries(dash-lib PRIVATE ${READLINE_LIBRARIES})
endif()

# 可执行文件
add_executable(dash ${MAIN_SOURCE})
target_link_libraries(dash PRIVATE dash-lib)

# 补全测试程序
add_executable(test_completion test_completion.cpp)
target_link_libraries(test_completion PRIVATE dash-lib)

# 安装规则
install(TARGETS dash DESTINATION bin)

# 测试
if(BUILD_TESTS)
    enable_testing()
    # 添加测试子目录
    add_subdirectory(tests)
endif()
