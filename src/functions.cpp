#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <queue>
#include <tuple>
#include <climits>
using namespace std;

// Global grid and net storage
vector<vector<vector<int>>> grid;
vector<vector<tuple<int, int, int>>> all_nets; // Store pins of all nets
vector<string> net_names; // Store net names in order
vector<vector<vector<string>>> net_name_grid; // Parallel to grid, stores net name for each routed cell per layer

struct Point {
    int l,x, y;
    int cost;
    Point(int l, int x, int y, int c) : l(l), x(x), y(y), cost(c) {}
};

// Utility Functions
bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

// Calculate Manhattan distance from (0,0) to a point
int manhattan_distance(int x, int y) {
    return x + y;
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
// Remove trailing spaces
net_name.erase(net_name.find_last_not_of(" \t\r\n") + 1);
std::cout << "Net name: " << net_name << std::endl;
    net_names.push_back(net_name); // Track net name order

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
        sort(pins.begin(), pins.end(), [](const tuple<int, int, int>& a, const tuple<int, int, int>& b) {
        int dist_a = manhattan_distance(get<1>(a), get<2>(a));
        int dist_b = manhattan_distance(get<1>(b), get<2>(b));
        return dist_a < dist_b;
    });

    all_nets.push_back(pins); 

}

void route_net(vector<vector<vector<int>>>& grid, Point src, Point dst, const string& net_name) {
    // Prepare cost grid
    vector<vector<vector<int>>> cost_grid = grid;
    for (int l = 0; l < 2; ++l) {
        for (int x = 0; x < cost_grid[l].size(); ++x) {
            for (int y = 0; y < cost_grid[l][x].size(); ++y) {
                if (grid[l][x][y] == 0) {
                    cost_grid[l][x][y] = INT_MAX;
                }
                else {
                    cost_grid[l][x][y] = -1; // Block obstacles, pins, and other paths
                }
            }
        }
    }

    // Mark source and destination as valid for routing
    cost_grid[src.l][src.x][src.y] = INT_MAX;
    cost_grid[dst.l][dst.x][dst.y] = INT_MAX;

    queue<Point> q;
    q.push(src);
    cost_grid[src.l][src.x][src.y] = 0;

    while (!q.empty()) {
        Point current = q.front(); q.pop();
        if (current.l == dst.l && current.x == dst.x && current.y == dst.y) break;

        // Check horizontal and vertical moves in current layer
        if (current.x > 0 && cost_grid[current.l][current.x - 1][current.y] == INT_MAX) {
            cost_grid[current.l][current.x - 1][current.y] = current.cost + 1;
            q.push(Point(current.l, current.x - 1, current.y, current.cost + 1));
        }
        if (current.x < cost_grid[current.l].size() - 1 && cost_grid[current.l][current.x + 1][current.y] == INT_MAX) {
            cost_grid[current.l][current.x + 1][current.y] = current.cost + 1;
            q.push(Point(current.l, current.x + 1, current.y, current.cost + 1));
        }
        if (current.y > 0 && cost_grid[current.l][current.x][current.y - 1] == INT_MAX) {
            cost_grid[current.l][current.x][current.y - 1] = current.cost + 2;
            q.push(Point(current.l, current.x, current.y - 1, current.cost + 2));
        }
        if (current.y < cost_grid[current.l][current.x].size() - 1 && cost_grid[current.l][current.x][current.y + 1] == INT_MAX) {
            cost_grid[current.l][current.x][current.y + 1] = current.cost + 2;
            q.push(Point(current.l, current.x, current.y + 1, current.cost + 2));
        }

        // Check layer transitions
        // Move up from layer 0 to layer 1
        if (current.l == 0 && cost_grid[1][current.x][current.y] == INT_MAX) {
            cost_grid[1][current.x][current.y] = current.cost + 10;
            q.push(Point(1, current.x, current.y, current.cost + 10));
        }
        // Move down from layer 1 to layer 0
        if (current.l == 1 && cost_grid[0][current.x][current.y] == INT_MAX) {
            cost_grid[0][current.x][current.y] = current.cost + 10;
            q.push(Point(0, current.x, current.y, current.cost + 10));
        }
    }

    Point current = dst;
    while (current.l != src.l || current.x != src.x || current.y != src.y) {
        // Only mark as route if it's not a pin
        if (grid[current.l][current.x][current.y] != 1) {
            grid[current.l][current.x][current.y] = 2;
            net_name_grid[current.l][current.x][current.y] = net_name;
        }
        int min_cost = INT_MAX;
        Point next = current;

        // Check all possible moves including layer transitions
        if (current.x > 0 && cost_grid[current.l][current.x - 1][current.y] != -1 && cost_grid[current.l][current.x - 1][current.y] < min_cost)
            next = Point(current.l, current.x - 1, current.y, min_cost = cost_grid[current.l][current.x - 1][current.y]);
        if (current.x < cost_grid[current.l].size() - 1 && cost_grid[current.l][current.x + 1][current.y] != -1 && cost_grid[current.l][current.x + 1][current.y] < min_cost)
            next = Point(current.l, current.x + 1, current.y, min_cost = cost_grid[current.l][current.x + 1][current.y]);
        if (current.y > 0 && cost_grid[current.l][current.x][current.y - 1] != -1 && cost_grid[current.l][current.x][current.y - 1] < min_cost)
            next = Point(current.l, current.x, current.y - 1, min_cost = cost_grid[current.l][current.x][current.y - 1]);
        if (current.y < cost_grid[current.l][current.x].size() - 1 && cost_grid[current.l][current.x][current.y + 1] != -1 && cost_grid[current.l][current.x][current.y + 1] < min_cost)
            next = Point(current.l, current.x, current.y + 1, min_cost = cost_grid[current.l][current.x][current.y + 1]);
        
        // Check layer transitions in backtracking
        if (current.l == 0 && cost_grid[1][current.x][current.y] != -1 && cost_grid[1][current.x][current.y] < min_cost)
            next = Point(1, current.x, current.y, min_cost = cost_grid[1][current.x][current.y]);
        if (current.l == 1 && cost_grid[0][current.x][current.y] != -1 && cost_grid[0][current.x][current.y] < min_cost)
            next = Point(0, current.x, current.y, min_cost = cost_grid[0][current.x][current.y]);

        // If we're changing layers, mark both layers with a via
        if (current.l != next.l) {
            // Mark via in both layers at this (x, y)
            for (int l = 0; l < 2; ++l) {
                grid[l][current.x][current.y] = 3;
                net_name_grid[l][current.x][current.y] = net_name;
            }
        }

        current = next;
    }
}

void route_all_nets() {
    // Initialize net_name_grid for both layers
    net_name_grid = vector<vector<vector<string>>>(2, vector<vector<string>>(grid[0].size(), vector<string>(grid[0][0].size(), "")));

    // First, place all pins from all nets
    for (const auto& net_pins : all_nets) {
        for (const auto& pin : net_pins) {
            int layer = get<0>(pin) - 1;
            int x = get<1>(pin);
            int y = get<2>(pin);
            grid[layer][x][y] = 1;  // Mark as pin
        }
    }

    // Now route each net
    for (size_t net_id = 0; net_id < all_nets.size(); ++net_id) {
        const auto& net_pins = all_nets[net_id];
        const string& net_name = net_names[net_id];

        if (net_pins.size() >= 2) {
            for (size_t i = 0; i < net_pins.size() - 1; ++i) {
                auto src = net_pins[i];
                auto dst = net_pins[i + 1];
                int src_layer = get<0>(src) - 1, sx = get<1>(src), sy = get<2>(src);
                int dst_layer = get<0>(dst) - 1, dx = get<1>(dst), dy = get<2>(dst);

                // Only route if both pins are not obstacles
                if (grid[src_layer][sx][sy] != -1 && grid[dst_layer][dx][dy] != -1) {
                    Point src_point(src_layer, sx, sy, 0);
                    Point dst_point(dst_layer, dx, dy, 0);
                    route_net(grid, src_point, dst_point, net_name);
                } else {
                    cout << "Skipping net segment due to obstacle at pin location.\n";
                }
            }
        }
    }
}

void print_grid() {
    // Print both layers
    for (int layer = 0; layer < 2; ++layer) {
        for (int i = 0; i < grid[layer].size(); ++i) {
            for (int j = 0; j < grid[layer][i].size(); ++j) {
                // Check if this cell is a via in either layer
                bool is_via = (grid[0][i][j] == 3) || (grid[1][i][j] == 3);
                if (grid[layer][i][j] != 0 || is_via) {
                    std::string value_str;
                    if (is_via) {
                        value_str = "X";  // Display X for vias
                    } else {
                        value_str = std::to_string(grid[layer][i][j]);
                    }
                    cout << "Layer" << layer + 1 << " - Cell (" << i << ", " << j << ") = " << value_str << endl;
                }
            }
        }
    }
}

void readfile(string filename) {
    ifstream in(filename);
    string s;

    string size;
    getline(in, size);
    size_t xpos = size.find('x');
    int rows = stoi(size.substr(0, xpos));
    int cols = stoi(size.substr(xpos + 1));
    // Initialize grid with correct dimensions: 2 layers, rows x cols
    grid = vector<vector<vector<int>>>(2, vector<vector<int>>(cols, vector<int>(rows, 0)));
    net_name_grid = vector<vector<vector<string>>>(2, vector<vector<string>>(cols, vector<string>(rows, "")));

    while (getline(in, s)) {
        if (starts_with(s, "OBS")) {
            pair<int, int> coords = extract_coords_obstacle(s);
            grid[0][coords.first][coords.second] = -1;
            grid[1][coords.first][coords.second] = -1;  // Enable obstacles on both layers
            cout << s << endl;
        }
        else if (starts_with(s, "net")) {
            fill_nets(s);
        }
    }

    route_all_nets();
    print_grid();
    in.close();
}
