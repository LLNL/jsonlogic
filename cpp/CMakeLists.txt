# Copyright 2024 Lawrence Livermore National Security, LLC and other CLIPPy
# Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: MIT

# May work with older but not yet tested
cmake_minimum_required(VERSION 3.26)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project( jsonlogic
         VERSION 0.2
         DESCRIPTION "A JsonLogic Interpreter for C++"
         LANGUAGES CXX)

include(GNUInstallDirs)
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG=1")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -ggdb -march=native")

# Download and build Boost::json
set(BOOST_URL
    "https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-cmake.tar.gz"
    CACHE STRING "URL to fetch Boost tarball")


set(BOOST_INCLUDE_LIBRARIES json lexical_cast range)
set(BUILD_SHARED_LIBS ON)
FetchContent_Declare(
    Boost
    URL ${BOOST_URL})
FetchContent_MakeAvailable(Boost)


option(JSONLOGIC_ENABLE_BENCH "Build the benchmark code" ON)
option(JSONLOGIC_ENABLE_TESTS "Build the test code" ON)


add_library(jsonlogic SHARED ${CMAKE_CURRENT_SOURCE_DIR}/src/logic.cc)
target_include_directories(jsonlogic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(jsonlogic PUBLIC -Wall -Wextra -pedantic)

target_link_libraries(jsonlogic LINK_PUBLIC Boost::json)

set_target_properties(jsonlogic PROPERTIES PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/jsonlogic/logic.hpp)
set_target_properties(jsonlogic PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_property(TARGET jsonlogic PROPERTY CXX_STANDARD 20)

install(TARGETS jsonlogic LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                             PUBLIC_HEADER DESTINATION "include/jsonlogic")
install(FILES include/jsonlogic/details/ast-core.hpp DESTINATION "include/jsonlogic/details")
install(FILES include/jsonlogic/details/ast-full.hpp DESTINATION "include/jsonlogic/details")

if(JSONLOGIC_ENABLE_BENCH)
    message(STATUS "Building benchmarks: ${JSONLOGIC_ENABLE_BENCH}")
    add_subdirectory(bench)
endif()
if(JSONLOGIC_ENABLE_TESTS)
    message(STATUS "Building tests: ${JSONLOGIC_ENABLE_TESTS}")
    enable_testing()
    add_subdirectory(tests)
endif()
