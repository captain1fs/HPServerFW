cmake_minimum_required(VERSION 2.8)
#项目名称
project(windgent)

include(cmake/utils.cmake)

#开启编译时的详细信息打印
set(CMAKE_VERBOSE_MAKEFILE ON)
#设置g++编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories("/home/fangshao/CPP/Project/yaml-cpp/build/")

#设置依赖的源文件
set(LIB_SRC
    windgent/log.cc    
    windgent/util.cc    
    windgent/config.cc    
    windgent/thread.cc
    windgent/mutex.cc
    windgent/fiber.cc
)

#设置依赖的库
set(YAMLCPP
    /home/fangshao/CPP/Project/yaml-cpp/build/libyaml-cpp.so
)

#指定编译文件，编译可执行程序
add_library(windgent SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(windgent)

#编译库文件
#add_library(windgent_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (windgent_static PROPERTIES OUTPUT_NAME "windgent")

#设置依赖的库
set(LIB_LIB
    windgent
    pthread
    ${YAMLCPP}
)

#指定编译文件
add_executable(test_log tests/test_log.cc)
add_dependencies(test_log windgent)
force_redefine_file_macro_for_sources(test_log)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_log ${LIB_LIB})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config windgent)
force_redefine_file_macro_for_sources(test_config)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_config ${LIB_LIB})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread windgent)
force_redefine_file_macro_for_sources(test_thread)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_thread ${LIB_LIB})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util windgent)
force_redefine_file_macro_for_sources(test_util)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_util ${LIB_LIB})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber windgent)
force_redefine_file_macro_for_sources(test_fiber)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_fiber ${LIB_LIB})


#指定可执行文件输出路径
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#指定库文件输出路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)