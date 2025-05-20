import routing
import pygame
from collections import defaultdict
import argparse
import time

parser = argparse.ArgumentParser(description='Routing Grid Simulation & Visualizer')
parser.add_argument('input_file', help='Path to the input file')
args = parser.parse_args()
routing.readfile(args.input_file)

grid_layers = routing.get_grid()
net_name_grid_layers = routing.get_net_name_grid()
all_steps = routing.get_all_step_sequences()
all_routes = routing.get_all_routed_paths()
net_names = routing.get_net_names()

rows, cols = len(grid_layers[0][0]), len(grid_layers[0])
current_layer = 0

window_width, window_height = 800, 800
base_cell_size = min(window_width // cols, window_height // rows)
zoom = 1.0
cell_size = int(base_cell_size * zoom)
virtual_width, virtual_height = cols * cell_size, rows * cell_size

button_width = 100
button_height = 30
button_margin = 10
layer_buttons = [
    pygame.Rect(button_margin, button_margin, button_width, button_height),
    pygame.Rect(button_margin + button_width + button_margin, button_margin, button_width, button_height)
]

grid_draw_x = 0
button_area_height = button_height + 2 * button_margin
grid_draw_y = button_area_height
grid_draw_width = window_width
grid_draw_height = window_height - grid_draw_y

colors = {-1: (0, 0, 0), 0: (255, 255, 255), 1: (255, 0, 0), 3: (0, 0, 0)}

route_colors = [
    (0, 200, 0), (0, 120, 255), (255, 180, 0), (180, 0, 255),
    (0, 255, 180), (255, 0, 120), (120, 255, 0), (255, 255, 0),
    (0, 255, 255), (255, 0, 0),
]

def get_route_color(order_str):
    try:
        order = int(order_str)
        return route_colors[(order - 1) % len(route_colors)]
    except ValueError:
        return (128, 128, 128)

pygame.init()
screen = pygame.display.set_mode((window_width, window_height))
pygame.display.set_caption("Routing Grid Viewer & Simulator")
font = pygame.font.SysFont(None, 24)
small_font = pygame.font.SysFont(None, 16)
virtual_surface = pygame.Surface((virtual_width, virtual_height))
scroll_x, scroll_y = 0, 0
infoObject = pygame.display.Info()
fullscreen = False

step_index = 0
route_index = 0
net_index = 0
phase = 'BFS'
bfs_seq, trace_seq = [], []
bfs_done = set()
trace_done = set()
show_static = False

route_seq = all_routes[net_index]
steps = all_steps[net_index]
split = next((i for i in range(len(steps)-1) if steps[i] == steps[i+1]), len(steps)-1)
bfs_seq = steps[:split]
trace_seq = steps[split+1:]

# Detect via cells
via_cells = {(x, y) for x in range(cols) for y in range(rows)
             if grid_layers[0][x][y] == 3 or grid_layers[1][x][y] == 3}

clock = pygame.time.Clock()
running = True

while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_ESCAPE:
                running = False
            elif event.key == pygame.K_s:
                show_static = not show_static
            elif event.key == pygame.K_l:
                current_layer = 1 - current_layer
            elif event.key == pygame.K_LEFT:
                scroll_x = max(0, scroll_x - cell_size)
            elif event.key == pygame.K_RIGHT:
                scroll_x = min(scroll_x + cell_size, max(0, virtual_width - window_width))
            elif event.key == pygame.K_UP:
                scroll_y = max(0, scroll_y - cell_size)
            elif event.key == pygame.K_DOWN:
                scroll_y = min(scroll_y + cell_size, max(0, virtual_height - window_height))
            elif event.key == pygame.K_PLUS or event.key == pygame.K_EQUALS:
                zoom = min(5.0, zoom * 1.1)
                cell_size = max(5, int(base_cell_size * zoom))
                virtual_width, virtual_height = cols * cell_size, rows * cell_size
                virtual_surface = pygame.Surface((virtual_width, virtual_height))
            elif event.key == pygame.K_MINUS or event.key == pygame.K_UNDERSCORE:
                zoom = max(0.1, zoom / 1.1)
                cell_size = max(5, int(base_cell_size * zoom))
                virtual_width, virtual_height = cols * cell_size, rows * cell_size
                virtual_surface = pygame.Surface((virtual_width, virtual_height))
            elif event.key == pygame.K_F11:
                fullscreen = not fullscreen
                if fullscreen:
                    screen = pygame.display.set_mode((infoObject.current_w, infoObject.current_h), pygame.FULLSCREEN)
                    window_width, window_height = infoObject.current_w, infoObject.current_h
                else:
                    window_width, window_height = 800, 800
                    screen = pygame.display.set_mode((window_width, window_height))
                base_cell_size = min(window_width // cols, window_height // rows)
                cell_size = max(5, int(base_cell_size * zoom))
                virtual_width, virtual_height = cols * cell_size, rows * cell_size
                virtual_surface = pygame.Surface((virtual_width, virtual_height))
        elif event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1:
                for i, button in enumerate(layer_buttons):
                    if button.collidepoint(event.pos):
                        current_layer = i
            elif event.button == 4:
                zoom = min(5.0, zoom * 1.1)
                cell_size = max(5, int(base_cell_size * zoom))
                virtual_width, virtual_height = cols * cell_size, rows * cell_size
                virtual_surface = pygame.Surface((virtual_width, virtual_height))
            elif event.button == 5:
                zoom = max(0.1, zoom / 1.1)
                cell_size = max(5, int(base_cell_size * zoom))
                virtual_width, virtual_height = cols * cell_size, rows * cell_size
                virtual_surface = pygame.Surface((virtual_width, virtual_height))

    virtual_surface.fill((220, 220, 220))

    for x in range(cols):
        for y in range(rows):
            val = grid_layers[current_layer][x][y]
            net = net_name_grid_layers[current_layer][x][y]
            is_via = (grid_layers[0][x][y] == 3 or grid_layers[1][x][y] == 3)
            rect = pygame.Rect(x * cell_size, y * cell_size, cell_size - 2, cell_size - 2)

            if val == 1:
                color = (255, 0, 0)
            elif is_via:
                color = (0, 0, 0)
            elif show_static and net:
                color = get_route_color(net)
            elif phase == 'DONE' and net:  # Final static rendering
                color = get_route_color(net)
            elif phase == 'DONE' and net:
                color = get_route_color(net)  # Show each net with its original color
            elif not show_static and (x, y) in route_seq[:route_index] and phase == 'ROUTE':
                color = get_route_color(str(net_index + 1))  # During animation
            elif val == 5 and show_static:
                color = (150, 150, 150)  # Optional: static toggle
            elif val == 5:
                color = (255, 255, 255)  # Hide routed path unless done or toggled

            else:
                color = colors.get(val, (255, 255, 255))

            pygame.draw.rect(virtual_surface, color, rect)

            if is_via:
                pygame.draw.line(virtual_surface, (255, 255, 255), (rect.x + 5, rect.y + 5),
                                 (rect.x + rect.width - 5, rect.y + rect.height - 5), 2)
                pygame.draw.line(virtual_surface, (255, 255, 255), (rect.x + rect.width - 5, rect.y + 5),
                                 (rect.x + 5, rect.y + rect.height - 5), 2)

            if x == 0:
                label = small_font.render(str(y), True, (100, 100, 100))
                virtual_surface.blit(label, (rect.x + 2, rect.y + 2))
            if y == 0:
                label = small_font.render(str(x), True, (100, 100, 100))
                virtual_surface.blit(label, (rect.x + 2, rect.y + 2))

    if not show_static:
        for x, y in bfs_done:
            r = pygame.Rect(x * cell_size, y * cell_size, cell_size, cell_size)
            color = get_route_color(str(net_index + 1))
            pygame.draw.rect(virtual_surface, color, r, 2)

        if phase == 'BFS':
            if step_index < len(bfs_seq):
                step = bfs_seq[step_index]
                if step in via_cells:
                    current_layer = 1 - current_layer
                bfs_done.add(step)
                step_index += 1
            else:
                phase = 'TRACE'
                step_index = 0
                time.sleep(0.3)

        for x, y in trace_done:
            r = pygame.Rect(x * cell_size, y * cell_size, cell_size, cell_size)
            color = get_route_color(str(net_index + 1))
            pygame.draw.rect(virtual_surface, color, r, 2)

        if phase == 'TRACE':
            if step_index < len(trace_seq):
                step = trace_seq[step_index]
                if step in via_cells:
                    current_layer = 1 - current_layer
                trace_done.add(step)
                step_index += 1
            else:
                phase = 'ROUTE'
                route_index = 0
                time.sleep(0.3)

        if phase == 'ROUTE':
            if route_index < len(route_seq):
                step = route_seq[route_index]
                if step in via_cells:
                    current_layer = 1 - current_layer
                route_index += 1
            else:
                time.sleep(0.5)
                net_index += 1
                if net_index < len(all_steps):
                    steps = all_steps[net_index]
                    route_seq = all_routes[net_index]
                    split = next((i for i in range(len(steps) - 1) if steps[i] == steps[i + 1]), len(steps) - 1)
                    bfs_seq = steps[:split]
                    trace_seq = steps[split + 1:]
                    bfs_done.clear()
                    trace_done.clear()
                    step_index = 0
                    route_index = 0
                    phase = 'BFS'
                else:
                    phase = 'DONE'

    for x in range(cols):
        for y in range(rows):
            if grid_layers[current_layer][x][y] == -1:
                rect = pygame.Rect(x * cell_size, y * cell_size, cell_size - 2, cell_size - 2)
                pygame.draw.rect(virtual_surface, (0, 0, 0), rect)

    screen.fill((220, 220, 220))
    for i, button in enumerate(layer_buttons):
        color = (100, 100, 255) if i == current_layer else (200, 200, 200)
        pygame.draw.rect(screen, color, button)
        text = font.render(f"Layer {i+1}", True, (0, 0, 0))
        screen.blit(text, text.get_rect(center=button.center))

    screen.blit(
        virtual_surface,
        (grid_draw_x, grid_draw_y),
        area=pygame.Rect(scroll_x, scroll_y, min(grid_draw_width, virtual_width - scroll_x), min(grid_draw_height, virtual_height - scroll_y))
    )

    mode_text = "Static View (S to toggle)" if show_static else f"Phase: {phase} | Net: {net_index + 1}/{len(all_steps)}"
    label = font.render(mode_text, True, (0, 0, 0))
    screen.blit(label, (window_width // 2 - 100, 5))

    pygame.display.flip()
    clock.tick(10)

pygame.quit()
