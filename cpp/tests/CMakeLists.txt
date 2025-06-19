add_executable(testeval src/testeval.cpp)
target_link_libraries(testeval PRIVATE jsonlogic Boost::json Boost::lexical_cast Boost::range)
target_include_directories(testeval PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# Create a fixture setup test that builds testeval if needed
add_test(NAME "build_testeval" 
         COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target testeval)
set_tests_properties("build_testeval" PROPERTIES
    LABELS "setup"
    FIXTURES_SETUP testeval_fixture)

# Get all JSON test files
file(GLOB_RECURSE JSON_TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/json/*.json")

# Add individual tests for each JSON file (normal mode)
foreach(json_file ${JSON_TEST_FILES})
    get_filename_component(test_name ${json_file} NAME_WE)
    add_test(NAME "jsonlogic_${test_name}"
             COMMAND bash -c "cat '${json_file}' | $<TARGET_FILE:testeval>"
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties("jsonlogic_${test_name}" PROPERTIES
        LABELS "jsonlogic;normal"
        TIMEOUT 5
        FIXTURES_REQUIRED testeval_fixture)
endforeach()

# Add individual tests for each JSON file (strict mode)
foreach(json_file ${JSON_TEST_FILES})
    get_filename_component(test_name ${json_file} NAME_WE)
    add_test(NAME "jsonlogic_${test_name}_strict"
             COMMAND bash -c "cat '${json_file}' | $<TARGET_FILE:testeval> -s"
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties("jsonlogic_${test_name}_strict" PROPERTIES
        LABELS "jsonlogic;strict"
        TIMEOUT 5
        FIXTURES_REQUIRED testeval_fixture)
endforeach()

# Custom target to run tests with clean summary (excludes setup test)
add_custom_target(test_clean
    COMMAND ${CMAKE_CTEST_COMMAND} --exclude-regex "^_setup_"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests with clean summary (excluding setup tests)")

# Custom target to run only jsonlogic tests  
add_custom_target(test_jsonlogic
    COMMAND ${CMAKE_CTEST_COMMAND} -L jsonlogic --exclude-regex "^_setup_"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running only jsonlogic tests")
