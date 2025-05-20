import routing
import pygame
from collections import defaultdict
from collections import deque
import sys
import argparse

parser = argparse.ArgumentParser(description='Visualize routing grid from input file')
parser.add_argument('input_file', help='Path to the input file')
args = parser.parse_args()
routing.readfile(args.input_file)

# Get both layers from the grid
grid_layers = routing.get_grid()  # Now we get both layers
current_layer = 0  # Start with layer 0
grid = grid_layers[current_layer]  # Current layer to display
net_name_grid_layers = routing.get_net_name_grid()  # 3D array: [layer][x][y]

rows, cols = len(grid), len(grid[0])
window_width, window_height = 800, 800  # Fixed window size
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

# Define colors for cell values
colors = {
    -1: (0, 0, 0),        # Obstacle: Black
     0: (255, 255, 255),  # Empty: White
     1: (255, 0, 0),      # Pin: Red
     3: (0, 0, 0),        # Via: Black
}

# Assign a unique color to each net name (different green shades)
net_color_map = defaultdict(lambda: (0, 255, 0))
net_colors = [
    (0, 200, 0),    # Green
    (0, 120, 255),  # Blue
    (255, 180, 0),  # Orange
    (180, 0, 255),  # Purple
    (0, 255, 180),  # Aqua
    (255, 0, 120),  # Pink
    (120, 255, 0),  # Lime
    (255, 255, 0),  # Yellow
    (0, 255, 255),  # Cyan
    (255, 0, 0),    # Red (avoid for pins, but OK for routes if needed)
]

def get_net_color(net_name):
    if net_name not in net_color_map:
        net_color_map[net_name] = net_colors[len(net_color_map) % len(net_colors)]
    return net_color_map[net_name]

pygame.init()
infoObject = pygame.display.Info()
fullscreen = False
screen = pygame.display.set_mode((window_width, window_height))
pygame.display.set_caption("Routing Grid Visualizer")
font = pygame.font.SysFont(None, 24)
small_font = pygame.font.SysFont(None, 16)

# Create a virtual surface for the whole grid
virtual_surface = pygame.Surface((virtual_width, virtual_height))

# Initial scroll position
scroll_x, scroll_y = 0, 0

def draw_layer_buttons():
    for i, button in enumerate(layer_buttons):
        color = (100, 100, 255) if i == current_layer else (200, 200, 200)
        pygame.draw.rect(screen, color, button)
        text = font.render(f"Layer {i+1}", True, (0, 0, 0))
        text_rect = text.get_rect(center=button.center)
        screen.blit(text, text_rect)

def clamp_scroll():
    global scroll_x, scroll_y
    if virtual_width <= grid_draw_width:
        scroll_x = 0
    else:
        scroll_x = max(0, min(scroll_x, virtual_width - grid_draw_width))
    if virtual_height <= grid_draw_height:
        scroll_y = 0
    else:
        scroll_y = max(0, min(scroll_y, virtual_height - grid_draw_height))

def update_cell_size():
    global cell_size, virtual_width, virtual_height, grid_draw_width, grid_draw_height
    cell_size = max(5, int(base_cell_size * zoom))
    virtual_width, virtual_height = cols * cell_size, rows * cell_size
    grid_draw_width = window_width
    grid_draw_height = window_height - grid_draw_y
    clamp_scroll()

# Main loop
running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_ESCAPE or event.key == pygame.K_q:
                running = False
            elif event.key == pygame.K_LEFT:
                scroll_x = max(0, scroll_x - cell_size)
                clamp_scroll()
            elif event.key == pygame.K_RIGHT:
                scroll_x = min(scroll_x + cell_size, max(0, virtual_width - window_width))
                clamp_scroll()
            elif event.key == pygame.K_UP:
                scroll_y = max(0, scroll_y - cell_size)
                clamp_scroll()
            elif event.key == pygame.K_DOWN:
                scroll_y = min(scroll_y + cell_size, max(0, virtual_height - window_height))
                clamp_scroll()
            elif event.key == pygame.K_F11:
                fullscreen = not fullscreen
                if fullscreen:
                    screen = pygame.display.set_mode((infoObject.current_w, infoObject.current_h), pygame.FULLSCREEN)
                    window_width, window_height = infoObject.current_w, infoObject.current_h
                else:
                    window_width, window_height = 800, 800
                    screen = pygame.display.set_mode((window_width, window_height))
                update_cell_size()
            elif event.key == pygame.K_PLUS or event.key == pygame.K_EQUALS:
                zoom = min(5.0, zoom * 1.1)
                update_cell_size()
            elif event.key == pygame.K_MINUS or event.key == pygame.K_UNDERSCORE:
                zoom = max(0.1, zoom / 1.1)
                update_cell_size()
        elif event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 4:  # Scroll up (zoom in)
                zoom = min(5.0, zoom * 1.1)
                update_cell_size()
            elif event.button == 5:  # Scroll down (zoom out)
                zoom = max(0.1, zoom / 1.1)
                update_cell_size()
            elif event.button == 1: 
                for i, button in enumerate(layer_buttons):
                    if button.collidepoint(event.pos):
                        current_layer = i
                        grid = grid_layers[current_layer]
                        break

    # Draw the grid on the virtual surface
    virtual_surface.fill((220, 220, 220))
    for i in range(rows):
        for j in range(cols):
            grid_x, grid_y = j, i
            is_via = (grid_layers[0][grid_x][grid_y] == 3) or (grid_layers[1][grid_x][grid_y] == 3)
            value = grid[grid_x][grid_y]
            net_name = net_name_grid_layers[current_layer][grid_x][grid_y]
            if value == 1:
                color = (255, 0, 0)
            elif is_via:
                color = (0, 0, 0)
            elif net_name:
                color = get_net_color(net_name)
            else:
                color = colors.get(value, (128, 128, 128))
            rect = pygame.Rect(j * cell_size, i * cell_size, cell_size - 2, cell_size - 2)
            pygame.draw.rect(virtual_surface, color, rect)
            if is_via:
                pygame.draw.line(virtual_surface, (255, 255, 255), 
                                 (rect.x + 5, rect.y + 5), 
                                 (rect.x + rect.width - 5, rect.y + rect.height - 5), 2)
                pygame.draw.line(virtual_surface, (255, 255, 255), 
                                 (rect.x + rect.width - 5, rect.y + 5), 
                                 (rect.x + 5, rect.y + rect.height - 5), 2)
            if j == 0:
                num = small_font.render(str(i), True, (100, 100, 100))
                virtual_surface.blit(num, (rect.x + 2, rect.y + 2))
            if i == 0:
                num = small_font.render(str(j), True, (100, 100, 100))
                virtual_surface.blit(num, (rect.x + 2, rect.y + 2))

    # Clear the screen
    screen.fill((220, 220, 220))
    draw_layer_buttons()
    visible_width = min(grid_draw_width, virtual_width - scroll_x)
    visible_height = min(grid_draw_height, virtual_height - scroll_y)
    screen.blit(
        virtual_surface,
        (grid_draw_x, grid_draw_y),
        area=pygame.Rect(
            scroll_x,
            scroll_y,
            visible_width,
            visible_height
        )
    )
    pygame.display.flip()

pygame.quit()
