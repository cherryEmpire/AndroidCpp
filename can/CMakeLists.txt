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

file(GLOB can_SOURCES
        ${CMAKE_SOURCE_DIR}/can/*.cpp
        )
file(GLOB can_HEADER
        ${CMAKE_SOURCE_DIR}/can/*.h
        ${CMAKE_SOURCE_DIR}/common/*.h
        ${CMAKE_SOURCE_DIR}/common/*.hpp
        )

include_directories(${CMAKE_SOURCE_DIR}/can
        ${CMAKE_SOURCE_DIR}/common
        )
add_executable(can_test ${can_SOURCES} ${can_HEADER})