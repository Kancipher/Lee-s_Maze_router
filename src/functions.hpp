// functions.hpp
#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <vector>
#include <tuple>
#include <string>

// Global data structures
extern std::vector<std::vector<std::vector<int>>> grid;
extern std::vector<std::vector<std::tuple<int,int,int>>> all_nets;
extern std::vector<std::string> net_names;
extern std::vector<std::vector<std::string>> net_name_grid;
extern std::vector<std::pair<int,int>> obstacle_sequence;
extern std::vector<std::pair<int,int>> step_sequence;
extern std::vector<std::pair<int,int>> pin_sequence;
extern std::vector<std::pair<int,int>> routed_path;  // final routed path only

// Function declarations
void readfile(const std::string& fname);
void route_all_nets();
void print_grid();

#endif // FUNCTIONS_HPP
