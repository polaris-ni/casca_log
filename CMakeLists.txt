cmake_minimum_required(VERSION 4.0)
project(casca_log C)

set(CMAKE_C_STANDARD 99)

include_directories(include)

add_executable(casca_log main.c)
