cmake_minimum_required(VERSION 3.14)

project(jl_tests)

# Determine the top-level build directory assuming standard build layout


# Add the benchmark executable(s)
add_executable(testeval src/testeval.cpp)
target_link_libraries(testeval PRIVATE jsonlogic Boost::json Boost::lexical_cast Boost::range)
target_include_directories(testeval PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)