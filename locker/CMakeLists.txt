aux_source_directory(. LIB_SRCS)			# 添加当前目录下所有的源文件
add_library (lockerlib STATIC ${LIB_SRCS})				# 当前目录下的文件生成一个链接库
target_include_directories(lockerlib PUBLIC . ../threadpool)
# 添加pthread链接
find_package(Threads)
# 链接库到项目中
target_link_libraries (lockerlib ${CMAKE_THREAD_LIBS_INIT})
