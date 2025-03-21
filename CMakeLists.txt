cmake_minimum_required(VERSION 3.10)

project(xdbi VERSION 2.0.0 DESCRIPTION "Database interface to the XTypes Database")

if(APPLE)
add_definitions(-std=c++17)
MACRO(CMAKE_USE_FULL_RPATH install_rpath)
    SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    SET(CMAKE_INSTALL_RPATH ${install_rpath})
    SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
ENDMACRO(CMAKE_USE_FULL_RPATH)
CMAKE_USE_FULL_RPATH("${CMAKE_INSTALL_PREFIX}/lib")
endif()

include(${CMAKE_SOURCE_DIR}/cmake/xdbi-dependencies.cmake)

# Export the library interface
install(EXPORT xdbi-targets
	NAMESPACE xdbi::
	DESTINATION lib/cmake/xdbi
)
# Create and install the version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file("xdbi-config-version.cmake"
	VERSION ${xdbi_VERSION}
	COMPATIBILITY SameMajorVersion
)
configure_file(${CMAKE_SOURCE_DIR}/cmake/xdbi-config.cmake.in  ${CMAKE_BINARY_DIR}/cmake/xdbi-config.cmake @ONLY)
install(
	FILES
  ${CMAKE_BINARY_DIR}/cmake/xdbi-config.cmake
  ${CMAKE_SOURCE_DIR}/cmake/xdbi-dependencies.cmake
	${PROJECT_BINARY_DIR}/xdbi-config-version.cmake
	DESTINATION
		lib/cmake/xdbi
)

# ##############
# C++ Library #
# ##############

####GLOB is not recommended by CMake.#####
add_library(xdbi_cpp SHARED
	src/Client.cpp
	src/DbInterface.cpp
	src/FilesystemBasedBackend.cpp
	src/FilesystemBasedLock.cpp
  src/JsonDatabaseBackend.cpp
	src/MultiDbClient.cpp
  src/Server.cpp
  src/Serverless.cpp
)
target_compile_features(xdbi_cpp PUBLIC cxx_std_17)

target_include_directories(xdbi_cpp
  PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external>
  $<INSTALL_INTERFACE:include>
)

if(NOT APPLE)
  set(ADDITIONAL_DEPS stdc++fs) #for 18.04
endif()

target_link_libraries(xdbi_cpp PUBLIC
  pthread
  nlohmann_json::nlohmann_json
  PkgConfig::cpr
  xtypes_generator::xtypes_generator_cpp
  Boost::system
  spdlog::spdlog #for 18.04
  ${ADDITIONAL_DEPS}
)

#install external
# Install crow
install(DIRECTORY external/crow
	DESTINATION include/xdbi
	FILES_MATCHING PATTERN "*.h"
)
# Install cxxopts
install(DIRECTORY external/cxxopts
	DESTINATION include/xdbi
	FILES_MATCHING PATTERN "*.hpp"
)
# Install the binaries
install(TARGETS xdbi_cpp EXPORT xdbi-targets
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)
# Install header files
set(XDBI_HEADERS
    include/Backend.hpp
    include/Client.hpp
    include/DbInterface.hpp
    include/FilesystemBasedBackend.hpp
    include/FilesystemBasedLock.hpp
    include/JsonDatabaseBackend.hpp
    include/Logger.hpp
    include/MultiDbClient.hpp
    include/Server.hpp
    include/Serverless.hpp
    include/JsonMerge.hpp
    )

    install(FILES ${XDBI_HEADERS} DESTINATION include/xdbi)

# Install pkg-config file
configure_file(xdbi.pc.in ${CMAKE_BINARY_DIR}/xdbi.pc @ONLY)
install(
	FILES ${CMAKE_BINARY_DIR}/xdbi.pc
	DESTINATION lib/pkgconfig/
)

# ###############
# Executables #
# #############
FILE(GLOB EXECUTABLE_SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/bin/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/bin/*.py
)

foreach(EXECUTABLE_SOURCE ${EXECUTABLE_SOURCE_FILES})
  string(REGEX REPLACE ".*/bin/" "" EXECUTABLE_NAME ${EXECUTABLE_SOURCE})

  if(${EXECUTABLE_SOURCE} MATCHES ".cpp$")
    string(REPLACE ".cpp" "" EXECUTABLE_NAME ${EXECUTABLE_NAME})
    message(STATUS "Building & installing executable ${EXECUTABLE_NAME} from ${EXECUTABLE_SOURCE}")
    add_executable(${EXECUTABLE_NAME} ${EXECUTABLE_SOURCE})

    target_include_directories(${EXECUTABLE_NAME}
      PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/external>
    )
    target_link_libraries(${EXECUTABLE_NAME} PUBLIC
      pthread
      xdbi_cpp
      xtypes_generator::xtypes_generator_cpp
    )
    install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)
  elseif(${EXECUTABLE_SOURCE} MATCHES ".py$")
    string(REPLACE ".py" "" EXECUTABLE_NAME ${EXECUTABLE_NAME})
    message(STATUS "Installing python executable ${EXECUTABLE_NAME} from ${EXECUTABLE_SOURCE}")
    configure_file(${EXECUTABLE_SOURCE} ${CMAKE_BINARY_DIR}/${EXECUTABLE_NAME} COPYONLY)
    install(FILES ${CMAKE_BINARY_DIR}/${EXECUTABLE_NAME} DESTINATION bin/ PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ WORLD_READ)
  endif()
endforeach()

# ##########################
# PYBIND11 Python Binding #
# ##########################
if("$ENV{PYTHON}" STREQUAL "")
  set(PYTHON_EXECUTABLE "python")
  message(STATUS "Using default python.")
else()
  set(PYTHON_EXECUTABLE $ENV{PYTHON})
  message(STATUS ${PYTHON_EXECUTABLE})
endif()
execute_process(
  COMMAND ${PYTHON_EXECUTABLE} -c "import sys; print(str(sys.version_info.major)+'.'+str(sys.version_info.minor))"
  OUTPUT_VARIABLE PYBIND11_PYTHON_VERSION
)
string(STRIP ${PYBIND11_PYTHON_VERSION} PYBIND11_PYTHON_VERSION)
message(STATUS "Python version: ${PYBIND11_PYTHON_VERSION}")

FILE(GLOB PYBIND_SOURCES
  ${CMAKE_SOURCE_DIR}/pybind/*.cpp
)
pybind11_add_module(xdbi_py ${PYBIND_SOURCES})
target_include_directories(xdbi_py
  PUBLIC
  ${PYTHON_INCLUDE_DIRS}
)

target_link_libraries(xdbi_py PUBLIC
  xdbi_cpp
  ${PYTHON_LIBRARIES}
  nlohmann_json::nlohmann_json
  pybind11::pybind11_headers
  pybind11_json
  xtypes_generator::xtypes_generator_py
  xtypes_generator::xtypes_generator_cpp
)
add_dependencies(xdbi_py xdbi_cpp)

install(TARGETS xdbi_py COMPONENT python
LIBRARY DESTINATION lib/python${PYBIND11_PYTHON_VERSION}/site-packages)

# #########
# # Tests #
# #########
add_subdirectory(test EXCLUDE_FROM_ALL)

# documentation
configure_file(${CMAKE_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_BINARY_DIR}/doc/Doxyfile @ONLY)
add_custom_target(doc
  COMMAND doxygen ${CMAKE_BINARY_DIR}/doc/Doxyfile
)
