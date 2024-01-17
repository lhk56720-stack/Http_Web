#指定cmake版本
cmake_minimum_required(VERSION 3.28.1)
project(ReactorHttp_CPP LANGUAGES CXX)
#包含头文件
include_directories(${PROJECT_SOURCE_DIR}/include)
#搜索源文件路径
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC)
#设置MakeFile和可执行文件的输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
add_executable(ReactorHttp_CPP
        ${SRC})
