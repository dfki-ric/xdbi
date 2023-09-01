#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>
#include <pybind11/stl.h>
#include<pybind11/stl/filesystem.h>

#include "JsonDatabaseBackend.hpp"

namespace py = pybind11;
namespace nl = nlohmann;
using namespace xdbi;

void PYBIND11_INIT_CLASS_JSONDATABASEBACKEND(py::module_ &m)
{
    // NOTE: The 3rd argument is a different default holder. Default is std::unique_ptr but we need std::shared_ptr.
    py::class_<JsonDatabaseBackend>(m, "JsonDatabaseBackend")
        .def(py::init<std::string>(),
             py::arg("dbPath"))
        .def("add", py::overload_cast<const nl::json&>(&JsonDatabaseBackend::add),
             py::arg("xtypes"))
        .def("update", py::overload_cast<const nl::json&>(&JsonDatabaseBackend::update),
             py::arg("xtypes"))
      .def("find", py::overload_cast<const std::string&, const nl::json&>(&JsonDatabaseBackend::find),
           py::arg("classname"), py::arg("properties"))
      .def("remove", py::overload_cast<const std::string&>(&JsonDatabaseBackend::remove),
           py::arg("uri"))
      .def("clear", &JsonDatabaseBackend::clear)
      .def("load", py::overload_cast<const std::string&, const std::string&>(&JsonDatabaseBackend::load),
           py::arg("uri"), py::arg("classname"))
      .def("setWorkingGraph", py::overload_cast<const std::string&>(&JsonDatabaseBackend::setWorkingGraph),
           py::arg("graph"))
      .def("dumps", py::overload_cast<const nl::json&>(&JsonDatabaseBackend::dumps),
           py::arg("data"));
}
