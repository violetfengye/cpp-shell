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
endif()

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if(USE_READLINE AND READLINE_FOUND)
    include_directories(${READLINE_INCLUDE_DIR})
endif()

# 源文件
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

# 头文件
file(GLOB_RECURSE HEADERS 
    "include/*.h"
)
message("所有被编译的头文件:")
foreach(header IN LISTS HEADERS)
    message(${header})
endforeach()
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

# 安装规则
install(TARGETS dash DESTINATION bin)

# # 测试
# if(BUILD_TESTS)
# enable_testing()
# # 添加测试子目录
# add_subdirectory(tests)
# endif()
