#pragma once
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <fstream>

// Define the global 3D grid: grid[layer][x][y]
extern std::vector<std::vector<std::vector<int>>> grid;

// Struct for routing point in the grid
struct Point {
    int l, x, y;
    int cost;
    Point(int l, int x, int y, int c);
};

// Utility functions
bool starts_with(const std::string& str, const std::string& prefix);
std::pair<int, int> extract_coords_obstacle(const std::string& line);
std::tuple<int, int, int> parse_pin(const std::string& pin_str);

// Core parsing and routing functions
void fill_nets(std::string s);
void route_net(std::vector<std::vector<std::vector<int>>>& grid);
void readfile(std::string filename);

// Output/Debugging
void print_grid();
void visualize_grid();  // Optional box-style output

extern std::vector<std::vector<std::vector<std::string>>> net_name_grid;
extern std::vector<std::pair<int, int>> obstacle_sequence;
extern std::vector<std::pair<int, int>> step_sequence;
extern std::vector<std::pair<int, int>> pin_sequence;
extern std::vector<std::pair<int, int>> routed_path;
extern std::vector<std::vector<std::pair<int, int>>> all_step_sequences;
extern std::vector<std::vector<std::pair<int, int>>> all_routed_paths;


#endif // FUNCTIONS_H

