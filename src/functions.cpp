#include "functions.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <numeric>
#include <queue>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>

Grid grid;
Names net_name_grid;
std::vector<RouteResult> route_results;
namespace {
int via_cost = 10, direction_cost = 10;
constexpr int max_states = 200000;
struct Net {
  std::string name;
  std::vector<int> pins;
};
struct Board {
  int rows = 0, cols = 0;
  std::vector<int> owner; // -2 obstacle, -1 free, >=0 net index; pins reserved
                          // before routing
  std::vector<Net> nets;
  int id(int l, int x, int y) const { return (l * cols + x) * rows + y; }
  Cell cell(int n) const {
    return {n / (cols * rows), (n / rows) % cols, n % rows};
  }
};
std::string trim(std::string s) {
  auto a = s.find_first_not_of(" \t\r");
  if (a == s.npos)
    return {};
  return s.substr(a, s.find_last_not_of(" \t\r") - a + 1);
}
Board parse(const std::string &path) {
  std::ifstream in(path);
  if (!in)
    throw std::runtime_error("Cannot open input");
  if (std::filesystem::file_size(path) > 1024 * 1024)
    throw std::invalid_argument("Input exceeds 1 MiB");
  Board b;
  std::string line;
  std::smatch m;
  int number = 0, total_pins = 0;
  std::set<std::string> names;
  const std::regex size(R"(([0-9]{1,6})\s*x\s*([0-9]{1,6}))"),
      obs(R"(OBS\s*\(\s*([0-9]{1,6})\s*,\s*([0-9]{1,6})\s*\))"),
      pin(R"(^\(\s*([12])\s*,\s*([0-9]{1,6})\s*,\s*([0-9]{1,6})\s*\))");
  auto fail = [&](const std::string &why) {
    throw std::invalid_argument("Line " + std::to_string(number) + ": " + why);
  };
  while (std::getline(in, line)) {
    ++number;
    if (line.size() > 4096)
      fail("Line exceeds 4096 bytes");
    line = trim(line);
    if (line.empty())
      continue;
    if (!b.rows) {
      if (!std::regex_match(line, m, size))
        fail("Expected rows x columns");
      long long r = std::stoll(m[1]), c = std::stoll(m[2]);
      if (r < 1 || c < 1 || r > max_states || c > max_states ||
          r * c > max_states / 2)
        fail("Grid must have 1..100000 cells per layer");
      b.rows = static_cast<int>(r);
      b.cols = static_cast<int>(c);
      b.owner.assign(2 * r * c, -1);
      continue;
    }
    if (std::regex_match(line, m, obs)) {
      int x = std::stoi(m[1]), y = std::stoi(m[2]);
      if (x >= b.cols || y >= b.rows)
        fail("Obstacle outside grid");
      for (int l = 0; l < 2; ++l) {
        int n = b.id(l, x, y);
        if (b.owner[n] >= 0)
          fail("Obstacle overlaps pin");
        b.owner[n] = -2;
      }
      continue;
    }
    auto pos = line.find('(');
    if (pos == line.npos)
      fail("Expected OBS or net pins");
    std::string name = trim(line.substr(0, pos));
    if (!std::regex_match(name, std::regex(R"(net[A-Za-z0-9_-]{0,61})")) ||
        !names.insert(name).second)
      fail("Invalid or duplicate net name");
    if (b.nets.size() >= 256)
      fail("Maximum 256 nets");
    Net net{name, {}};
    std::string rest = trim(line.substr(pos));
    while (!rest.empty()) {
      if (!std::regex_search(rest, m, pin))
        fail("Expected (layer,x,y), layer 1 or 2");
      int l = std::stoi(m[1]) - 1, x = std::stoi(m[2]), y = std::stoi(m[3]);
      if (x >= b.cols || y >= b.rows)
        fail("Pin outside grid");
      int n = b.id(l, x, y);
      if (b.owner[n] != -1)
        fail("Pin overlaps obstacle or another pin");
      b.owner[n] = static_cast<int>(b.nets.size());
      net.pins.push_back(n);
      if (++total_pins > 2048)
        fail("Maximum 2048 pins");
      rest = trim(m.suffix());
    }
    if (net.pins.size() < 2)
      fail("Net needs at least two distinct pins");
    b.nets.push_back(std::move(net));
  }
  if (!b.rows || b.nets.empty())
    throw std::invalid_argument("Grid and at least one net required");
  return b;
}
// Multi-source Dijkstra connects each additional pin to the existing tree.
// Parents are explicit; no board mutation takes place until a complete branch
// exists.
bool connect(const Board &b, int net, const std::set<int> &tree, int target,
             std::vector<int> &path, std::int64_t &cost) {
  using Item = std::pair<std::int64_t, int>;
  std::vector<std::int64_t> dist(b.owner.size(),
                                 std::numeric_limits<std::int64_t>::max());
  std::vector<int> parent(b.owner.size(), -1);
  std::priority_queue<Item, std::vector<Item>, std::greater<Item>> q;
  for (int s : tree) {
    dist[s] = 0;
    q.push({0, s});
  }
  while (!q.empty()) {
    auto [d, n] = q.top();
    q.pop();
    if (d != dist[n])
      continue;
    if (n == target)
      break;
    auto [l, x, y] = b.cell(n);
    auto relax = [&](int ll, int xx, int yy, int weight) {
      if (xx < 0 || xx >= b.cols || yy < 0 || yy >= b.rows)
        return;
      int next = b.id(ll, xx, yy);
      if (b.owner[next] != -1 && b.owner[next] != net)
        return;
      if (d + weight < dist[next]) {
        dist[next] = d + weight;
        parent[next] = n;
        q.push({dist[next], next});
      }
    };
    relax(l, x - 1, y, l == 0 ? 1 : direction_cost);
    relax(l, x + 1, y, l == 0 ? 1 : direction_cost);
    relax(l, x, y - 1, l == 1 ? 1 : direction_cost);
    relax(l, x, y + 1, l == 1 ? 1 : direction_cost);
    relax(1 - l, x, y, via_cost);
  }
  if (dist[target] == std::numeric_limits<std::int64_t>::max())
    return false;
  cost = dist[target];
  for (int n = target; n != -1; n = parent[n])
    path.push_back(n);
  return true;
}
} // namespace
void set_costs(int via, int nonpreferred) {
  if (via < 1 || via > 1000000 || nonpreferred < 1 || nonpreferred > 1000000)
    throw std::invalid_argument("Costs must be 1..1000000");
  via_cost = via;
  direction_cost = nonpreferred;
}
void readfile(std::string input, std::string output) {
  if (std::filesystem::exists(output) &&
      std::filesystem::equivalent(input, output))
    throw std::invalid_argument("Input and output must differ");
  Board b = parse(input);
  Grid next_grid(
      2, std::vector<std::vector<int>>(b.cols, std::vector<int>(b.rows)));
  Names next_names(2, std::vector<std::vector<std::string>>(
                          b.cols, std::vector<std::string>(b.rows)));
  for (std::size_t n = 0; n < b.owner.size(); ++n) {
    auto [l, x, y] = b.cell(static_cast<int>(n));
    if (b.owner[n] == -2)
      next_grid[l][x][y] = -1;
    else if (b.owner[n] >= 0)
      next_grid[l][x][y] = 1;
  }
  std::vector<RouteResult> results(b.nets.size());
  std::vector<int> order(b.nets.size());
  std::iota(order.begin(), order.end(), 0);
  auto span = [&](int i) {
    int minx = b.cols, miny = b.rows, maxx = 0, maxy = 0;
    for (int n : b.nets[i].pins) {
      auto [l, x, y] = b.cell(n);
      (void)l;
      minx = std::min(minx, x);
      miny = std::min(miny, y);
      maxx = std::max(maxx, x);
      maxy = std::max(maxy, y);
    }
    return std::make_tuple(maxx - minx + maxy - miny, b.nets[i].pins.size(), i);
  };
  std::stable_sort(order.begin(), order.end(),
                   [&](int a, int c) { return span(a) < span(c); });
  for (int i : order) {
    const Net &net = b.nets[i];
    std::set<int> tree{net.pins.front()};
    std::vector<std::vector<int>> branches;
    std::int64_t total = 0;
    bool ok = true;
    for (std::size_t p = 1; p < net.pins.size(); ++p) {
      std::vector<int> path;
      std::int64_t cost = 0;
      if (!connect(b, i, tree, net.pins[p], path, cost)) {
        ok = false;
        break;
      }
      tree.insert(path.begin(), path.end());
      branches.push_back(std::move(path));
      total += cost;
    }
    results[i] = {net.name, ok, ok ? total : 0, {}};
    if (!ok)
      continue; // Failed nets retain only their reserved pins; no partial
                // tracks.
    for (int n : tree) {
      b.owner[n] = i;
      auto [l, x, y] = b.cell(n);
      if (next_grid[l][x][y] != 1)
        next_grid[l][x][y] = 2;
      next_names[l][x][y] = std::to_string(i + 1);
      results[i].cells.push_back({l + 1, x, y});
    }
    for (const auto &path : branches)
      for (std::size_t p = 1; p < path.size(); ++p) {
        auto [l, x, y] = b.cell(path[p]);
        auto [pl, px, py] = b.cell(path[p - 1]);
        (void)px;
        (void)py;
        if (l != pl) {
          next_grid[l][x][y] = 3;
          next_grid[pl][x][y] = 3;
        }
      }
  }
  // Existing outputs are deliberately not truncated. Choose a fresh path for
  // each run.
  if (std::filesystem::exists(output))
    throw std::invalid_argument("Output exists; choose a new path");
  std::ofstream out(output);
  if (!out)
    throw std::runtime_error("Cannot create output");
  for (const auto &r : results) {
    out << r.name;
    if (!r.routed)
      out << " UNROUTABLE";
    else
      for (auto [l, x, y] : r.cells)
        out << " (" << l << "," << x << "," << y << ")";
    out << "\n";
  }
  out.close();
  if (!out)
    throw std::runtime_error("Output write failed");
  grid = std::move(next_grid);
  net_name_grid = std::move(next_names);
  route_results = std::move(results);
}
