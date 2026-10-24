#include "functions.hpp"
#include <iostream>
#include <string>
int main(int argc, char **argv) {
  try {
    if (argc != 3 && argc != 5) {
      std::cerr
          << "Usage: maze-router INPUT OUTPUT [VIA_COST NONPREFERRED_COST]\n";
      return 1;
    }
    auto number = [](const std::string &s) {
      std::size_t end = 0;
      int v = std::stoi(s, &end);
      if (end != s.size())
        throw std::invalid_argument("Invalid cost");
      return v;
    };
    if (argc == 5)
      set_costs(number(argv[3]), number(argv[4]));
    readfile(argv[1], argv[2]);
    bool ok = true;
    for (const auto &r : route_results) {
      std::cout << r.name << ": " << (r.routed ? "ROUTED" : "UNROUTABLE")
                << " cost=" << r.cost << " cells=" << r.cells.size() << "\n";
      ok = ok && r.routed;
    }
    return ok ? 0 : 2;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
