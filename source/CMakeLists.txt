cmake_minimum_required(VERSION 3.10)
project(casca_log C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(hooks)
add_subdirectory(utils)

set(CASCA_LOG_SOURCES
        casca_log.c
        ${CASCA_LOG_HOOKS_SOURCES}
        ${CASCA_LOG_SECURE_FUNC_SOURCES}
)

add_library(casca_log ${CASCA_LOG_SOURCES} ${CASCA_LOG_HOOKS_HEADERS} ${CASCA_LOG_SECURE_FUNC_HEADERS})

target_include_directories(casca_log
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        ${CASCA_LOG_HOOKS_INCLUDE_DIR}
        ${CASCA_LOG_SECURE_FUNC_INCLUDE_DIR}
)

