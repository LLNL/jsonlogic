# cmake_minimum_required(VERSION 3.14)

# project(jl-bench-eqs)

# set(CMAKE_BUILD_TYPE Debug)
include(FetchContent)
set(BUILD_TESTING OFF) # Disable Faker tests
FetchContent_Declare(faker
    GIT_REPOSITORY https://github.com/cieslarmichal/faker-cxx.git
    GIT_TAG v4.1.0
)

FetchContent_MakeAvailable(faker)
FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG        v3.3.1
)
FetchContent_MakeAvailable(cxxopts)

add_executable(jl-bench-eq src/benchmark-equality.cpp)
add_compile_options(-Wall -Wextra -Wunused-variable)

message(STATUS, "bench: CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")

target_compile_options(jl-bench-eq PRIVATE -O3)
target_include_directories(jl-bench-eq SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../bench/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# target_include_directories(jl-bench-eq SYSTEM PRIVATE
#     ${CMAKE_BINARY_DIR}/_deps/faker-src/include
# )

target_compile_definitions(jl-bench-eq PRIVATE
"RUNTIME_INCLUDES=\"-I ${CMAKE_SOURCE_DIR}/include \
-I ${boost_align_SOURCE_DIR}/include \
-I ${boost_align_SOURCE_DIR}/include \
-I ${boost_assert_SOURCE_DIR}/include \
-I ${boost_config_SOURCE_DIR}/include \
-I ${boost_container_SOURCE_DIR}/include \
-I ${boost_container_hash_SOURCE_DIR}/include \
-I ${boost_core_SOURCE_DIR}/include \
-I ${boost_describe_SOURCE_DIR}/include \
-I ${boost_endian_SOURCE_DIR}/include \
-I ${boost_intrusive_SOURCE_DIR}/include \
-I ${boost_json_SOURCE_DIR}/include \
-I ${boost_move_SOURCE_DIR}/include \
-I ${boost_mp11_SOURCE_DIR}/include \
-I ${boost_predef_SOURCE_DIR}/include \
-I ${boost_static_assert_SOURCE_DIR}/include \
-I ${boost_system_SOURCE_DIR}/include \
-I ${boost_throw_exception_SOURCE_DIR}/include \
-I ${boost_variant2_SOURCE_DIR}/include \
-I ${boost_winapi_SOURCE_DIR}/include\" "
)

target_link_libraries(jl-bench-eq PRIVATE jsonlogic faker-cxx ${CMAKE_DL_LIBS})

add_executable(jl-bench-membership src/benchmark-membership.cpp)
target_compile_options(jl-bench-membership PRIVATE -O3)
target_include_directories(jl-bench-membership SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../bench/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(jl-bench-membership PRIVATE jsonlogic faker-cxx)

add_executable(jl-bench-generic src/benchmark-generic.cpp)
target_include_directories(jl-bench-generic SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../bench/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(jl-bench-generic PRIVATE faker-cxx jsonlogic cxxopts)
target_compile_features(jl-bench-generic PRIVATE cxx_std_20)
target_compile_options(jl-bench-generic PRIVATE -O3)

# Copy all .json files from bench/src to the build directory's bench folder
file(GLOB BENCH_JSON_FILES "${CMAKE_SOURCE_DIR}/bench/src/*.json")
foreach(jsonfile ${BENCH_JSON_FILES})
    get_filename_component(jsonfile_name "${jsonfile}" NAME)
    configure_file(${jsonfile} ${CMAKE_BINARY_DIR}/bench/${jsonfile_name} COPYONLY)
endforeach()
