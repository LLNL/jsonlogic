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
    add_custom_target(bench
        DEPENDS jl-bench-eq jl-bench-membership jl-bench-generic
    )
endif()
if(JSONLOGIC_ENABLE_TESTS)
    message(STATUS "Building tests: ${JSONLOGIC_ENABLE_TESTS}")
    include(CTest)
    
    # Enable parallel testing by default
    include(ProcessorCount)
    ProcessorCount(N)
    if(NOT N EQUAL 0)
        set(CTEST_BUILD_FLAGS -j${N})
        set(ctest_test_args ${ctest_test_args} PARALLEL_LEVEL ${N})
    endif()
    
    add_subdirectory(tests)
endif()

# Create symlink to compile_commands.json for clangd
set(COMPILE_COMMANDS_SOURCE "${CMAKE_BINARY_DIR}/compile_commands.json")
set(COMPILE_COMMANDS_LINK "${CMAKE_SOURCE_DIR}/build/compile_commands.json")

# Ensure the build directory exists
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

# Create or update the symlink
if(EXISTS "${COMPILE_COMMANDS_SOURCE}")
    # Remove existing symlink/file if it exists
    if(EXISTS "${COMPILE_COMMANDS_LINK}")
        file(REMOVE "${COMPILE_COMMANDS_LINK}")
    endif()
    
    # Create new symlink
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink 
                "${COMPILE_COMMANDS_SOURCE}"
                "${COMPILE_COMMANDS_LINK}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/build"
        RESULT_VARIABLE symlink_result
    )
    
    if(symlink_result EQUAL 0)
        message(STATUS "Created symlink: ${COMPILE_COMMANDS_LINK} -> ${COMPILE_COMMANDS_SOURCE}")
    else()
        message(WARNING "Failed to create compile_commands.json symlink")
    endif()
endif()
