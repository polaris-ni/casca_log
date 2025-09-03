cmake_minimum_required(VERSION 4.0)
project(casca_log C)

set(CMAKE_C_STANDARD 99)

include_directories(include)
include_directories(source/hooks)

add_subdirectory(source)

add_executable(casca_log_exe main.c)

target_link_libraries(casca_log_exe casca_log)
