# CMake specifications
# ------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.7...3.26)
project(
  webserv
  DESCRIPTION "A simple web server"
  HOMEPAGE_URL "https://github.com/ciuckc/webserv"
  VERSION 0.1
  LANGUAGES CXX)

# CMake variables
# ------------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER g++)

# Optimization/debug flags
# -------------------------------------------------------#----------------------
#   Always:  generate assembly for native architecture   #
#   Debug:   keep debug info and don't optimize          #
#   Release: full send optimization                      #
# -------------------------------------------------------#
set(CMAKE_CXX_FLAGS         -march=native)
set(CMAKE_CXX_FLAGS_DEBUG   -g)
set(CMAKE_CXX_FLAGS_RELEASE -O3)

# Compilation specifications
# ------------------------------------------------------------------------------
add_compile_options(-Wall -Wextra -Werror)

# Includes
# ------------------------------------------------------------------------------
include_directories(srcs)

add_executable(webserv
        srcs/main.cc
        srcs/Server.h
        srcs/Server.cc
        srcs/config/Config.h
        srcs/server/Route.h
        srcs/server/VServer.h
        srcs/http/Message.h
        srcs/http/Message.cc
        srcs/http/Request.h
        srcs/http/Request.cc
        srcs/http/Response.h
        srcs/http/Response.cc
        srcs/http/ErrorResponse.h
        srcs/http/ErrorResponse.cc
        srcs/io/Socket.h
        srcs/io/Socket.cc
        srcs/io/EventQueue.h
        srcs/io/EventQueue.cc
        srcs/io/Connection.h
        srcs/io/Connection.cc
        srcs/io/ConnectionBuffer.h
        srcs/io/ConnectionBuffer.cc
        srcs/io/BufferPool.h
        srcs/io/IOException.h
        srcs/io/task/IOTask.h
        srcs/io/task/ReadRequest.h
        srcs/io/task/ReadRequest.cc
        srcs/io/task/SendResponse.h
        srcs/io/task/SendResponse.cc
        srcs/util/WebServ.h
        srcs/util/Log.h)

set_target_properties(webserv PROPERTIES RUNTIME_OUTPUT_DIRECTORY
        ${CMAKE_SOURCE_DIR})
