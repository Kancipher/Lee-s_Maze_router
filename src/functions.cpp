// Modified functions.cpp
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

vector<vector<vector<int>>>       grid;
vector<vector<tuple<int,int,int>>> all_nets;
vector<string>                    net_names;
vector<vector<string>>            net_name_grid;
vector<pair<int,int>>             obstacle_sequence;
vector<pair<int,int>>             step_sequence;
vector<pair<int,int>>             pin_sequence;
vector<pair<int,int>>             routed_path;  // NEW: holds only final routed path
vector<vector<pair<int,int>>> all_step_sequences;   // each net's BFS+trace
vector<vector<pair<int,int>>> all_routed_paths;     // each net's final routed path


struct Point {
    int x,y,cost;
    Point(int x,int y,int c): x(x), y(y), cost(c) {}
};

bool starts_with(const string& s, const string& p) {
    return s.size()>=p.size() && s.compare(0,p.size(),p)==0;
}

pair<int,int> extract_coords_obstacle(const string& line) {
    auto p1 = line.find('('), p2 = line.find(',',p1), p3 = line.find(')',p2);
    int x = stoi(line.substr(p1+1,p2-p1-1));
    int y = stoi(line.substr(p2+1,p3-p2-1));
    return {x,y};
}

tuple<int,int,int> parse_pin(const string& s) {
    auto p1 = s.find('('), p2 = s.find(',',p1), p3 = s.find(',',p2+1), p4 = s.find(')',p3);
    int L = stoi(s.substr(p1+1,p2-p1-1));
    int x = stoi(s.substr(p2+1,p3-p2-1));
    int y = stoi(s.substr(p3+1,p4-p3-1));
    return {L,x,y};
}

void fill_nets(string s) {
    auto pos = s.find('(');
    string name = s.substr(0,pos);
    name.erase(name.find_last_not_of(" \t\r\n")+1);
    cout<<"Net name: "<<name<<endl;
    net_names.push_back(name);

    vector<tuple<int,int,int>> pins;
    stringstream ss(s.substr(pos));
    char c; string buf;
    while(ss>>c) {
        if(c=='(') {
            buf="(";
            while(ss>>c && c!=')') buf+=c;
            buf+=')';
            pins.push_back(parse_pin(buf));
            auto &t = pins.back();
            pin_sequence.emplace_back(get<1>(t), get<2>(t));
        }
    }
    all_nets.push_back(pins);
}

void route_net(vector<vector<vector<int>>>& grid, Point src, Point dst, const string& net_name) {
    auto cost = grid[0];
    for(int i=0;i<cost.size();++i)
        for(int j=0;j<cost[0].size();++j)
            cost[i][j] = (grid[0][i][j]==0||grid[0][i][j]==1) ? INT_MAX : -1;

    queue<Point> q;
    q.push(src);
    cost[src.x][src.y]=0;

    step_sequence.clear();
    step_sequence.emplace_back(src.x, src.y);

    while(!q.empty()) {
        auto cur = q.front(); q.pop();
        step_sequence.emplace_back(cur.x, cur.y);
        if(cur.x==dst.x && cur.y==dst.y) break;
        auto try_push=[&](int nx,int ny,int extra){
            if(nx>=0&&nx<cost.size()&&ny>=0&&ny<cost[0].size()&& cost[nx][ny]==INT_MAX){
                cost[nx][ny]=cur.cost+extra;
                q.push(Point(nx,ny,cost[nx][ny]));
            }
        };
        try_push(cur.x-1,cur.y,1);
        try_push(cur.x+1,cur.y,1);
        try_push(cur.x,cur.y-1,2);
        try_push(cur.x,cur.y+1,2);
    }

    Point cur = dst;
    routed_path.clear();  // reset before new trace
    while(cur.x!=src.x || cur.y!=src.y) {
        grid[0][cur.x][cur.y]=2;
        net_name_grid[cur.x][cur.y]=net_name;
        step_sequence.emplace_back(cur.x, cur.y);
        routed_path.emplace_back(cur.x, cur.y);

        int mc=INT_MAX; Point nxt=cur;
        auto consider=[&](int nx,int ny){
            if(nx>=0&&nx<cost.size()&&ny>=0&&ny<cost[0].size()){
                int c=cost[nx][ny];
                if(c>=0 && c<mc) { mc=c; nxt=Point(nx,ny,c); }
            }
        };
        consider(cur.x-1,cur.y);
        consider(cur.x+1,cur.y);
        consider(cur.x,cur.y-1);
        consider(cur.x,cur.y+1);
        cur=nxt;
    }
    routed_path.emplace_back(src.x, src.y);
    reverse(routed_path.begin(), routed_path.end());
}

void route_all_nets() {
    net_name_grid.assign(grid[0].size(), vector<string>(grid[0][0].size(), ""));
    vector<pair<int,int>> saved_pins;
    for(size_t k=0;k<all_nets.size();++k) {
        auto& pins = all_nets[k];
        const auto& nm = net_names[k];
        vector<pair<int,int>> blocked;
        for(int x=0;x<grid[0].size();++x)
            for(int y=0;y<grid[0][0].size();++y)
                if(grid[0][x][y]==1) {
                    bool in_cur=false;
                    for(auto&t:pins)
                        if(x==get<1>(t)&&y==get<2>(t)) { in_cur=true; break; }
                    if(!in_cur) { grid[0][x][y]=-1; blocked.emplace_back(x,y); }
                }

        for(size_t i=0;i+1<pins.size();++i) {
            auto [L1,sx,sy]=pins[i];
            auto [L2,dx,dy]=pins[i+1];
            grid[L1-1][sx][sy]=1;
            grid[L2-1][dx][dy]=1;
            route_net(grid, Point(sx,sy,0), Point(dx,dy,0), nm);
            all_step_sequences.push_back(step_sequence);      // store a copy per net
            all_routed_paths.push_back(routed_path);          // store the route
        }

        for(auto&p:blocked) grid[0][p.first][p.second]=1;
        saved_pins.insert(saved_pins.end(), blocked.begin(), blocked.end());
    }
    for(auto&p:saved_pins) grid[0][p.first][p.second]=1;
    for(auto& pins: all_nets)
      for(auto&t: pins)
        grid[get<0>(t)-1][get<1>(t)][get<2>(t)] = 1;
}

void print_grid() {
    for(int i=0;i<grid[0].size();++i)
        for(int j=0;j<grid[0][i].size();++j)
            if(grid[0][i][j]!=0)
                cout<<"Layer 1 - Cell ("<<i<<", "<<j<<") = "<<grid[0][i][j]<<endl;
}

void readfile(const string& fname) {
    ifstream in(fname);
    string line;
    getline(in,line);
    int R=stoi(line.substr(0,line.find('x')));
    int C=stoi(line.substr(line.find('x')+1));
    grid.assign(2, vector<vector<int>>(R, vector<int>(C,0)));

    while(getline(in,line)) {
        if(starts_with(line,"OBS")) {
            auto [x,y] = extract_coords_obstacle(line);
            grid[0][x][y] = -1;
            cout<<line<<endl;
            obstacle_sequence.emplace_back(x,y);
        }
        else if(starts_with(line,"net")) {
            fill_nets(line);
        }
    }
    in.close();
    route_all_nets();
    print_grid();
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
