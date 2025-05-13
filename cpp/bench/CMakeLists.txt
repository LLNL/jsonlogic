# cmake_minimum_required(VERSION 3.14)

project(jsonlogic_benchmarks)

set(BUILD_TESTING OFF) # Disable Faker tests
FetchContent_Declare(faker
    GIT_REPOSITORY https://github.com/cieslarmichal/faker-cxx.git
    GIT_TAG main
)

FetchContent_MakeAvailable(faker)


add_executable(jsonlogic_benchmark src/benchmark1.cpp)
target_include_directories(jsonlogic_benchmark PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(jsonlogic_benchmark PRIVATE jsonlogic faker-cxx)


add_executable(jl-bench-in src/benchmark-membership.cpp)
target_include_directories(jl-bench-in PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(jl-bench-in PRIVATE jsonlogic faker-cxx)


