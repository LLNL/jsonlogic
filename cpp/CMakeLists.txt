# Copyright 2024 Lawrence Livermore National Security, LLC and other CLIPPy
# Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: MIT

# May work with older but not yet tested
cmake_minimum_required(VERSION 3.26)

project( jsonlogic
         VERSION 0.2
         DESCRIPTION "A JsonLogic Interpreter for C++"
         LANGUAGES CXX)

include(GNUInstallDirs)

find_package(Boost 1.80 COMPONENTS json REQUIRED)

include_directories( ${Boost_INCLUDE_DIR} )

# jsonlogic target

add_library(jsonlogic SHARED src/logic.cc)

target_compile_options(jsonlogic PUBLIC -Wall -Wextra -pedantic)


target_include_directories(jsonlogic PRIVATE include)
target_link_libraries(jsonlogic LINK_PUBLIC ${Boost_JSON_LIBRARY} )
set_target_properties(jsonlogic PROPERTIES PUBLIC_HEADER include/jsonlogic/logic.hpp)
set_property(TARGET jsonlogic PROPERTY CXX_STANDARD 17)

install(TARGETS jsonlogic LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                             PUBLIC_HEADER DESTINATION "include/jsonlogic")
install(FILES include/jsonlogic/details/ast-core.hpp DESTINATION "include/jsonlogic/details")
install(FILES include/jsonlogic/details/ast-full.hpp DESTINATION "include/jsonlogic/details")

#~ add_executable(examples)

