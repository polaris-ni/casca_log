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
add_test(NAME CascaLogTest COMMAND casca_log_test)

if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    message(STATUS "_CRT_SECURE_NO_WARNINGS is defined to suppress CRT SECURE WARNINGS")
endif ()

clog_enable_asan_for_target(casca_log_test)