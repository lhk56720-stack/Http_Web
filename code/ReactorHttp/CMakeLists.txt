#指定Cmake版本
cmake_minimum_required(VERSION 3.28.1)
project(ReactorHttp LANGUAGES C)
#搜索头文件路径
include_directories(${PROJECT_SOURCE_DIR}/include)
#搜索源文件路径
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC)
#设置输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
add_executable(ReactorHttp
    ${SRC})