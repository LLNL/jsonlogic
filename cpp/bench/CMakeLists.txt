# Copyright 2020 Lawrence Livermore National Security, LLC and other CLIPPy
# Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: MIT

# Works with 3.11 and tested through 3.15 (not tested yet)
cmake_minimum_required(VERSION 3.14)
set(ALLOW_DUPLICATE_CUSTOM_TARGETS TRUE)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)

endif()
set(CMAKE_OSX_SYSROOT)
project(Benchmark
    VERSION 0.01
    DESCRIPTION "Benchmarks JSONLogic"
    LANGUAGES CXX)

include(FetchContent)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    
    # Let's ensure -std=c++xx instead of -std=g++xx
    set(CMAKE_CXX_EXTENSIONS OFF)

    # Let's nicely support folders in IDE's
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    # Testing only available if this is the main app
    include(CTest)

endif()
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
# Set paths to jsonlogic and boost from the grandparent directory's build tree
set(JSONLOGIC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(BOOST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/_deps/boost-src")

message(STATUS "JSONLOGIC_DIR: ${JSONLOGIC_DIR}")
list(APPEND CMAKE_PREFIX_PATH "${JSONLOGIC_DIR}/CMakeLists.txt")

message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
# Add jsonlogic and boost as dependencies
find_package(jsonlogic REQUIRED PATHS "${JSONLOGIC_DIR}")
find_package(Boost REQUIRED COMPONENTS json PATHS "${BOOST_DIR}")

message(STATUS "jsonlogic source dir: ${JSONLOGIC_DIR}")
message(STATUS "Boost source dir: ${BOOST_DIR}")

include_directories("${PROJECT_SOURCE_DIR}/include")
message(STATUS "adding executable")
add_executable(benchmark1 src/benchmark1.cpp)

target_compile_options(benchmark1 PRIVATE -O3)
target_link_libraries(benchmark1 PRIVATE Boost::json jsonlogic)
target_include_directories(
    benchmark1 PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${JSONLOGIC_DIR}/cpp/include
    ${BOOST_DIR}/include
)
