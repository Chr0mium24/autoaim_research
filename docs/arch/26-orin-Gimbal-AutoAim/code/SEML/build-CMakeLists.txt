cmake_minimum_required(VERSION 3.0)#最低CMake版本

project (SEML_TEST)# 工程名

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
############################################################
# Create a library
############################################################

add_library(worker_library STATIC 
${PROJECT_SOURCE_DIR}/../Test/src/test_memory.c
)
#库的源文件Hello.cpp生成静态库hello_library

target_include_directories(worker_library
    PUBLIC 
 ${PROJECT_SOURCE_DIR}/../Test/Inc
)
# target_include_directories为一个目标（可能是一个库library也可能是可执行文件）添加头文件路径。


############################################################
# Create an executable
############################################################

add_executable(worker_binary 
${PROJECT_SOURCE_DIR}/../Test/src/main.c
)
#指定用哪个源文件生成可执行文件

target_link_libraries( worker_binary
    PUBLIC 
    worker_library
)
#链接可执行文件和静态库
#设置这个可执行文件需要包含的库的路径