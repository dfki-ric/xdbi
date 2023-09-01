#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include "Client.hpp"

namespace py = pybind11;
namespace nl = nlohmann;
using namespace xdbi;

void PYBIND11_INIT_CLASS_CLIENT(py::module_ &m)
{
    // NOTE: The 3rd argument is a different default holder. Default is std::unique_ptr but we need std::shared_ptr.
    py::class_<Client, std::shared_ptr<Client>>(m, "Client")
        .def(py::init<const xtypes::XTypeRegistryPtr, const std::string, const std::string>(),
             py::arg("registry"), py::arg("dbAddress"), py::arg("graph"))
        .def("isReady", &Client::isReady)
        .def("setDbUser", &Client::setDbUser,
             py::arg("dbUser"))
        .def("setDbPassword", &Client::setDbPassword,
             py::arg("dbPassword"))

        .def("setWorkingGraph", &Client::setWorkingGraph)
        .def("getWorkingGraph", &Client::getWorkingGraph)
        .def("setDbAddress", &Client::setDbAddress)
        .def("getAbsoluteDbPath", &Client::getAbsoluteDbPath)
        .def("getAbsoluteDbGraphPath", &Client::getAbsoluteDbGraphPath)
        .def("load", &Client::load,
             py::arg("uri"),  py::arg("classname") = "", py::arg("search_depth") = -1)
        .def("clear", &Client::clear)
        .def("remove", &Client::remove,
             py::arg("uri"))
        .def("add", py::overload_cast<std::vector<XTypePtr>, const int>(&Client::add),
             py::arg("xtypes"), py::arg("depth_limit")=-1)
        .def("add", py::overload_cast<nl::json>(&Client::add),
             py::arg("xtypes"))
        .def("update", py::overload_cast<std::vector<XTypePtr>, const int>(&Client::update),
             py::arg("xtypes"), py::arg("depth_limit")=-1)
        .def("update", py::overload_cast<nl::json>(&Client::update),
             py::arg("xtypes"))
        .def("find", &Client::find,
             py::arg("classname") = "", py::arg("properties") = nl::json{}, py::arg("search_depth") = -1)
        .def("uris", &Client::uris,
             py::arg("classname") = "", py::arg("properties") = nl::json{});
}
