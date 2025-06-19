add_executable(testeval src/testeval.cpp)
target_link_libraries(testeval PRIVATE jsonlogic Boost::json Boost::lexical_cast Boost::range)
target_include_directories(testeval PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Get all JSON test files
file(GLOB_RECURSE JSON_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/json/*.json")

# Add individual tests for each JSON file (normal mode)
foreach(json_file ${JSON_TEST_FILES})
    get_filename_component(test_name ${json_file} NAME_WE)
    add_test(NAME "jsonlogic_${test_name}"
             COMMAND testeval "${json_file}"
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties("jsonlogic_${test_name}" PROPERTIES
        LABELS "jsonlogic;normal"
        TIMEOUT 5
    )
endforeach()

# Add individual tests for each JSON file (strict mode)
foreach(json_file ${JSON_TEST_FILES})
    get_filename_component(test_name ${json_file} NAME_WE)
    add_test(NAME "jsonlogic_${test_name}_strict"
             COMMAND testeval -s "${json_file}"
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties("jsonlogic_${test_name}_strict" PROPERTIES
        LABELS "jsonlogic;strict"
        TIMEOUT 5)
endforeach()
