aux_source_directory(. POST_LIB_SRCS)			# 添加当前目录下所有的源文件
add_library (postlib STATIC ${POST_LIB_SRCS})				# 当前目录下的文件生成一个链接库
target_include_directories(postlib PUBLIC . ../FormParser)  #允许上级继承头文件
target_link_libraries (postlib formlib sqllib)