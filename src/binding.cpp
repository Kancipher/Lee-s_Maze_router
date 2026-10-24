#include "functions.hpp"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;
PYBIND11_MODULE(routing, m) {
  m.def("readfile", &readfile);
  m.def("set_costs", &set_costs);
  m.def("get_grid", []() { return grid; });
  m.def("get_net_name_grid", []() { return net_name_grid; });
  m.def("get_results", []() {
    py::list out;
    for (const auto &r : route_results) {
      py::dict d;
      d["name"] = r.name;
      d["routed"] = r.routed;
      d["cost"] = r.cost;
      d["cells"] = r.cells;
      out.append(d);
    }
    return out;
  });
}
