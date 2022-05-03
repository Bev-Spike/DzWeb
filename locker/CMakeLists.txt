

aux_source_directory(. LIB_SRCS)			# 添加当前目录下所有的源文件
add_library (lockerlib ${LIB_SRCS})				# 当前目录下的文件生成一个链接库
