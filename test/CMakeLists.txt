cmake_minimum_required(VERSION 3.5)

# First, we create the test types used in the unit tests
add_subdirectory(xdbi_test_type)

if(APPLE)
  set(LIB_SUFFIX dylib)
else(APPLE)
  set(LIB_SUFFIX so)
endif()
add_executable(xdbi_test
  unit_tests.cpp
)
add_dependencies(xdbi_test xdbi_test_type_cpp)
target_compile_features(xdbi_test PUBLIC cxx_std_17)
target_include_directories(xdbi_test
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(xdbi_test PRIVATE
  xdbi_cpp
  xdbi_test_type_cpp
)
if(Catch_FOUND)
  catch_discover_tests(xdbi_test)
endif()

### Set up test environment
set(TEST_DB_PATH "${CMAKE_CURRENT_BINARY_DIR}/temp/test_db")
file(MAKE_DIRECTORY ${TEST_DB_PATH})
set(TEST_DB_PATH2 "${CMAKE_CURRENT_BINARY_DIR}/temp/test_db2")
file(MAKE_DIRECTORY ${TEST_DB_PATH2})
set(TEST_MULTIDB_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/temp/MultiDbClient_config.json")
configure_file(${CMAKE_SOURCE_DIR}/examples/MultiDbClient_config.json ${TEST_MULTIDB_CONFIG})
### CPP Test ###
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test.sh.in ${CMAKE_CURRENT_BINARY_DIR}/test.sh @ONLY)
add_custom_target(cpp_test
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test.sh
  DEPENDS xdbi_test
)

### PYTHON Test ###
if("$ENV{PYTHON}" STREQUAL "")
  set(PYTHON "python")
  message(STATUS "Using default python.")
else()
  set(PYTHON $ENV{PYTHON})
  message(STATUS ${PYTHON})
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/py_test.sh.in ${CMAKE_CURRENT_BINARY_DIR}/py_test.sh @ONLY)
add_custom_target(py_test
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/py_test.sh
  DEPENDS xdbi_py
)
