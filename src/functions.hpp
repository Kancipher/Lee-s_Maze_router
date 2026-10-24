#pragma once
#include <cstdint>
#include <string>
#include <tuple>
#include <vector>
using Grid = std::vector<std::vector<std::vector<int>>>;
using Names = std::vector<std::vector<std::vector<std::string>>>;
using Cell = std::tuple<int, int, int>;
struct RouteResult {
  std::string name;
  bool routed;
  std::int64_t cost;
  std::vector<Cell> cells;
};
extern Grid grid;
extern Names net_name_grid;
extern std::vector<RouteResult> route_results;
void set_costs(int via, int nonpreferred);
void readfile(std::string input, std::string output);
