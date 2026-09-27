cmake_minimum_required(VERSION 3.31)

project(P0x859 LANGUAGES CXX)

add_library(scsp_queue INTERFACE)
target_compile_features(scsp_queue INTERFACE cxx_std_20)
target_compile_options(scsp_queue INTERFACE -Wall -Wextra -Wpedantic)
target_include_directories(scsp_queue INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()