cmake_minimum_required(VERSION 3.16)
project(CppLearningDemos)

set(CMAKE_CXX_STANDARD 14)

#配置boost库
#添加头文件搜索路径
include_directories(/Users/yshen/CodeLife/boost_1_73_0/prefix/include)
include_directories(/usr/local/Cellar/openssl@1.1/1.1.1d/include)
#添加库文件搜索路径
link_directories(/Users/yshen/CodeLife/boost_1_73_0/prefix/lib)
link_directories(/usr/local/Cellar/openssl@1.1/1.1.1d/lib)

add_executable(CppLearningDemos main.cpp src/SomeTools.cpp src/SomeTools.h src/TestClazzSize.cpp src/TestClazzSize.h demos.cpp src/TestFileTemplate.cpp src/TestFileTemplate.h src/SimpleHttpClient.cpp src/SimpleHttpClient.h src/SimpleHttpClient2.cpp src/SimpleHttpClient2.h src/ROOT_CERITICATES.h src/SimpleHttpsClient.cpp src/SimpleHttpsClient.h)
#使用openssl 需要link如下两个库, 使用非head-only的库需要手动link
target_link_libraries(CppLearningDemos ssl crypto boost_coroutine)

#配置多个main函数同时运行
#servers
add_executable(servers servers.cpp src/SimpleHttpServer.cpp src/SimpleHttpServer.h src/RequestHandler.h)
target_link_libraries(servers ssl crypto boost_coroutine)