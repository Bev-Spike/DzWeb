aux_source_directory(. FORM_LIB_SRCS)			# 添加当前目录下所有的源文件
add_library (formlib STATIC ${FORM_LIB_SRCS})				# 当前目录下的文件生成一个链接库
target_include_directories(formlib PUBLIC .)  #允许上级继承头文件