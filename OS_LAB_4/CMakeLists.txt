cmake_minimum_required(VERSION 3.15)
project(OS_LAB_4 LANGUAGES CXX)

# ------------------------- Настройки C++ -------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------- Основная программа -------------------------
add_executable(OS_LAB_4
    OS_LAB_4.cpp
    receiver.cpp
    receiver.h
    sender.cpp
    sender.h
    queue_file.cpp
    queue_file.h
    sync_utils.cpp
    sync_utils.h
)

if (MSVC)
    target_compile_options(OS_LAB_4 PRIVATE /W4)
    target_compile_definitions(OS_LAB_4 PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(OS_LAB_4 PRIVATE -Wall -Wextra -pedantic)
endif()

target_link_libraries(OS_LAB_4 PRIVATE kernel32 user32)

# ------------------------- ТЕСТЫ -------------------------
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(OS_LAB_4_tests
    tests.cpp
    receiver.cpp
    receiver.h
    sender.cpp
    sender.h
    queue_file.cpp
    queue_file.h
    sync_utils.cpp
    sync_utils.h
)

if (MSVC)
    target_compile_options(OS_LAB_4_tests PRIVATE /W4)
    target_compile_definitions(OS_LAB_4_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(OS_LAB_4_tests PRIVATE -Wall -Wextra -pedantic)
endif()

target_link_libraries(OS_LAB_4_tests PRIVATE 
    GTest::gtest 
    GTest::gtest_main 
    GTest::gmock
    kernel32 
    user32
)

include(GoogleTest)
gtest_discover_tests(OS_LAB_4_tests
    EXTRA_ARGS "--gtest_output=xml:${CMAKE_BINARY_DIR}/test_results.xml"
)