#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <queue>
#include <tuple>
#include <climits>
#include <algorithm>
using namespace std;

vector<vector<vector<int>>> grid;
vector<vector<tuple<int, int, int>>> all_nets; 
vector<string> net_names; 
vector<vector<vector<string>>> net_name_grid; 
// Add global variable to store path order
vector<vector<tuple<int, int, int>>> net_paths;  // For each net, store its path in order

// Add global variables and setter for user-supplied costs
int g_via_cost = 10;
int g_nonpref_cost = 10;
void set_costs(int via, int nonpref) {
    g_via_cost = via;
    g_nonpref_cost = nonpref;
}

struct Point {
    int l,x, y;
    int cost;
    Point(int l, int x, int y, int c) : l(l), x(x), y(y), cost(c) {}
};

bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}



pair<int, int> extract_coords_obstacle(const string& line) {
    size_t start = line.find('(');
    size_t comma = line.find(',', start);
    size_t end = line.find(')', comma);
    int x = stoi(line.substr(start + 1, comma - start - 1));
    int y = stoi(line.substr(comma + 1, end - comma - 1));
    return { x, y };
}

tuple<int, int, int> parse_pin(const string& pin_str) {
    size_t start = pin_str.find('(');
    size_t end = pin_str.find(')');
    string coords = pin_str.substr(start + 1, end - start - 1);
    stringstream ss(coords);
    int layer, x, y;
    char comma;
    ss >> layer >> comma >> x >> comma >> y;
    return { layer, x, y };
}

void fill_nets(string s) {
    size_t paren_pos = s.find('(');
    string net_name = s.substr(0, paren_pos);
    net_name.erase(net_name.find_last_not_of(" \t\r\n") + 1);
    std::cout << "Net name: " << net_name << std::endl;
    net_names.push_back(net_name); 

    stringstream ss(s.substr(s.find('(')));
    string pin_str;
    vector<tuple<int, int, int>> pins;

    char c;
    while (ss >> c) {
        if (c == '(') {
            pin_str = "(";
            while (ss >> c && c != ')') {
                pin_str += c;
            }
            if (c == ')') {
                pin_str += ')';
                pins.push_back(parse_pin(pin_str));
            }
        }
    }

    int rows = grid[0].size();
    int cols = grid[0][0].size();
    auto edge_dist = [rows, cols](const tuple<int, int, int>& pin) {
        int x = get<1>(pin);
        int y = get<2>(pin);
        return min({x, y, rows - 1 - x, cols - 1 - y});
    };
    auto start_it = min_element(pins.begin(), pins.end(), [&](const auto& a, const auto& b) {
        return edge_dist(a) < edge_dist(b);
    });
    if (start_it != pins.begin()) {
        iter_swap(pins.begin(), start_it);
    }
    int start_layer = get<0>(pins[0]);
    vector<tuple<int, int, int>> same_layer, other_layer;
    for (size_t i = 1; i < pins.size(); ++i) {
        if (get<0>(pins[i]) == start_layer)
            same_layer.push_back(pins[i]);
        else
            other_layer.push_back(pins[i]);
    }
    auto greedy_sort = [](const tuple<int, int, int>& start, vector<tuple<int, int, int>> group) {
        vector<tuple<int, int, int>> result;
        tuple<int, int, int> current = start;
        while (!group.empty()) {
            auto nearest_it = min_element(group.begin(), group.end(), [&](const auto& a, const auto& b) {
                int da = abs(get<1>(a) - get<1>(current)) + abs(get<2>(a) - get<2>(current));
                int db = abs(get<1>(b) - get<1>(current)) + abs(get<2>(b) - get<2>(current));
                return da < db;
            });
            result.push_back(*nearest_it);
            current = *nearest_it;
            group.erase(nearest_it);
        }
        return result;
    };
    vector<tuple<int, int, int>> sorted_pins = {pins[0]};
    auto sorted_same_layer = greedy_sort(pins[0], same_layer);
    sorted_pins.insert(sorted_pins.end(), sorted_same_layer.begin(), sorted_same_layer.end());
    if (!other_layer.empty()) {
        auto sorted_other_layer = greedy_sort(sorted_pins.back(), other_layer);
        sorted_pins.insert(sorted_pins.end(), sorted_other_layer.begin(), sorted_other_layer.end());
    }
    pins = sorted_pins;

    all_nets.push_back(pins); 
}

void route_net(vector<vector<vector<int>>>& grid, Point src, Point dst, const string& net_name) {
    vector<vector<vector<int>>> cost_grid = grid;
    for (int l = 0; l < 2; ++l) {
        for (int x = 0; x < cost_grid[l].size(); ++x) {
            for (int y = 0; y < cost_grid[l][x].size(); ++y) {
                if (grid[l][x][y] == 0) {
                    cost_grid[l][x][y] = INT_MAX;
                }
                else {
                    cost_grid[l][x][y] = -1;
                }
            }
        }
    }

    cost_grid[src.l][src.x][src.y] = INT_MAX;
    cost_grid[dst.l][dst.x][dst.y] = INT_MAX;

    // Define comparison for priority queue
    auto compare = [](const Point& a, const Point& b) {
        return a.cost > b.cost;  // Min heap
    };
    priority_queue<Point, vector<Point>, decltype(compare)> pq(compare);
    
    // Initialize source
    cost_grid[src.l][src.x][src.y] = 0;
    pq.push(Point(src.l, src.x, src.y, 0));

    while (!pq.empty()) {
        Point current = pq.top(); pq.pop();
        
        // Skip if we've found a better path to this node
        if (current.cost > cost_grid[current.l][current.x][current.y]) {
            continue;
        }

        if (current.l == dst.l && current.x == dst.x && current.y == dst.y) break;

        // Layer-dependent move costs
        int horiz_cost = (current.l == 0) ? 1 : g_nonpref_cost;
        int vert_cost  = (current.l == 0) ? g_nonpref_cost : 1;

        // Check all possible moves
        vector<Point> moves = {
            Point(current.l, current.x - 1, current.y, current.cost + horiz_cost),  // left
            Point(current.l, current.x + 1, current.y, current.cost + horiz_cost),  // right
            Point(current.l, current.x, current.y - 1, current.cost + vert_cost),   // up
            Point(current.l, current.x, current.y + 1, current.cost + vert_cost),   // down
            Point(1 - current.l, current.x, current.y, current.cost + g_via_cost)   // layer change
        };

        for (const auto& next : moves) {
            // Skip if out of bounds
            if (next.x < 0 || next.x >= cost_grid[next.l].size() || 
                next.y < 0 || next.y >= cost_grid[next.l][next.x].size()) {
                continue;
            }

            // Skip if blocked
            if (cost_grid[next.l][next.x][next.y] == -1) {
                continue;
            }

            // If we found a better path
            if (next.cost < cost_grid[next.l][next.x][next.y]) {
                cost_grid[next.l][next.x][next.y] = next.cost;
                pq.push(next);
            }
        }
    }

    // Track the path in order
    vector<tuple<int, int, int>> path;
    Point current = dst;
    while (current.l != src.l || current.x != src.x || current.y != src.y) {
        if (grid[current.l][current.x][current.y] != 1) {
            grid[current.l][current.x][current.y] = 2;
            net_name_grid[current.l][current.x][current.y] = net_name;
            path.push_back({current.l + 1, current.x, current.y});  // Store path in order
        }
        bool found = false;
        int horiz_cost = (current.l == 0) ? 1 : g_nonpref_cost;
        int vert_cost  = (current.l == 0) ? g_nonpref_cost : 1;
        // Check all possible moves and follow the one that matches the cost difference
        // Left
        if (!found && current.x > 0 && cost_grid[current.l][current.x - 1][current.y] != -1) {
            if (cost_grid[current.l][current.x - 1][current.y] + horiz_cost == cost_grid[current.l][current.x][current.y]) {
                current = Point(current.l, current.x - 1, current.y, cost_grid[current.l][current.x - 1][current.y]);
                found = true;
            }
        }
        // Right
        if (!found && current.x < cost_grid[current.l].size() - 1 && cost_grid[current.l][current.x + 1][current.y] != -1) {
            if (cost_grid[current.l][current.x + 1][current.y] + horiz_cost == cost_grid[current.l][current.x][current.y]) {
                current = Point(current.l, current.x + 1, current.y, cost_grid[current.l][current.x + 1][current.y]);
                found = true;
            }
        }
        // Up
        if (!found && current.y > 0 && cost_grid[current.l][current.x][current.y - 1] != -1) {
            if (cost_grid[current.l][current.x][current.y - 1] + vert_cost == cost_grid[current.l][current.x][current.y]) {
                current = Point(current.l, current.x, current.y - 1, cost_grid[current.l][current.x][current.y - 1]);
                found = true;
            }
        }
        // Down
        if (!found && current.y < cost_grid[current.l][current.x].size() - 1 && cost_grid[current.l][current.x][current.y + 1] != -1) {
            if (cost_grid[current.l][current.x][current.y + 1] + vert_cost == cost_grid[current.l][current.x][current.y]) {
                current = Point(current.l, current.x, current.y + 1, cost_grid[current.l][current.x][current.y + 1]);
                found = true;
            }
        }
        // Via (layer change)
        int other_layer = 1 - current.l;
        if (!found && cost_grid[other_layer][current.x][current.y] != -1) {
            if (cost_grid[other_layer][current.x][current.y] + g_via_cost == cost_grid[current.l][current.x][current.y]) {
                // Mark via on both layers
                for (int l = 0; l < 2; ++l) {
                    grid[l][current.x][current.y] = 3;
                    net_name_grid[l][current.x][current.y] = net_name;
                }
                path.push_back({current.l + 1, current.x, current.y});  // Store current layer
                path.push_back({other_layer + 1, current.x, current.y});  // Store via layer
                current = Point(other_layer, current.x, current.y, cost_grid[other_layer][current.x][current.y]);
                found = true;
            }
        }
        // If no move found (should not happen), break to avoid infinite loop
        if (!found) break;
    }
    // Store the complete path for this net
    net_paths.push_back(path);
}

void route_all_nets() {
    // Initialize net_name_grid for both layers
    net_name_grid = vector<vector<vector<string>>>(2, vector<vector<string>>(grid[0].size(), vector<string>(grid[0][0].size(), "")));
    // Clear previous paths
    net_paths.clear();

    // First, place all pins from all nets
    for (const auto& net_pins : all_nets) {
        for (const auto& pin : net_pins) {
            int layer = get<0>(pin) - 1;
            int x = get<1>(pin);
            int y = get<2>(pin);
            grid[layer][x][y] = 1;
        }
    }

    // --- Net ordering heuristic: bounding box, then total length ---
    vector<tuple<int, int, int, int>> net_order; // (bounding_box_size, pins_in_bbox, total_length, net_index)
    for (size_t i = 0; i < all_nets.size(); ++i) {
        const auto& pins = all_nets[i];
        int min_x = INT_MAX, max_x = INT_MIN, min_y = INT_MAX, max_y = INT_MIN;
        int total_length = 0;
        for (const auto& pin : pins) {
            int x = get<1>(pin);
            int y = get<2>(pin);
            min_x = min(min_x, x);
            max_x = max(max_x, x);
            min_y = min(min_y, y);
            max_y = max(max_y, y);
        }
        // Bounding box size: (width + height)
        int bbox = (max_x - min_x) + (max_y - min_y);
        
        // Count pins inside the bounding box
        int pins_in_bbox = 0;
        for (const auto& pin : pins) {
            int x = get<1>(pin);
            int y = get<2>(pin);
            if (x >= min_x && x <= max_x && y >= min_y && y <= max_y) {
                pins_in_bbox++;
            }
        }
        
        // Total net length: sum of Manhattan distances between consecutive pins
        for (size_t j = 1; j < pins.size(); ++j) {
            int dx = abs(get<1>(pins[j]) - get<1>(pins[j-1]));
            int dy = abs(get<2>(pins[j]) - get<2>(pins[j-1]));
            total_length += dx + dy;
        }
        net_order.push_back({bbox, pins_in_bbox, total_length, (int)i});
    }
    // Sort: smaller bbox first, then fewer pins in bbox, then shorter total length
    sort(net_order.begin(), net_order.end());
    // --- End net ordering heuristic ---

    // Now route each net in the chosen order
    int routing_order = 0;  // Track the order in which nets are routed
    for (const auto& order : net_order) {
        int net_id = get<3>(order);
        const auto& net_pins = all_nets[net_id];
        const string& net_name = net_names[net_id];
        routing_order++;  // Increment the routing order counter

        if (net_pins.size() >= 2) {
            for (size_t i = 0; i < net_pins.size() - 1; ++i) {
                auto src = net_pins[i];
                auto dst = net_pins[i + 1];
                int src_layer = get<0>(src) - 1, sx = get<1>(src), sy = get<2>(src);
                int dst_layer = get<0>(dst) - 1, dx = get<1>(dst), dy = get<2>(dst);

                if (grid[src_layer][sx][sy] != -1 && grid[dst_layer][dx][dy] != -1) {
                    Point src_point(src_layer, sx, sy, 0);
                    Point dst_point(dst_layer, dx, dy, 0);
                    // Pass routing order instead of net name
                    route_net(grid, src_point, dst_point, to_string(routing_order));
                } else {
                    cout << "Skipping net segment due to obstacle at pin location.\n";
                }
            }
        }
    }
}

void print_grid(string output_filename) {
    ofstream out(output_filename);
    
    // Output each net's path in the order it was routed
    for (size_t net_idx = 0; net_idx < net_names.size(); ++net_idx) {
        const string& net_name = net_names[net_idx];
        if (net_idx < net_paths.size() && !net_paths[net_idx].empty()) {
            out << net_name;
            for (const auto& cell : net_paths[net_idx]) {
                out << " (" << get<0>(cell) << "," << get<1>(cell) << "," << get<2>(cell) << ")";
            }
            out << endl;
        }
    }
    out.close();
}

void readfile(string input_filename, string output_filename) {
    ifstream in(input_filename);
    string s;

    string size;
    getline(in, size);
    size_t xpos = size.find('x');
    int rows = stoi(size.substr(0, xpos));
    int cols = stoi(size.substr(xpos + 1));
    grid = vector<vector<vector<int>>>(2, vector<vector<int>>(cols, vector<int>(rows, 0)));
    net_name_grid = vector<vector<vector<string>>>(2, vector<vector<string>>(cols, vector<string>(rows, "")));
    while (getline(in, s)) {
        if (starts_with(s, "OBS")) {
            pair<int, int> coords = extract_coords_obstacle(s);
            grid[0][coords.first][coords.second] = -1;
            grid[1][coords.first][coords.second] = -1;  
            cout << s << endl;
        }
        else if (starts_with(s, "net")) {
            fill_nets(s);
        }
    }
    route_all_nets();
    print_grid(output_filename);
    in.close();
}

