# cmake_minimum_required(VERSION 3.14)

project(jsonlogic_benchmarks)

set(BUILD_TESTING OFF) # Disable Faker tests
FetchContent_Declare(faker
    GIT_REPOSITORY https://github.com/cieslarmichal/faker-cxx.git
    GIT_TAG main
)

FetchContent_MakeAvailable(faker)


add_executable(jsonlogic_benchmark src/benchmark1.cpp)
target_compile_options(jsonlogic_benchmark PRIVATE -O3)
target_include_directories(jsonlogic_benchmark PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(jsonlogic_benchmark PRIVATE
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

target_link_libraries(jsonlogic_benchmark PRIVATE jsonlogic faker-cxx ${CMAKE_DL_LIBS})

add_executable(jl-bench-in src/benchmark-membership.cpp)
target_compile_options(jl-bench-in PRIVATE -O3)
target_include_directories(jl-bench-in PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(jl-bench-in PRIVATE jsonlogic faker-cxx)
