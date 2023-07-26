cmake_minimum_required(VERSION 3.0.0)

project(ftp)

include_directories(./include)

add_executable(server server.cpp 
src/Acceptor.cpp 
src/Buffer.cpp 
src/Channel.cpp
src/Connection.cpp
src/Epoll.cpp
src/EventLoop.cpp 
src/Server.cpp 
src/Socket.cpp 
src/InetAddress.cpp
src/ThreadPool.cpp 
src/util.cpp
src/handler.cpp 
src/common.cpp)


 add_executable(client client.cpp 
src/common.cpp )


target_link_libraries(server -pthread)

target_link_libraries(client -pthread)

