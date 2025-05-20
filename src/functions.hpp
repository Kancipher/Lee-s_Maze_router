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
    int x, y;
    int cost;
    Point(int x, int y, int c);
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

void set_costs(int via, int nonpref);

#endif // FUNCTIONS_H


