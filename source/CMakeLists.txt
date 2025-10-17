cmake_minimum_required(VERSION 3.10)
project(casca_log C)

set(CMAKE_C_STANDARD 11)

add_subdirectory(hooks)
add_subdirectory(utils)
add_subdirectory(filter)
add_subdirectory(formatter)
add_subdirectory(dispatcher)
add_subdirectory(recorder)

set(CASCA_LOG_SOURCES
        casca_log.c
        ${CASCA_LOG_HOOKS_SOURCES}
        ${CASCA_LOG_UTILS_SOURCES}
        ${CASCA_LOG_FILTER_SOURCES}
        ${CASCA_LOG_FORMATTER_SOURCES}
        ${CASCA_LOG_DISPATCHER_SOURCES}
        ${CASCA_LOG_RECORDER_SOURCES}
)

add_library(casca_log ${CASCA_LOG_SOURCES}
        ${CASCA_LOG_HOOKS_HEADERS}
        ${CASCA_LOG_UTILS_HEADERS}
        ${CASCA_LOG_FILTER_HEADERS}
        ${CASCA_LOG_FORMATTER_HEADERS}
        ${CASCA_LOG_DISPATCHER_HEADERS}
        ${CASCA_LOG_RECORDER_HEADERS}
)

target_include_directories(casca_log
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CASCA_LOG_HOOKS_INCLUDE_DIR}
        ${CASCA_LOG_UTILS_INCLUDE_DIR}
        ${CASCA_LOG_FILTER_INCLUDE_DIR}
        ${CASCA_LOG_FORMATTER_INCLUDE_DIR}
        ${CASCA_LOG_DISPATCHER_INCLUDE_DIR}
        ${CASCA_LOG_RECORDER_INCLUDE_DIR}
)

