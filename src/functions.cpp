#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <queue>
#include <tuple>
#include <climits>
#include <algorithm>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

using namespace std;

vector<vector<vector<int>>> grid;
vector<vector<tuple<int, int, int>>> all_nets;
vector<string> net_names;
vector<vector<vector<string>>> net_name_grid;
vector<pair<int,int>> obstacle_sequence;
vector<pair<int,int>> step_sequence;
vector<pair<int,int>> pin_sequence;
vector<pair<int,int>> routed_path;
vector<vector<pair<int,int>>> all_step_sequences;
vector<vector<pair<int,int>>> all_routed_paths;


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

    for (const auto& p : pins) {
        pin_sequence.emplace_back(get<1>(p), get<2>(p));
    }

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

    queue<Point> q;
    q.push(src);
    cost_grid[src.l][src.x][src.y] = 0;

    while (!q.empty()) {
        Point current = q.front(); q.pop();
        step_sequence.emplace_back(current.x, current.y);
        if (current.l == dst.l && current.x == dst.x && current.y == dst.y) break;

        // Layer-dependent move costs
        int horiz_cost = (current.l == 0) ? 1 : 2;
        int vert_cost  = (current.l == 0) ? 2 : 1;

        if (current.x > 0 && cost_grid[current.l][current.x - 1][current.y] == INT_MAX) {
            cost_grid[current.l][current.x - 1][current.y] = current.cost + horiz_cost;
            q.push(Point(current.l, current.x - 1, current.y, current.cost + horiz_cost));
        }
        if (current.x < cost_grid[current.l].size() - 1 && cost_grid[current.l][current.x + 1][current.y] == INT_MAX) {
            cost_grid[current.l][current.x + 1][current.y] = current.cost + horiz_cost;
            q.push(Point(current.l, current.x + 1, current.y, current.cost + horiz_cost));
        }
        if (current.y > 0 && cost_grid[current.l][current.x][current.y - 1] == INT_MAX) {
            cost_grid[current.l][current.x][current.y - 1] = current.cost + vert_cost;
            q.push(Point(current.l, current.x, current.y - 1, current.cost + vert_cost));
        }
        if (current.y < cost_grid[current.l][current.x].size() - 1 && cost_grid[current.l][current.x][current.y + 1] == INT_MAX) {
            cost_grid[current.l][current.x][current.y + 1] = current.cost + vert_cost;
            q.push(Point(current.l, current.x, current.y + 1, current.cost + vert_cost));
        }

        if (current.l == 0 && cost_grid[1][current.x][current.y] == INT_MAX) {
            cost_grid[1][current.x][current.y] = current.cost + 10;
            q.push(Point(1, current.x, current.y, current.cost + 10));
        }
        if (current.l == 1 && cost_grid[0][current.x][current.y] == INT_MAX) {
            cost_grid[0][current.x][current.y] = current.cost + 10;
            q.push(Point(0, current.x, current.y, current.cost + 10));
        }
    }

    Point current = dst;
    routed_path.emplace_back(current.x, current.y);
    while (current.l != src.l || current.x != src.x || current.y != src.y) {
        if (grid[current.l][current.x][current.y] != 1 ) {
            grid[current.l][current.x][current.y] = 2;
            net_name_grid[current.l][current.x][current.y] = net_name;
        }
        int min_cost = INT_MAX;
        Point next = current;

        // Layer-dependent move costs
        int horiz_cost = (current.l == 0) ? 1 : 2;
        int vert_cost  = (current.l == 0) ? 2 : 1;

        if (current.x > 0 && cost_grid[current.l][current.x - 1][current.y] != -1) {
            int cost = cost_grid[current.l][current.x - 1][current.y] + horiz_cost;
            if (cost_grid[current.l][current.x - 1][current.y] < min_cost)
                next = Point(current.l, current.x - 1, current.y, min_cost = cost_grid[current.l][current.x - 1][current.y]);
        }
        if (current.x < cost_grid[current.l].size() - 1 && cost_grid[current.l][current.x + 1][current.y] != -1) {
            int cost = cost_grid[current.l][current.x + 1][current.y] + horiz_cost;
            if (cost_grid[current.l][current.x + 1][current.y] < min_cost)
                next = Point(current.l, current.x + 1, current.y, min_cost = cost_grid[current.l][current.x + 1][current.y]);
        }
        if (current.y > 0 && cost_grid[current.l][current.x][current.y - 1] != -1) {
            int cost = cost_grid[current.l][current.x][current.y - 1] + vert_cost;
            if (cost_grid[current.l][current.x][current.y - 1] < min_cost)
                next = Point(current.l, current.x, current.y - 1, min_cost = cost_grid[current.l][current.x][current.y - 1]);
        }
        if (current.y < cost_grid[current.l][current.x].size() - 1 && cost_grid[current.l][current.x][current.y + 1] != -1) {
            int cost = cost_grid[current.l][current.x][current.y + 1] + vert_cost;
            if (cost_grid[current.l][current.x][current.y + 1] < min_cost)
                next = Point(current.l, current.x, current.y + 1, min_cost = cost_grid[current.l][current.x][current.y + 1]);
        }
        if (current.l == 0 && cost_grid[1][current.x][current.y] != -1 && cost_grid[1][current.x][current.y] < min_cost)
            next = Point(1, current.x, current.y, min_cost = cost_grid[1][current.x][current.y]);
        if (current.l == 1 && cost_grid[0][current.x][current.y] != -1 && cost_grid[0][current.x][current.y] < min_cost)
            next = Point(0, current.x, current.y, min_cost = cost_grid[0][current.x][current.y]);

        if (current.l != next.l) {
            for (int l = 0; l < 2; ++l) {
                grid[l][current.x][current.y] = 3;
                net_name_grid[l][current.x][current.y] = net_name;
            }
        }
        routed_path.emplace_back(next.x, next.y);
        current = next;
    }
    all_step_sequences.push_back(step_sequence);
    all_routed_paths.push_back(routed_path);
    step_sequence.clear();
    routed_path.clear();

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
            grid[layer][x][y] = 1;
        }
    }

    // --- Net ordering heuristic: bounding box, then total length ---
    vector<tuple<int, int, int>> net_order; // (bounding_box_size, total_length, net_index)
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
        // Total net length: sum of Manhattan distances between consecutive pins
        for (size_t j = 1; j < pins.size(); ++j) {
            int dx = abs(get<1>(pins[j]) - get<1>(pins[j-1]));
            int dy = abs(get<2>(pins[j]) - get<2>(pins[j-1]));
            total_length += dx + dy;
        }
        net_order.push_back({bbox, total_length, (int)i});
    }
    // Sort: smaller bbox first, then shorter total length
    sort(net_order.begin(), net_order.end());
    // --- End net ordering heuristic ---

    // Now route each net in the chosen order
    for (const auto& order : net_order) {
        int net_id = get<2>(order);
        const auto& net_pins = all_nets[net_id];
        const string& net_name = net_names[net_id];

        if (net_pins.size() >= 2) {
            for (size_t i = 0; i < net_pins.size() - 1; ++i) {
                auto src = net_pins[i];
                auto dst = net_pins[i + 1];
                int src_layer = get<0>(src) - 1, sx = get<1>(src), sy = get<2>(src);
                int dst_layer = get<0>(dst) - 1, dx = get<1>(dst), dy = get<2>(dst);

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
    for (int layer = 0; layer < 2; ++layer) {
        for (int i = 0; i < grid[layer].size(); ++i) {
            for (int j = 0; j < grid[layer][i].size(); ++j) {
                bool is_via = (grid[0][i][j] == 3) || (grid[1][i][j] == 3);
                if (grid[layer][i][j] != 0 || is_via) {
                    std::string value_str;
                    if (is_via) {
                        value_str = "X";
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
    grid = vector<vector<vector<int>>>(2, vector<vector<int>>(cols, vector<int>(rows, 0)));
    net_name_grid = vector<vector<vector<string>>>(2, vector<vector<string>>(cols, vector<string>(rows, "")));
    while (getline(in, s)) {
        if (starts_with(s, "OBS")) {
            pair<int, int> coords = extract_coords_obstacle(s);
            grid[0][coords.first][coords.second] = -1;
            grid[1][coords.first][coords.second] = -1;
            obstacle_sequence.push_back(coords);
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

PYBIND11_MODULE(routing, m) {
    m.def("readfile",            &readfile);
    m.def("get_grid",            [](){ return grid; });
    m.def("get_net_name_grid",   [](){ return net_name_grid; });
    m.def("get_obstacle_sequence", [](){ return obstacle_sequence; });
    m.def("get_pin_sequence",      [](){ return pin_sequence;     });
    m.def("get_step_sequence",     [](){ return step_sequence;    });
    m.def("get_routed_path",       [](){ return routed_path;      });  // NEW
    m.def("get_all_step_sequences", []() { return all_step_sequences; });
    m.def("get_all_routed_paths",   []() { return all_routed_paths; });
    m.def("get_net_names",          []() { return net_names; });

}
