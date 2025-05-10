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

struct Point {
    int x, y;
    int cost;
    Point(int x, int y, int c) : x(x), y(y), cost(c) {}
};

// Utility Functions
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
    size_t net_pos = s.find("net");
    string net_name = s.substr(net_pos + 4, s.find('(') - net_pos - 4);

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
    all_nets.push_back(pins); // Just store pins for later routing
}

void route_net(vector<vector<vector<int>>>& grid) {
    vector<Point> pins;
    for (int i = 0; i < grid[0].size(); ++i) {
        for (int j = 0; j < grid[0][i].size(); ++j) {
            if (grid[0][i][j] == 1) {
                pins.push_back(Point(i, j, 0));
            }
        }
    }
    if (pins.size() < 2) return;

    for (size_t i = 0; i < pins.size() - 1; ++i) {
        vector<vector<int>> cost_grid = grid[0];
        for (int x = 0; x < cost_grid.size(); ++x) {
            for (int y = 0; y < cost_grid[x].size(); ++y) {
                if (grid[0][x][y] == 0 || grid[0][x][y] == 1) {
                    cost_grid[x][y] = INT_MAX;
                }
                else {
                    cost_grid[x][y] = -1; // Block obstacles and other paths
                }
            }
        }

        queue<Point> q;
        q.push(pins[i]);
        cost_grid[pins[i].x][pins[i].y] = 0;

        while (!q.empty()) {
            Point current = q.front(); q.pop();
            if (current.x == pins[i + 1].x && current.y == pins[i + 1].y) break;

            if (current.x > 0 && cost_grid[current.x - 1][current.y] == INT_MAX) {
                cost_grid[current.x - 1][current.y] = current.cost + 1;
                q.push(Point(current.x - 1, current.y, current.cost + 1));
            }
            if (current.x < cost_grid.size() - 1 && cost_grid[current.x + 1][current.y] == INT_MAX) {
                cost_grid[current.x + 1][current.y] = current.cost + 1;
                q.push(Point(current.x + 1, current.y, current.cost + 1));
            }
            if (current.y > 0 && cost_grid[current.x][current.y - 1] == INT_MAX) {
                cost_grid[current.x][current.y - 1] = current.cost + 2;
                q.push(Point(current.x, current.y - 1, current.cost + 2));
            }
            if (current.y < cost_grid[0].size() - 1 && cost_grid[current.x][current.y + 1] == INT_MAX) {
                cost_grid[current.x][current.y + 1] = current.cost + 2;
                q.push(Point(current.x, current.y + 1, current.cost + 2));
            }
        }

        Point current = pins[i + 1];
        while (current.x != pins[i].x || current.y != pins[i].y) {
            grid[0][current.x][current.y] = 2;
            int min_cost = INT_MAX;
            Point next = current;

            if (current.x > 0 && cost_grid[current.x - 1][current.y] != -1 && cost_grid[current.x - 1][current.y] < min_cost)
                next = Point(current.x - 1, current.y, min_cost = cost_grid[current.x - 1][current.y]);
            if (current.x < cost_grid.size() - 1 && cost_grid[current.x + 1][current.y] != -1 && cost_grid[current.x + 1][current.y] < min_cost)
                next = Point(current.x + 1, current.y, min_cost = cost_grid[current.x + 1][current.y]);
            if (current.y > 0 && cost_grid[current.x][current.y - 1] != -1 && cost_grid[current.x][current.y - 1] < min_cost)
                next = Point(current.x, current.y - 1, min_cost = cost_grid[current.x][current.y - 1]);
            if (current.y < cost_grid[0].size() - 1 && cost_grid[current.x][current.y + 1] != -1 && cost_grid[current.x][current.y + 1] < min_cost)
                next = Point(current.x, current.y + 1, min_cost = cost_grid[current.x][current.y + 1]);

            current = next;
        }
    }
}

void route_all_nets() {
    vector<tuple<int, int, int>> all_pins; // Store all start and end pins

    for (const auto& net_pins : all_nets) {
        // Clear previous pin markers
        for (int x = 0; x < grid[0].size(); ++x) {
            for (int y = 0; y < grid[0][0].size(); ++y) {
                if (grid[0][x][y] == 1) grid[0][x][y] = 0;
            }
        }

        if (net_pins.size() >= 2) {
            auto src = net_pins.front();
            auto dst = net_pins.back();
            int src_layer = get<0>(src) - 1, sx = get<1>(src), sy = get<2>(src);
            int dst_layer = get<0>(dst) - 1, dx = get<1>(dst), dy = get<2>(dst);

            if (grid[src_layer][sx][sy] != -1 && grid[dst_layer][dx][dy] != -1) {
                grid[src_layer][sx][sy] = 1;
                grid[dst_layer][dx][dy] = 1;
                all_pins.push_back(src);
                all_pins.push_back(dst);
                route_net(grid);
            }
            else {
                cout << "Skipping net due to obstacle at pin location.\n";
            }

            all_pins.push_back(src);
            all_pins.push_back(dst);
        }

        route_net(grid);
    }

    for (const auto& pin : all_pins) {
        int layer = get<0>(pin) - 1;
        int x = get<1>(pin);
        int y = get<2>(pin);
        grid[layer][x][y] = 1;
    }
}

void print_grid() {
    int layer = 0; // Only print Layer 1
    for (int i = 0; i < grid[layer].size(); ++i) {
        for (int j = 0; j < grid[layer][i].size(); ++j) {
            if (grid[layer][i][j] != 0) {
                cout << "Layer 1 - Cell (" << i << ", " << j << ") = " << grid[layer][i][j] << endl;
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
    grid = vector<vector<vector<int>>>(2, vector<vector<int>>(rows, vector<int>(cols, 0)));

    while (getline(in, s)) {
        if (starts_with(s, "OBS")) {
            pair<int, int> coords = extract_coords_obstacle(s);
            grid[0][coords.first][coords.second] = -1;
            // grid[1][coords.first][coords.second] = -1; // Temporarily disabled
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
