cmake_minimum_required(VERSION 3.14)
project(casca_log_build)

include(FetchContent)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_language(CXX)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE CASCA_LOG_TEST_SOURCES "./*.cpp")
set(CASCA_LOG_TEST_SOURCES ${CASCA_LOG_TEST_SOURCES} PARENT_SCOPE)

add_executable(casca_log_test ${CASCA_LOG_TEST_SOURCES})
target_link_libraries(casca_log_test GTest::gtest_main casca_log)
include(../source/CascaLogCMakeUtils.cmake)
casca_log_configure_switch_macro(casca_log_test "CASCA_LOG_DEBUG" "debug mode" OFF)
casca_log_configure_switch_macro(casca_log_test "CASCA_LOG_MEM_POOL" "use memory pool optimization" ON)
casca_log_configure_value_macro(casca_log_test "CASCA_LOG_ERR_BUF_SIZE" "single err buffer size" 256)
casca_log_configure_value_macro(casca_log_test "CASCA_LOG_SINGLE_LOG_MAX_SIZE" "single log item max size" 256)
casca_log_configure_value_macro(casca_log_test "CASCA_LOG_TARGET_RECORDER_MAX_NUM" "max recorder num supported" 4)
casca_log_configure_value_macro(casca_log_test "CASCA_LOG_CACHE_LINE_SIZE" "cache line size" 64)
casca_log_configure_value_macro(casca_log_test "CASCA_LOG_PROCESS_NAME_MAX_SIZE" "process name max size" 64)
casca_log_configure_value_macro(casca_log_test "CASCA_LOG_MODULE_NAME_MAX_SIZE" "module name max size" 64)
casca_log_configure_value_macro(casca_log_test "CASCA_LOG_FILENAME_MAX_SIZE" "filename max size" 64)
casca_log_configure_value_macro(casca_log_test "CASCA_LOG_FILEPATH_MAX_SIZE" "filepath max size" 256)
add_test(NAME CascaLogTest COMMAND casca_log_test)

if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    message(STATUS "BUILD casca_log_test: _CRT_SECURE_NO_WARNINGS is defined to suppress CRT SECURE WARNINGS")
endif ()

clog_enable_asan_for_target(casca_log_test)