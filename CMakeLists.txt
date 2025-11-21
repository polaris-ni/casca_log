cmake_minimum_required(VERSION 3.14)
project(casca_log_build C)

option(BUILD_CASCA_LOG_TEST_SUITE "Enable GoogleTest for casca-log" OFF)
option(BUILD_CASCA_LOG_TEST_ASAN "Enable AddressSanitizer for casca-log and tests" OFF)

function(clog_enable_asan_for_target TARGET_NAME)
    if (BUILD_CASCA_LOG_TEST_ASAN)
        if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${TARGET_NAME} PRIVATE
                    -fsanitize=address
                    -fno-omit-frame-pointer
                    -g
                    -O1
            )
            target_link_options(${TARGET_NAME} PRIVATE
                    -fsanitize=address
            )
            message(STATUS "AddressSanitizer is enabled on ${TARGET_NAME}")
        else ()
            message(STATUS "AddressSanitizer is not supported on ${TARGET_NAME}")
        endif ()
    endif ()
endfunction()

add_subdirectory(source)
if (BUILD_CASCA_LOG_TEST_SUITE)
    enable_testing()
    add_subdirectory(test)
endif ()