#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
using namespace std;
vector<vector<vector<int>>> grid;

bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() &&
           str.compare(0, prefix.size(), prefix) == 0;
}

pair<int, int> extract_coords_obstacle(const string& line) {
    size_t start = line.find('(');
    size_t comma = line.find(',', start);
    size_t end = line.find(')', comma);
    int x = stoi(line.substr(start + 1, comma - start - 1));
    int y = stoi(line.substr(comma + 1, end - comma - 1));

    return {x, y};
}

tuple<int, int, int> parse_pin(const string& pin_str) {
    size_t start = pin_str.find('(');
    size_t end = pin_str.find(')');
    string coords = pin_str.substr(start + 1, end - start - 1);
    
    stringstream ss(coords);
    int layer, x, y;
    char comma; 
    ss >> layer >> comma >> x >> comma >> y;
    return {layer, x, y};
}

void fill_nets(string s) {
    size_t net_pos = s.find("net");
    string net_name = s.substr(net_pos + 4, s.find('(') - net_pos - 4);

    stringstream ss(s.substr(s.find('(')));
    string pin_str;

}
void route_net(vector<vector<vector<int>>> grid)
{

}
void print_grid() {
    for (int layer = 0; layer < grid.size(); ++layer) {
        for (int i = 0; i < grid[layer].size(); ++i) {
            for (int j = 0; j < grid[layer][i].size(); ++j) {
                if (grid[layer][i][j] != 0) { 
                    cout << "Layer " << (layer + 1) << " - Cell (" << i << ", " << j << ") = " << grid[layer][i][j] << endl;
                }
            }
        }
    }
}

void readfile(string filename)
{
    ifstream in;
    in.open(filename);
    string s;

    //grid size
    string size;
    getline(in,size);
    size_t xpos = size.find('x');
    int rows = std::stoi(size.substr(0, xpos));
    int cols = std::stoi(size.substr(xpos + 1));
    grid = vector<vector<vector<int>>>(2, vector<vector<int>>(rows, vector<int>(cols, 0)));

    //routing
    while (getline(in, s))
    {
        if(starts_with(s,"OBS"))
        {
            pair<int,int> coords = extract_coords_obstacle(s);
            grid[0][coords.first][coords.second]=-1;
            grid[1][coords.first][coords.second]=-1;
            cout << s << endl;
        }
        else if(starts_with(s,"net"))
        {
            fill_nets(s);
            route_net(grid);

        }
    }
    print_grid();
    in.close();
}