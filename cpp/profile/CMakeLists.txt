# cmake_minimum_required(VERSION 3.14)

# project(jl-bench-eqs)

set(BUILD_TESTING OFF) # Disable Faker tests
FetchContent_Declare(faker
    GIT_REPOSITORY https://github.com/cieslarmichal/faker-cxx.git
    GIT_TAG main
)

FetchContent_MakeAvailable(faker)

add_executable(profile-jl-eq src/profile-jl-eq.cpp)
# target_compile_options(-O3 -pg -Wall -Wextra -Wunused-variable)

message(STATUS, "profile: CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")

# target_compile_options(jl-bench-eq PRIVATE -O3)
target_compile_options(profile-jl-eq PRIVATE -O3 -pg -Wall -Wextra -Wunused-variable)
target_include_directories(profile-jl-eq SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# target_include_directories(jl-bench-eq SYSTEM PRIVATE
#     ${CMAKE_BINARY_DIR}/_deps/faker-src/include
# )


target_link_libraries(profile-jl-eq PRIVATE jsonlogic faker-cxx  ${CMAKE_DL_LIBS})

# add_executable(jl-bench-membership src/benchmark-membership.cpp)
# target_compile_options(jl-bench-membership PRIVATE -O3)
# target_include_directories(jl-bench-membership SYSTEM PRIVATE
#     ${CMAKE_SOURCE_DIR}/bench/include
#     ${CMAKE_SOURCE_DIR}/include
# )

# target_include_directories(jl-bench-membership SYSTEM PRIVATE
#     ${CMAKE_BINARY_DIR}/_deps/faker-src/include
# )

# target_link_libraries(jl-bench-membership PRIVATE jsonlogic faker-cxx)
