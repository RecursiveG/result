cmake_minimum_required(VERSION 3.11)
project(result VERSION 0.0.1 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -ggdb -O0")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

add_library(result INTERFACE)
target_include_directories(result INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

##
## Unit tests
##
enable_testing()
find_package(GTest REQUIRED)

add_executable(result_test
        ${CMAKE_CURRENT_SOURCE_DIR}/result_test.cpp)
target_link_libraries(result_test PRIVATE result GTest::GTest GTest::Main)
gtest_discover_tests(result_test)
