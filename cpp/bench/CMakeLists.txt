cmake_minimum_required(VERSION 3.14)

project(jsonlogic_benchmarks)
include(FetchContent) 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Determine the top-level build directory assuming standard build layout
get_filename_component(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
message(STATUS "Root directory: ${ROOT_DIR}")
set(Boost_LIBRARY_DIR "${ROOT_DIR}/build/_deps/boost-build")
set(Boost_SOURCE_DIR "${ROOT_DIR}/build/_deps/boost-src")
file(GLOB Boost_INCLUDE_DIRS "${Boost_SOURCE_DIR}/libs/*/include")

# Add the benchmark executable(s)
add_executable(jsonlogic_benchmark
    src/benchmark1.cpp  # Replace with your actual benchmark source(s)
)

set(BUILD_TESTING OFF) # Disable Faker tests
FetchContent_Declare(faker
    GIT_REPOSITORY https://github.com/cieslarmichal/faker-cxx.git
    GIT_TAG main
)

FetchContent_MakeAvailable(faker)


# Link with jsonlogic and Boost
target_link_libraries(jsonlogic_benchmark PRIVATE
    jsonlogic
    faker-cxx
    )


# include_directories(${BOOST_INCLUDE_DIRS})

# message(STATUS "faker_SOURCE_DIR: ${faker_SOURCE_DIR}")
# message(STATUS "PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
# message(STATUS "Boost_INCLUDE_DIR: ${Boost_INCLUDE_DIR}")
# message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")

target_link_directories(jsonlogic_benchmark PRIVATE
    ${Boost_LIBRARY_DIR}
    # ${Boost_LIBRARY_DIR}/libs/json
    ${ROOT_DIR}/build
)
# Ensure Boost include directories are available
target_include_directories(jsonlogic_benchmark PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    # ${Boost_INCLUDE_DIR}/libs/json/include
    ${faker_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_SOURCE_DIR}
    ${ROOT_DIR}/include
)
