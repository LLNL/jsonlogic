cmake_minimum_required(VERSION 3.14)

project(jl_tests)

# Determine the top-level build directory assuming standard build layout

message(STATUS "boost_SOURCE_DIR: ${boost_SOURCE_DIR}")
# Add the benchmark executable(s)
add_executable(testeval src/testeval.cpp)
target_link_libraries(testeval PRIVATE jsonlogic Boost::json Boost::range Boost:boost)
target_include_directories(testeval PRIVATE
#  ${boost_SOURCE_DIR}
#     ${PROJECT_SOURCE_DIR}/include
#     ${CMAKE_SOURCE_DIR}/include
#     ${Boost_INCLUDE_DIRS}
    ${boost_SOURCE_DIR}
)