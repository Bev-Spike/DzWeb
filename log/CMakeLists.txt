aux_source_directory(. LOG_LIB_SRCS)			# 添加当前目录下所有的源文件
add_library (loglib STATIC ${LOG_LIB_SRCS})				# 当前目录下的文件生成一个链接库
target_include_directories(loglib PUBLIC . ../threadpool)