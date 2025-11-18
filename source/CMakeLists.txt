cmake_minimum_required(VERSION 3.10)
project(casca_log C)

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

add_library(casca_log STATIC ${CASCA_LOG_SOURCES}
        ${CASCA_LOG_HOOKS_HEADERS}
        ${CASCA_LOG_UTILS_HEADERS}
        ${CASCA_LOG_FILTER_HEADERS}
        ${CASCA_LOG_FORMATTER_HEADERS}
        ${CASCA_LOG_DISPATCHER_HEADERS}
        ${CASCA_LOG_RECORDER_HEADERS}
)

target_include_directories(casca_log
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CASCA_LOG_HOOKS_INCLUDE_DIR}
        ${CASCA_LOG_UTILS_INCLUDE_DIR}
        ${CASCA_LOG_FILTER_INCLUDE_DIR}
        ${CASCA_LOG_FORMATTER_INCLUDE_DIR}
        ${CASCA_LOG_DISPATCHER_INCLUDE_DIR}
        ${CASCA_LOG_RECORDER_INCLUDE_DIR}
)

if (CMAKE_C_COMPILER_ID)
    if (CMAKE_C_COMPILER_VERSION)
        message(STATUS "Casca Log Compiler: ${CMAKE_C_COMPILER_ID}(Version: ${CMAKE_C_COMPILER_VERSION})")
    else ()
        message(STATUS "Casca Log Compiler: ${CMAKE_C_COMPILER_ID}(Version: UNKNOWN)")
    endif ()
endif ()

if (MSVC)
    if (MSVC_VERSION GREATER_EQUAL 1928)
        message(STATUS "Using MSVC(Version: ${MSVC_VERSION}), enabled \"/experimental:c11atomics /std:c11\" to support C11 STD ATOMIC")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /experimental:c11atomics /std:c11")
    else ()
        message(FATAL_ERROR "Using MSVC(Version: ${MSVC_VERSION}), not support C11 STD ATOMIC")
    endif ()
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    message(STATUS "_CRT_SECURE_NO_WARNINGS is defined to suppress CRT SECURE WARNINGS")
else ()
    # avoid passing "-std=c11" to MSVC
    set(CMAKE_C_STANDARD 11)
endif ()

clog_enable_asan_for_target(casca_log)