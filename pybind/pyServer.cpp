#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include "Server.hpp"

namespace py = pybind11;
namespace nl = nlohmann;
using namespace xdbi;

void PYBIND11_INIT_CLASS_SERVER(py::module_ &m)
{
    // NOTE: The 3rd argument is a different default holder. Default is std::unique_ptr but we need std::shared_ptr.
    py::class_<Server>(m, "Server")
        .def(py::init<std::string, std::string, int>(),
             py::arg("dbPath"), py::arg("dbAddress") = "0.0.0.0", py::arg("dbPort") = 8183)
        .def("start", &Server::start)
        .def("stop", &Server::stop)
        .def_readonly("dbAddress", &Server::dbAddress)
        .def_readonly("dbPort", &Server::dbPort)
        .def_readonly("dbPath", &Server::dbPath);
}
