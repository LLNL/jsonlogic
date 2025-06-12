add_executable(testeval src/testeval.cpp)
target_link_libraries(testeval PRIVATE jsonlogic Boost::json Boost::lexical_cast Boost::range)
target_include_directories(testeval PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)