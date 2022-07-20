cmake_minimum_required (VERSION 3.10) #最低要求的CMake版本
project(ToyWeb LANGUAGES C CXX)  # 项目名称

#如果没有指定编译类型，自动设置为Debug
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 设置编译的主文件(自己写的)
set(SRC_FILE ./main.cpp)
#set(SRC_FILE ./test.cpp)
#set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")

# C++17
set(CMAKE_CXX_STANDARD 17)
# 设置编译器是否一定要支持该标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 设置是否启用GCC的扩展功能
set(CMAKE_CXX_EXTENSIONS OFF)

#set(CMAKE_CXX_FLAGS "${CAMKE_CXX_FLAGS} -std=c++11 -pthread")
# 要求编译可执行文件
add_executable (${PROJECT_NAME} ${SRC_FILE}) 

# 添加pthread链接
find_package(Threads)
# 链接库到项目中
target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

#添加头文件查找路径
INCLUDE_DIRECTORIES(./http
                    ./locker
                    ./threadpool
                    ./log
                    ./sql
                    ./FormParser
                    ./PostSolver
                    ./timer
                    ./signal)
# TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC ./threadpool)


# 添加需要编译的子目录(只能一个一个添加)
add_subdirectory(./http)
add_subdirectory(./locker)
add_subdirectory(./log)
add_subdirectory(./sql)
add_subdirectory(./FormParser)
add_subdirectory(./PostSolver)
add_subdirectory(./timer)

# 添加链接库
target_link_libraries(${PROJECT_NAME} httplib lockerlib loglib sqllib  formlib postlib timerlib)