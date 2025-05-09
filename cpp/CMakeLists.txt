# Copyright 2024 Lawrence Livermore National Security, LLC and other CLIPPy
# Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: MIT

# May work with older but not yet tested
cmake_minimum_required(VERSION 3.26)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project( jsonlogic
         VERSION 0.2
         DESCRIPTION "A JsonLogic Interpreter for C++"
         LANGUAGES CXX)

include(GNUInstallDirs)
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)


# Download and build Boost::json
set(BOOST_URL 
    "https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-cmake.tar.gz" 
    CACHE STRING "URL to fetch Boost tarball")


set(BOOST_INCLUDE_LIBRARIES json)
set(BUILD_SHARED_LIBS ON)
FetchContent_Declare(
    Boost 
    URL ${BOOST_URL})
FetchContent_MakeAvailable(Boost)


add_library(jsonlogic SHARED src/logic.cc)
target_link_libraries(jsonlogic LINK_PUBLIC Boost::json)
target_include_directories(jsonlogic PRIVATE include)
target_compile_options(jsonlogic PUBLIC -Wall -Wextra -pedantic)


set_target_properties(jsonlogic PROPERTIES PUBLIC_HEADER include/jsonlogic/logic.hpp)
set_target_properties(jsonlogic PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_property(TARGET jsonlogic PROPERTY CXX_STANDARD 20)

install(TARGETS jsonlogic LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                             PUBLIC_HEADER DESTINATION "include/jsonlogic")
install(FILES include/jsonlogic/details/ast-core.hpp DESTINATION "include/jsonlogic/details")
install(FILES include/jsonlogic/details/ast-full.hpp DESTINATION "include/jsonlogic/details")
