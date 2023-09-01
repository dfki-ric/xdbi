#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>
#include <pybind11_json/pybind11_json.hpp>

#if __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #include <boost/filesystem.hpp>
    namespace fs = boost::filesystem;
#endif

#include "DbInterface.hpp"

namespace py = pybind11;
using namespace xdbi;

void PYBIND11_INIT_CLASS_SERVER(py::module_ &);
void PYBIND11_INIT_CLASS_CLIENT(py::module_ &);
void PYBIND11_INIT_CLASS_SERVERLESS(py::module_ &);
void PYBIND11_INIT_CLASS_JSONDATABASEBACKEND(py::module_ &);
void PYBIND11_INIT_CLASS_MULTIDBCLIENT(py::module_ &);

PYBIND11_MODULE(xdbi_py, m)
 {
    PYBIND11_INIT_CLASS_SERVER(m);
    PYBIND11_INIT_CLASS_CLIENT(m);
    PYBIND11_INIT_CLASS_SERVERLESS(m);
    PYBIND11_INIT_CLASS_JSONDATABASEBACKEND(m);
    PYBIND11_INIT_CLASS_MULTIDBCLIENT(m);

    m.def("db_interface_from_config", &DbInterface::from_config,
          py::arg("registry"), py::arg("config"), py::arg("read_only") = true);
    m.def("get_available_backends", &DbInterface::get_available_backends);
    m.def("set_logger_level", &DbInterface::setLoggerLevel,
     py::arg("level"), py::arg("verbose")=false);
    m.def("getDefaultConfig", &DbInterface::getDefaultConfig);

}
