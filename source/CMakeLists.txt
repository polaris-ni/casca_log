cmake_minimum_required(VERSION 3.10)
project(casca_log C)

add_subdirectory(hooks)
add_subdirectory(utils)
add_subdirectory(filter)
add_subdirectory(formatter)
add_subdirectory(dispatcher)
add_subdirectory(recorder)
add_subdirectory(channel)

set(CASCA_LOG_SOURCES
        casca_log_core.c
        ${CASCA_LOG_HOOKS_SOURCES}
        ${CASCA_LOG_UTILS_SOURCES}
        ${CASCA_LOG_FILTER_SOURCES}
        ${CASCA_LOG_FORMATTER_SOURCES}
        ${CASCA_LOG_DISPATCHER_SOURCES}
        ${CASCA_LOG_RECORDER_SOURCES}
        ${CASCA_LOG_CHANNEL_SOURCES}
)

add_library(casca_log STATIC ${CASCA_LOG_SOURCES}
        ${CASCA_LOG_HOOKS_HEADERS}
        ${CASCA_LOG_UTILS_HEADERS}
        ${CASCA_LOG_FILTER_HEADERS}
        ${CASCA_LOG_FORMATTER_HEADERS}
        ${CASCA_LOG_DISPATCHER_HEADERS}
        ${CASCA_LOG_RECORDER_HEADERS}
        ${CASCA_LOG_CHANNEL_HEADERS}
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
        ${CASCA_LOG_CHANNEL_INCLUDE_DIR}
)

include(CascaLogCMakeUtils.cmake)
casca_log_configure_switch_macro(casca_log "CASCA_LOG_DEBUG" "debug mode" OFF)
casca_log_configure_switch_macro(casca_log "CASCA_LOG_MEM_POOL" "use memory pool optimization" ON)
casca_log_configure_value_macro(casca_log "CASCA_LOG_ERR_BUF_SIZE" "single err buffer size" 256)
casca_log_configure_value_macro(casca_log "CASCA_LOG_SINGLE_LOG_MAX_SIZE" "single log item max size" 256)
casca_log_configure_value_macro(casca_log "CASCA_LOG_TARGET_RECORDER_MAX_NUM" "max recorder num supported" 4)
casca_log_configure_value_macro(casca_log "CASCA_LOG_CACHE_LINE_SIZE" "cache line size" 64)
casca_log_configure_value_macro(casca_log "CASCA_LOG_PROCESS_NAME_MAX_SIZE" "process name max size" 64)
casca_log_configure_value_macro(casca_log "CASCA_LOG_MODULE_NAME_MAX_SIZE" "module name max size" 64)
casca_log_configure_value_macro(casca_log "CASCA_LOG_FILENAME_MAX_SIZE" "filename max size" 64)
casca_log_configure_value_macro(casca_log "CASCA_LOG_FILEPATH_MAX_SIZE" "filepath max size" 256)
casca_log_configure_switch_macro(casca_log "CASCA_LOG_LOCKLESS" "log run on one thread, no need to use lock" OFF)

if (CMAKE_C_COMPILER_ID)
    if (CMAKE_C_COMPILER_VERSION)
        message(STATUS "BUILD ${PROJECT_NAME}: Compiler ${CMAKE_C_COMPILER_ID}(Version: ${CMAKE_C_COMPILER_VERSION})")
    else ()
        message(STATUS "BUILD ${PROJECT_NAME}: Compiler ${CMAKE_C_COMPILER_ID}(Version: UNKNOWN)")
    endif ()
endif ()

if (MSVC)
    if (MSVC_VERSION GREATER_EQUAL 1928)
        message(STATUS "BUILD ${PROJECT_NAME}: using MSVC(Version: ${MSVC_VERSION}), enabled \"/experimental:c11atomics /std:c11\" to support C11 STD ATOMIC")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /experimental:c11atomics /std:c11")
    else ()
        message(FATAL_ERROR "BUILD ${PROJECT_NAME}: using MSVC(Version: ${MSVC_VERSION}), not support C11 STD ATOMIC")
    endif ()
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    message(STATUS "BUILD ${PROJECT_NAME}: _CRT_SECURE_NO_WARNINGS is defined to suppress CRT SECURE WARNINGS")
else ()
    set(CMAKE_C_STANDARD 11) # avoid passing "-std=c11" to MSVC
endif ()

clog_enable_asan_for_target(casca_log)