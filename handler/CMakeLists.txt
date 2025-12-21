cmake_minimum_required(VERSION 3.10)

# 设置策略
if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif ()

project(handler_test VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB handler_SOURCES
        ${CMAKE_SOURCE_DIR}/handler/src/*.cpp
        ${CMAKE_SOURCE_DIR}/handler/TestHandler.cpp
        )
file(GLOB handler_HEADER ${CMAKE_SOURCE_DIR}/handler/include/*.h)

include_directories(${CMAKE_SOURCE_DIR}/handler/include)
add_executable(handler_test ${handler_SOURCES} ${handler_HEADER})