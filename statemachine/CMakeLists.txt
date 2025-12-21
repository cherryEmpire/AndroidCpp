cmake_minimum_required(VERSION 3.10)

# 设置策略
if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()

project(statemachine_test VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB statemachine_SOURCES ${CMAKE_SOURCE_DIR}/statemachine/*.cpp)
file(GLOB statemachine_HEADER ${CMAKE_SOURCE_DIR}/statemachine/*.h)

include_directories(${CMAKE_SOURCE_DIR}/statemachine)
add_executable(statemachine_test ${statemachine_SOURCES} ${statemachine_HEADER})
