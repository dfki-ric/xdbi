#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>
#include <pybind11/stl.h>
#include<pybind11/stl/filesystem.h>

#include "Serverless.hpp"

namespace py = pybind11;
namespace nl = nlohmann;
using namespace xdbi;

void PYBIND11_INIT_CLASS_SERVERLESS(py::module_ &m)
{
    // NOTE: The 3rd argument is a different default holder. Default is std::unique_ptr but we need std::shared_ptr.
    py::class_<Serverless, std::shared_ptr<Serverless>>(m, "Serverless")
        .def(py::init<const xtypes::XTypeRegistryPtr, const std::string, const std::string>(),
             py::arg("registry"), py::arg("dbPath"), py::arg("graph"))

        .def("setWorkingGraph", &Serverless::setWorkingGraph)
        .def("getWorkingGraph", &Serverless::getWorkingGraph)
        .def("setWorkingDbPath", &Serverless::setWorkingDbPath)
        .def("getWorkingDbPath", &Serverless::getWorkingDbPath)
        .def("getAbsoluteDbPath", &Serverless::getAbsoluteDbPath)
        .def("getAbsoluteDbGraphPath", &Serverless::getAbsoluteDbGraphPath)
        .def("load", &Serverless::load,
             py::arg("uri"), py::arg("classname") = "", py::arg("search_depth") = -1)
        .def("clear", &Serverless::clear)
        .def("remove", &Serverless::remove,
             py::arg("uri"))
        .def("add", py::overload_cast<std::vector<XTypePtr>, const int>(&Serverless::add),
             py::arg("xtypes"), py::arg("depth_limit")=-1)
        .def("add", py::overload_cast<nl::json>(&Serverless::add),
             py::arg("xtypes"))
        .def("update", py::overload_cast<std::vector<XTypePtr>, const int>(&Serverless::update),
             py::arg("xtypes"), py::arg("depth_limit")=-1)
        .def("update", py::overload_cast<nl::json>(&Serverless::update),
             py::arg("xtypes"))
        .def("find", &Serverless::find,
             py::arg("classname") = "", py::arg("properties") = nl::json{}, py::arg("search_depth") = -1)
        .def("uris", &Serverless::uris,
             py::arg("classname") = "", py::arg("properties") = nl::json{});
 }
