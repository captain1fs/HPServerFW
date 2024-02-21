cmake_minimum_required(VERSION 2.8)
#项目名称
project(windgent)

#在camke文件中可定义函数
include(cmake/utils.cmake)

#开启编译时的详细信息打印
set(CMAKE_VERBOSE_MAKEFILE ON)
#设置g++编译选项
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories("/home/fangshao/CPP/Project/yaml-cpp/build/")

#设置依赖的源文件
set(LIB_SRC   
    windgent/http/http.cc
    windgent/http/http_parser.cc
    windgent/http/http_connection.cc
    windgent/http/http_session.cc
    windgent/http/http_server.cc
    windgent/http/servlet.cc
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
ragelmaker(windgent/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/windgent)

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

windgent_add_executable(test_log "tests/test_log.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_config "tests/test_config.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_thread "tests/test_thread.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_util "tests/test_util.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_fiber "tests/test_fiber.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_scheduler "tests/test_scheduler.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_iomanager "tests/test_iomanager.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_hook "tests/test_hook.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_address "tests/test_address.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_socket "tests/test_socket.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_bytearray "tests/test_bytearray.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_http "tests/test_http.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_http_server "tests/test_http_server.cc" windgent "${LIB_LIB}")
windgent_add_executable(echo_server "examples/echo_server.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_http_connection "tests/test_http_connection.cc" windgent "${LIB_LIB}")
windgent_add_executable(test_uri "tests/test_uri.cc" windgent "${LIB_LIB}")
windgent_add_executable(abtest_http_server "samples/abtest_http_server.cc" windgent "${LIB_LIB}")

# #指定编译文件
# add_executable(test_log tests/test_log.cc)
# add_dependencies(test_log windgent)
# force_redefine_file_macro_for_sources(test_log)   #__FILE__
# #添加依赖的库文件
# target_link_libraries(test_log ${LIB_LIB})

#指定可执行文件输出路径
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#指定库文件输出路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)