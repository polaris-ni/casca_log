cmake_minimum_required(VERSION 3.14)
project(casca_log C)

set(CMAKE_C_STANDARD 11)

include_directories(include)
include_directories(source/hooks)

add_subdirectory(source)
option(CASCA_LOG_TEST_SUITE "Enable casca-log GTest dependency" OFF)
if (CASCA_LOG_TEST_SUITE)
    add_subdirectory(test)
endif ()

add_executable(casca_log_exe main.c)

target_link_libraries(casca_log_exe casca_log)
