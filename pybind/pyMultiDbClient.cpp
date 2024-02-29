#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include "MultiDbClient.hpp"

namespace py = pybind11;
namespace nl = nlohmann;
using namespace xdbi;

void PYBIND11_INIT_CLASS_MULTIDBCLIENT(py::module_ &m)
{
    // NOTE: The 3rd argument is a different default holder. Default is std::unique_ptr but we need std::shared_ptr.
    py::class_<MultiDbClient, std::shared_ptr<MultiDbClient>>(m, "MultiDbClient")
        .def(py::init<const xtypes::XTypeRegistryPtr, const nl::json &>(),
             py::arg("registry"), py::arg("config"))
        .def("isReady", &MultiDbClient::isReady)

        .def("setWorkingGraph", &MultiDbClient::setWorkingGraph)
        .def("getWorkingGraph", &MultiDbClient::getWorkingGraph)
        .def("getAbsoluteDbPath", &MultiDbClient::getAbsoluteDbPath)
        .def("getAbsoluteDbGraphPath", &MultiDbClient::getAbsoluteDbGraphPath)
        .def("load", &MultiDbClient::load,
             py::arg("uri"), py::arg("classname") = "")
        .def("clear", &MultiDbClient::clear)
        .def("remove", &MultiDbClient::remove,
             py::arg("uri"))
        .def("add", py::overload_cast<std::vector<XTypePtr>, const int>(&MultiDbClient::add),
             py::arg("xtypes"), py::arg("max_depth")=-1)
        .def("add", py::overload_cast<nl::json>(&MultiDbClient::add),
             py::arg("xtypes"))
        .def("update", py::overload_cast<std::vector<XTypePtr>, const int>(&MultiDbClient::update),
             py::arg("xtypes"), py::arg("max_depth")=-1)
        .def("update", py::overload_cast<nl::json>(&MultiDbClient::update),
             py::arg("xtypes"))
        .def("find", &MultiDbClient::find,
             py::arg("classname") = "", py::arg("properties") = nl::json{})
        .def("uris", &MultiDbClient::uris,
             py::arg("classname") = "", py::arg("properties") = nl::json{})
        .def("findAll", &MultiDbClient::findAll,
             py::arg("classname") = "", py::arg("properties") = nl::json{})
        .def("getMainInterface", &MultiDbClient::getMainInterface);
}
