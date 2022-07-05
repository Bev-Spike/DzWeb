aux_source_directory(. LIB_SRCS)			# 添加当前目录下所有的源文件
add_library (sqllib STATIC ${LIB_SRCS})				# 当前目录下的文件生成一个链接库
target_include_directories(sqllib PUBLIC .)
target_link_libraries(sqllib /usr/lib/x86_64-linux-gnu/libmysqlclient.so)