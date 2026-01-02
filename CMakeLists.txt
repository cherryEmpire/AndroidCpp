cmake_minimum_required(VERSION 3.10)

# 设置策略
if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()

project(cpp_test VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(handler)
add_subdirectory(statemachine)
add_subdirectory(can)