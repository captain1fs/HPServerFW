cmake_minimum_required(VERSION 2.8)
#项目名称
project(sylar)

#开启编译时的详细信息打印
set(CMAKE_VERBOSE_MAKEFILE ON)
#设置g++编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")


include_directories("/home/fangshao/CPP/Project/yaml-cpp/build/")


set(LIB_SRC
    sylar/log.cc    
    sylar/util.cc    
    sylar/config.cc    
)

#指定编译文件，编译可执行程序
add_library(sylar SHARED ${LIB_SRC})

#编译库文件
#add_library(sylar_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (sylar_static PROPERTIES OUTPUT_NAME "sylar")

add_executable(test_log tests/test_log.cc)
add_dependencies(test_log sylar)
#添加依赖的库文件
target_link_libraries(test_log sylar /home/fangshao/CPP/Project/yaml-cpp/build/libyaml-cpp.so)

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config sylar)
#添加依赖的库文件
target_link_libraries(test_config sylar /home/fangshao/CPP/Project/yaml-cpp/build/libyaml-cpp.so)

#指定可执行文件输出路径
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#指定库文件输出路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)