cmake_minimum_required(VERSION 3.0)
#对g++追加选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -Wall")
#指定使用g++编译器
set(CMAKE_CXX_COMPILER "g++")
#设置变量
set(SRC main.cpp  ./timer/lst_timer.cpp ./http/http_conn.cpp ./log/log.cpp ./CGImysql/sql_connection_pool.cpp webserver/webserver.cpp config/config.cpp )
#项目名称
project(tinywebserver)
#指定头文件搜索路径，相当与GCC的-I
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})
#向工程添加多个特定的库文件搜索路径，相当与GCC 的-L
link_directories(${PROJECT_SOURCE_DIR})
#生成库文件
#add_library()
#添加编译参数-Wall -std=c++11 -O2
add_compile_options(-g -Wall)
#生成可执行文件
add_executable(server ${SRC})
#为target添加需要链接的共享库，相当于GCC的-l参数
target_link_libraries(server pthread mysqlclient)
#向当前工程添加存放源文件的子目录，并可以指定中间二进制和目标二进制存放的位置
#add_subdirectory()
#发现一个目录下所有的源代码文件将列表存储在一个变量中，这个指令临时被用来自动构建源文件列表
#aux_source_directory()