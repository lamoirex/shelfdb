cmake_minimum_required(VERSION 3.20)
project(shelfdb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC reads sources in the local code page by default; force UTF-8
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Core library: storage and indexes, no Qt dependency
add_library(shelf_core STATIC
    core/RecordCodec.cpp
)
target_include_directories(shelf_core PUBLIC core)

# Tests
enable_testing()

add_executable(codec_test tests/codec_test.cpp)
target_link_libraries(codec_test PRIVATE shelf_core)
add_test(NAME codec_test COMMAND codec_test)
