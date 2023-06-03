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
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compilation specifications
# ------------------------------------------------------------------------------
add_compile_options(-Wall -Wextra -Werror)

# Includes
# ------------------------------------------------------------------------------
include_directories(srcs)
include_directories(srcs/config)
include_directories(srcs/http)
include_directories(srcs/server)

# Sources
# ------------------------------------------------------------------------------
add_executable(
  webserv
  srcs/main.cc
  srcs/Server.h
  srcs/config/Config.h
  srcs/server/Route.h
  srcs/server/VServer.h
  srcs/http/Headers.cc
  srcs/http/Request.h
  srcs/http/Request.cc
  srcs/http/Headers.h)

set_target_properties(webserv PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                         ${CMAKE_SOURCE_DIR})
