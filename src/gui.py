import routing
import pygame
import argparse
from collections import defaultdict

parser = argparse.ArgumentParser(description='Routing Simulation')
parser.add_argument('input_file', help='Path to routing input file')
args = parser.parse_args()
routing.readfile(args.input_file)

obs_seq   = routing.get_obstacle_sequence()
pin_seq   = routing.get_pin_sequence()
net_names = routing.get_net_names()
all_steps = routing.get_all_step_sequences()
all_routes = routing.get_all_routed_paths()
grid      = routing.get_grid()[0]
net_map   = routing.get_net_name_grid()

CELL = 40
rows, cols = len(grid), len(grid[0])
VW, VH     = cols * CELL, rows * CELL
GRID_WW, GRID_WH = min(VW, 600), min(VH, 800)
STAT_WW = 300
WIN_W, WIN_H = cols * CELL, rows * CELL  # FULL grid size

GLW, OBW = 2, 4
EXT = GLW // 2

BG_CLR   = (255,255,255)
GRID_CLR = (255,204,0)
OB_CLR   = (128,128,128)
PIN_CLR  = (255,0,0)
SRC_CLR  = (255,100,100)

NET_COLORS = [
    (101, 67, 33),  # brown
    (50, 90, 168),  # blue
    (34, 139, 34),  # forest green
    (218, 112, 214),# orchid
    (255, 165, 0),  # orange
    (70, 130, 180), # steel blue
    (199, 21, 133), # medium violet red
    (255, 20, 147), # deep pink
    (0, 206, 209),  # dark turquoise
    (154, 205, 50)  # yellow green
]

pygame.init()
screen = pygame.display.set_mode((WIN_W, WIN_H))
pygame.display.set_caption('Maze Router')
font  = pygame.font.SysFont(None,16)
clock = pygame.time.Clock()

vs = pygame.Surface((VW, VH))
scroll_x = scroll_y = 0
running = True

phase   = 0
start_t = pygame.time.get_ticks()
bfs_i = trace_i = route_i = 0

bfs_seq = []
trace_seq = []
route_seq = []

final_obstacles = set((x, y) for x, y in obs_seq)
bfs_done_cells = set()
trace_done_cells = set()
route_done_cells = defaultdict(set)

net_index = 0
net_color_map = {}

def get_net_color(name):
    if name not in net_color_map:
        idx = len(net_color_map) % len(NET_COLORS)
        net_color_map[name] = NET_COLORS[idx]
    return net_color_map[name]

def load_net(index):
    global bfs_seq, trace_seq, route_seq, bfs_i, trace_i, route_i
    bfs_done_cells.clear()
    trace_done_cells.clear()
    steps = all_steps[index]
    route_seq = all_routes[index]
    split = next((i for i in range(len(steps)-1) if steps[i] == steps[i+1]), len(steps)-1)
    bfs_seq = steps[:split]
    trace_seq = steps[split+1:]
    bfs_i = trace_i = route_i = 0

load_net(net_index)

while running:
    for ev in pygame.event.get():
        if ev.type == pygame.QUIT:
            running = False
        elif ev.type == pygame.KEYDOWN:
            if ev.key in (pygame.K_ESCAPE, pygame.K_q):
                running = False
            elif ev.key == pygame.K_SPACE and phase == 5:
                phase = 0
                start_t = pygame.time.get_ticks()
                bfs_done_cells.clear()
                trace_done_cells.clear()
                route_done_cells.clear()
                final_obstacles.clear()
                final_obstacles.update(obs_seq)
                net_index = 0
                load_net(net_index)
            elif ev.key == pygame.K_LEFT:
                scroll_x = max(0, scroll_x - CELL)
            elif ev.key == pygame.K_RIGHT:
                scroll_x = min(VW - GRID_WW, scroll_x + CELL)
            elif ev.key == pygame.K_UP:
                scroll_y = max(0, scroll_y - CELL)
            elif ev.key == pygame.K_DOWN:
                scroll_y = min(VH - GRID_WH, scroll_y + CELL)
            elif ev.key == pygame.K_F11:
                fs = screen.get_flags() & pygame.FULLSCREEN
                screen = pygame.display.set_mode((WIN_W, WIN_H), pygame.FULLSCREEN if not fs else 0)

    vs.fill(BG_CLR)
    for x in range(cols + 1):
        pygame.draw.line(vs, GRID_CLR, (x * CELL, 0), (x * CELL, VH), GLW)
    for y in range(rows + 1):
        pygame.draw.line(vs, GRID_CLR, (0, y * CELL), (VW, y * CELL), GLW)

    for x in range(rows):
        for y in range(cols):
            if (x, y) in final_obstacles:
                pygame.draw.rect(vs, OB_CLR, (y * CELL, x * CELL, CELL, CELL))
    for i, (x, y) in enumerate(pin_seq):
        clr = SRC_CLR if i == 0 else PIN_CLR
        pygame.draw.rect(vs, clr, pygame.Rect(y * CELL - EXT, x * CELL - EXT, CELL + 2 * EXT, CELL + 2 * EXT))

    now = pygame.time.get_ticks()
    if phase == 0 and now - start_t > 1000:
        phase = 1

    if phase >= 1:
        for x, y in bfs_done_cells:
            if (x, y) not in pin_seq:
                r = pygame.Rect(y * CELL + 2, x * CELL + 2, CELL - 4, CELL - 4)
                pygame.draw.rect(vs, (255, 255, 255), r)
                pygame.draw.rect(vs, (0, 200, 0), r, 2)

    if phase == 1:
        if bfs_i < len(bfs_seq):
            x, y = bfs_seq[bfs_i]
            bfs_done_cells.add((x, y))
            bfs_i += 1
            clock.tick(5)
        else:
            phase = 2

    if phase >= 2:
        for x, y in trace_done_cells:
            if (x, y) not in pin_seq:
                r = pygame.Rect(y * CELL, x * CELL, CELL, CELL)
                pygame.draw.rect(vs, (255, 255, 255), r)
                pygame.draw.rect(vs, (0, 200, 0), r, width=GLW)  # Use same thickness as yellow lines

    if phase == 2:
        if trace_i < len(trace_seq):
            x, y = trace_seq[trace_i]
            trace_done_cells.add((x, y))

            # If reaching a pin, temporarily shade it green
            if (x, y) in pin_seq:
                pygame.draw.rect(vs, (0, 255, 0), (y * CELL, x * CELL, CELL, CELL))
                screen.blit(vs, (0, 0), pygame.Rect(scroll_x, scroll_y, WIN_W, WIN_H))
                pygame.display.flip()
                pygame.time.delay(300)  # pause for 300 ms

            trace_i += 1
            clock.tick(5)
        else:
            phase = 3

    if phase == 3:
        route_i = 0
        phase = 4

    if phase >= 4:
        for name, cells in route_done_cells.items():
            color = get_net_color(name)
            for x, y in cells:
                pygame.draw.rect(vs, color, (y * CELL, x * CELL, CELL, CELL))

    if phase == 4:
        if route_i < len(route_seq):
            x, y = route_seq[route_i]
            netname = net_map[x][y]
            route_done_cells[netname].add((x, y))
            route_i += 1
            clock.tick(5)
        else:
            phase = 5

    if phase == 5:
        for name, cells in route_done_cells.items():
            for x, y in cells:
                final_obstacles.add((x, y))
        route_done_cells.clear()
        net_index += 1
        if net_index < len(all_steps):
            load_net(net_index)
            phase = 0
            start_t = pygame.time.get_ticks()
        else:
            phase = 6

    pygame.draw.rect(vs, GRID_CLR, (0, 0, VW, VH), OBW)
    for i in range(rows):
        vs.blit(font.render(str(i), True, (100, 100, 100)), (2, i * CELL + 2))
    for j in range(cols):
        vs.blit(font.render(str(j), True, (100, 100, 100)), (j * CELL + 2, 2))

    screen.fill(BG_CLR)
    screen.blit(vs, (0, 0), pygame.Rect(scroll_x, scroll_y, WIN_W, WIN_H))

    pygame.display.flip()

pygame.quit()
