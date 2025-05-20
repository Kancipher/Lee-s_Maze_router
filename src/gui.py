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

cell_size = 40
rows, cols = len(grid), len(grid[0])
virtual_width, virtual_height = cols * cell_size, rows * cell_size
window_width, window_height = min(virtual_width, 800), min(virtual_height, 800)
margin = 5  # Much smaller margin

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

# Layer switcher button properties
button_width = 100
button_height = 30
button_margin = 10
layer_buttons = [
    pygame.Rect(button_margin, button_margin, button_width, button_height),
    pygame.Rect(button_margin + button_width + button_margin, button_margin, button_width, button_height)
]

def draw_layer_buttons():
    for i, button in enumerate(layer_buttons):
        color = (100, 100, 255) if i == current_layer else (200, 200, 200)
        pygame.draw.rect(screen, color, button)
        text = font.render(f"Layer {i+1}", True, (0, 0, 0))
        text_rect = text.get_rect(center=button.center)
        screen.blit(text, text_rect)

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
            elif event.key == pygame.K_RIGHT:
                if virtual_width > window_width:
                    scroll_x = min(virtual_width - window_width, scroll_x + cell_size)
            elif event.key == pygame.K_UP:
                scroll_y = max(0, scroll_y - cell_size)
            elif event.key == pygame.K_DOWN:
                if virtual_height > window_height:
                    scroll_y = min(virtual_height - window_height, scroll_y + cell_size)
            elif event.key == pygame.K_F11:
                fullscreen = not fullscreen
                if fullscreen:
                    screen = pygame.display.set_mode((infoObject.current_w, infoObject.current_h), pygame.FULLSCREEN)
                    window_width, window_height = infoObject.current_w, infoObject.current_h
                else:
                    window_width, window_height = min(virtual_width, 800), min(virtual_height, 800)
                    screen = pygame.display.set_mode((window_width, window_height))
        elif event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 4:  # Scroll up
                scroll_y = max(0, scroll_y - cell_size)
            elif event.button == 5:  # Scroll down
                if virtual_height > window_height:
                    scroll_y = min(virtual_height - window_height, scroll_y + cell_size)
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
            # Swap i and j to match C++ grid indexing if needed
            grid_x, grid_y = j, i  # j is x (column), i is y (row)
            is_via = (grid_layers[0][grid_x][grid_y] == 3) or (grid_layers[1][grid_x][grid_y] == 3)
            value = grid[grid_x][grid_y]
            net_name = net_name_grid_layers[current_layer][grid_x][grid_y]
            if value == 1:
                color = (255, 0, 0)  # Always red for pins
            elif is_via:
                color = (0, 0, 0)  # Black for vias
            elif net_name:
                color = get_net_color(net_name)
            else:
                color = colors.get(value, (128, 128, 128))
            rect = pygame.Rect(j * cell_size, i * cell_size, cell_size - 2, cell_size - 2)
            pygame.draw.rect(virtual_surface, color, rect)
            
            # Draw X for vias
            if is_via:
                pygame.draw.line(virtual_surface, (255, 255, 255), 
                               (rect.x + 5, rect.y + 5), 
                               (rect.x + rect.width - 5, rect.y + rect.height - 5), 2)
                pygame.draw.line(virtual_surface, (255, 255, 255), 
                               (rect.x + rect.width - 5, rect.y + 5), 
                               (rect.x + 5, rect.y + rect.height - 5), 2)
            
            # Draw row/col numbers
            if j == 0:
                num = small_font.render(str(i), True, (100, 100, 100))
                virtual_surface.blit(num, (rect.x + 2, rect.y + 2))
            if i == 0:
                num = small_font.render(str(j), True, (100, 100, 100))
                virtual_surface.blit(num, (rect.x + 2, rect.y + 2))

    # Clear the screen
    screen.fill((220, 220, 220))
    draw_layer_buttons()
    screen.blit(virtual_surface, (0, button_height + 2 * button_margin), 
                area=pygame.Rect(scroll_x, scroll_y, window_width, window_height - (button_height + 2 * button_margin)))
    
    pygame.display.flip()

pygame.quit()
