#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "functions.hpp"

namespace py = pybind11;

PYBIND11_MODULE(routing, m) {
    m.def("readfile", &readfile, "Read input file and run routing");

    m.def("get_grid", []() {
        return grid;
    }, "Get the current routing grid");
}
