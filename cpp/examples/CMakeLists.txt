cmake_minimum_required(VERSION 3.14)

project(jsonlogic_examples)
include(FetchContent) 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Determine the top-level build directory assuming standard build layout
get_filename_component(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
message(STATUS "Root directory: ${ROOT_DIR}")
set(Boost_LIBRARY_DIR "${ROOT_DIR}/build/_deps/boost-build")
set(Boost_SOURCE_DIR "${ROOT_DIR}/build/_deps/boost-src")
file(GLOB Boost_INCLUDE_DIRS "${Boost_SOURCE_DIR}/libs/*/include")

# Add the benchmark executable(s)
add_executable(testeval
    src/testeval.cc  # Replace with your actual benchmark source(s)
)


# Link with jsonlogic and Boost
target_link_libraries(testeval PRIVATE
    jsonlogic
    )


target_link_directories(testeval PRIVATE
    ${Boost_LIBRARY_DIR}
    # ${Boost_LIBRARY_DIR}/libs/json
    ${ROOT_DIR}/build
)


# Ensure Boost include directories are available
target_include_directories(testeval PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    # ${Boost_INCLUDE_DIR}/libs/json/include
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_SOURCE_DIR}
    ${ROOT_DIR}/include
)
