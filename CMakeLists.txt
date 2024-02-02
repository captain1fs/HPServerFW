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
    windgent/http/http.cc
    windgent/http/http_parser.cc
    windgent/address.cc
    windgent/bytearray.cc
    windgent/config.cc
    windgent/fiber.cc
    windgent/fd_manager.cc
    windgent/hook.cc
    windgent/iomanager.cc
    windgent/log.cc 
    windgent/mutex.cc   
    windgent/scheduler.cc 
    windgent/socket.cc 
    windgent/socket_stream.cc 
    windgent/stream.cc 
    windgent/tcp_server.cc
    windgent/thread.cc
    windgent/timer.cc
    windgent/util.cc
)

ragelmaker(windgent/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/windgent/http)
ragelmaker(windgent/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/windgent/http)

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
    dl
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

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler windgent)
force_redefine_file_macro_for_sources(test_scheduler)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_scheduler ${LIB_LIB})

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager windgent)
force_redefine_file_macro_for_sources(test_iomanager)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_iomanager ${LIB_LIB})

add_executable(test_hook tests/test_hook.cc)
add_dependencies(test_hook windgent)
force_redefine_file_macro_for_sources(test_hook)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_hook ${LIB_LIB})

add_executable(test_address tests/test_address.cc)
add_dependencies(test_address windgent)
force_redefine_file_macro_for_sources(test_address)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_address ${LIB_LIB})

add_executable(test_socket tests/test_socket.cc)
add_dependencies(test_socket windgent)
force_redefine_file_macro_for_sources(test_socket)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_socket ${LIB_LIB})

add_executable(test_bytearray tests/test_bytearray.cc)
add_dependencies(test_bytearray windgent)
force_redefine_file_macro_for_sources(test_bytearray)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_bytearray ${LIB_LIB})

add_executable(test_http tests/test_http.cc)
add_dependencies(test_http windgent)
force_redefine_file_macro_for_sources(test_http)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_http ${LIB_LIB})

add_executable(test_http_parser tests/test_http_parser.cc)
add_dependencies(test_http_parser windgent)
force_redefine_file_macro_for_sources(test_http_parser)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_http_parser ${LIB_LIB})

add_executable(test_tcp_server tests/test_tcp_server.cc)
add_dependencies(test_tcp_server windgent)
force_redefine_file_macro_for_sources(test_tcp_server)   #__FILE__
#添加依赖的库文件
target_link_libraries(test_tcp_server ${LIB_LIB})

add_executable(echo_server examples/echo_server.cc)
add_dependencies(echo_server windgent)
force_redefine_file_macro_for_sources(echo_server)   #__FILE__
#添加依赖的库文件
target_link_libraries(echo_server ${LIB_LIB})

#指定可执行文件输出路径
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#指定库文件输出路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)