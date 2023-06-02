cmake_minimum_required(VERSION 3.25)
project(webserv)

set(CMAKE_CXX_STANDARD 98)

include_directories(srcs)

add_executable(webserv
        srcs/config/Config.h
        srcs/server/Route.h
        srcs/server/VServer.h
        srcs/Server.h
        srcs/main.cc srcs/http/Header.cc srcs/http/Request.h srcs/http/Request.cc)
