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
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER c++)

# Compilation specifications
# ------------------------------------------------------------------------------
add_compile_options(-Wall -Wextra -Werror)
# add_compile_options(-Wconversion -Wnull-dereference -Wnon-virtual-dtor
# -Wcast-align)

# Includes
# ------------------------------------------------------------------------------
include_directories(srcs)
include_directories(srcs/config)
include_directories(srcs/http)
include_directories(srcs/io)
include_directories(srcs/server)

add_executable(
  webserv
  srcs/main.cc
  srcs/Server.h
  srcs/Server.cc
  srcs/config/Config.h
  srcs/config/Config.cc
  srcs/config/ConfigParse.h
  srcs/config/ConfigParse.cc
  srcs/config/ConfigBlock.h
  srcs/config/ConfigBlock.cc
  srcs/server/Route.h
  srcs/server/VServer.h
  srcs/http/Request.h
  srcs/http/Request.cc
  srcs/http/Response.h
  srcs/http/Response.cc
  srcs/http/Headers.h
  srcs/http/Headers.cc
  srcs/http/ErrorResponse.h
  srcs/http/ErrorResponse.cc
  srcs/io/Socket.h
  srcs/io/Socket.cc
  srcs/io/EventQueue.h
  srcs/io/EventQueue.cc
  srcs/io/IOException.h
  srcs/io/IOTask.h)

set_target_properties(webserv PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                         ${CMAKE_SOURCE_DIR})
