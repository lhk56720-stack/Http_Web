#指定Cmake版本
cmake_minimum_required(VERSION 3.28.1)  
project(SimpleHttp LANGUAGES C)
#设置可执行程序的输出路径
#set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
#生成执行文件
add_executable(SimpleHttp
    main.c
    Server.c
    Server.h
) 