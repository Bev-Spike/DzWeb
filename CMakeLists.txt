cmake_minimum_required (VERSION 2.8) #最低要求的CMake版本
project(ToyWeb) # 项目名称

aux_source_directory(. SRC_FILE)			# 添加当前目录下所有的源文件

# C++11
add_definitions(-std=c++11)

# 要求编译可执行文件
add_executable (${PROJECT_NAME} ${SRC_FILE}) 

# 添加pthread链接
find_package(Threads)
# 链接库到项目中
target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

#添加头文件查找路径
INCLUDE_DIRECTORIES(./http
                    ./locker
                    ./threadpool)

# 添加需要编译的子目录(只能一个一个添加)
add_subdirectory(./http)
add_subdirectory(./locker)
                    

# 添加链接库
target_link_libraries(${PROJECT_NAME} httplib lockerlib)