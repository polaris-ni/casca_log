cmake_minimum_required(VERSION 3.14)
project(casca_log_build C)

add_subdirectory(source)
option(CASCA_LOG_TEST_SUITE "Enable casca-log GTest dependency" OFF)
if (CASCA_LOG_TEST_SUITE)
    enable_testing()
    add_subdirectory(test)
endif ()