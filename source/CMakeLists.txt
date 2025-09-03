cmake_minimum_required(VERSION 4.0)
project(casca_log C)

set(CMAKE_C_STANDARD 99)

add_subdirectory(hooks)

set(CASCA_LOG_SOURCES
        casca_log.c
        ${CASCA_LOG_HOOKS_SOURCES}
)

add_library(casca_log ${CASCA_LOG_SOURCES} ${CASCA_LOG_HOOKS_HEADERS})

target_include_directories(casca_log
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        ${CASCA_LOG_HOOKS_INCLUDE_DIR}
)

