# XDBI

This repository contains the database interface and backends to store, search and retrieve e.g. [XTypes](https://github.com/dfki-ric/xtypes).

XDBI was initiated and is currently developed at the
[Robotics Innovation Center](http://robotik.dfki-bremen.de/en/startpage.html) of the
[German Research Center for Artificial Intelligence (DFKI)](http://www.dfki.de) in Bremen,
together with the [Robotics Group](http://www.informatik.uni-bremen.de/robotik/index_en.php)
of the [University of Bremen](http://www.uni-bremen.de/en.html).

XDBI provides the database interface, for all types generated with the [xtypes_generator](https://github.com/dfki-ric/xtypes_generator).

## Installation
### Dependencies
- xtypes_generator
- CMake >= 3.10
- libspdlog
- cpr >= 1.3.0
- nlohmann_json >= 3.10.5
- pybind11 >=2.9.1
- pybind11_json >=0.2.12

### Manual
1. Clone this repository
2. Build and install via:
  ```bash
  cd xdbi
  mkdir build
  cd build
  cmake ..
  make install
  ```

### Autoproj
1. Add this repository to you autoproj setup.
2. Then run in your autoproj workspace:
  ```bash
  aup xdbi
  amake xdbi
  ```

## Testing
```bash
cd build
make cpp_test # for testing the C++ interface
make py_test # for testing the python interface
```


## Tools
XDBI comes with some generic database tools.

### jsondb
The most important one is `jsondb` it is used to start a HTTP/REST server providing access to the local database instance specified via  --db_path.
By default this server is reachable via "http://0.0.0.0:8183", but of course you can specify your own IP and port.

See also the output of `jsondb -h` for further information.

#### Merging JSON databases

If GIT is used to track and publish changes in a common database,
the standard line-based merge will create fake conflicts and is not able to correctly merge JSON files.
To overcome this issue there is a tool called `xdbi-json-merge` which can perform a 3-way merge on JSON files.

To use it, you have to edit the `.git/config` file and add the following section:

```
[merge "json"]
    name = Merge JSON files
    driver = xdbi-json-merge %O %A %B
```

Furthermore, you have to create a `.git/info/attributes` file (if not present) and add

```
* merge=text
/*/** merge=json
```

So on the top level (where README.md and stuff like that is) the standard merge will be used,
but in the subfolders named by Xtype classnames the JSON merger will be used.

### xdbi-clear & xdbi-remove
These tools are used to clear a complete database graph/directory (`xdbi-clear`) or to remove one specific XType instance specified via the passed URIs from the database.

See also the output of `xdbi-clear -h` or `xdbi-remove -h` for further information.

### Usage as CPP Library
You can use xdbi in you project by including and linking against the xdbi library.

For CMake use `find_package(xdbi REQUIRED)` in you CMakeLists.txt and link against `xdbi::xdbi_cpp` in you `target_link_libraries` call.

Please see the API documentation on how to use it in your code.

### Usage as Python module
XDBI comes with python bindings. Basically, you can use everything you'll find in the CPP-API-documentation in the same way in python.
Just `import xdbi_py` and you are ready to go.

You can have a look into bin/xdbi-clear.py, bin/xdbi-remove.py or into the unit tests in the test directory for examples.
