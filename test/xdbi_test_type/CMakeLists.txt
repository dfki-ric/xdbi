cmake_minimum_required(VERSION 3.10)
project(xdbi_test_type VERSION 0.1 DESCRIPTION "Test XTypes for XDBI")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(xtypes_generator REQUIRED)
xtypes_project()

### If you provide your package information via cmake, feel free to use the following lines
# configure_file(${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}-config.cmake @ONLY)
# install(FILES ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}-config.cmake DESTINATION lib/cmake/${PROJECT_NAME})
