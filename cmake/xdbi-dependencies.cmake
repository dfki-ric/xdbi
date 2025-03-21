# Find all dependencies
find_package(PkgConfig REQUIRED)
find_package(xtypes_generator REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
pkg_check_modules(cpr REQUIRED IMPORTED_TARGET cpr)
if(APPLE)
  pkg_check_modules(nlohmann_json REQUIRED IMPORTED_TARGET nlohmann_json)
  find_package(fmt REQUIRED) # todo: check if this is fine for linux as well
else(APPLE)
  find_package(nlohmann_json 3.10.5 REQUIRED)
  set(NLOHMANN_TARGET nlohmann_json::nlohmann_json)
endif(APPLE)

# ##########################
# PYBIND11 Python Binding #
# ##########################
if("$ENV{PYTHON}" STREQUAL "")
  set(PYTHON "python")
  message(STATUS "Using default python.")
else()
  set(PYTHON $ENV{PYTHON})
  message(STATUS ${PYTHON})
endif()
find_package(PythonLibs REQUIRED)
find_package(pybind11 REQUIRED)
find_package(pybind11_json REQUIRED)
