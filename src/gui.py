import routing
import pygame
from collections import defaultdict
from collections import deque
import sys

# Read grid from C++
routing.readfile("../MS1_TestCases/Testcase3/input.txt")
grid = routing.get_grid()[0]  # Visualize Layer 1
net_name_grid = routing.get_net_name_grid()  # 2D array of net names

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

    # Draw the grid on the virtual surface
    virtual_surface.fill((220, 220, 220))
    for i in range(rows):
        for j in range(cols):
            value = grid[i][j]
            net_name = net_name_grid[i][j]
            if value == 1:
                color = (255, 0, 0)  # Always red for pins
            elif net_name:
                color = get_net_color(net_name)
            else:
                color = colors.get(value, (128, 128, 128))
            rect = pygame.Rect(j * cell_size, i * cell_size, cell_size - 2, cell_size - 2)
            pygame.draw.rect(virtual_surface, color, rect)
            # Draw row/col numbers
            if j == 0:
                num = small_font.render(str(i), True, (100, 100, 100))
                virtual_surface.blit(num, (rect.x + 2, rect.y + 2))
            if i == 0:
                num = small_font.render(str(j), True, (100, 100, 100))
                virtual_surface.blit(num, (rect.x + 2, rect.y + 2))

    # Blit the visible part to the screen
    screen.blit(virtual_surface, (0, 0), area=pygame.Rect(scroll_x, scroll_y, window_width, window_height))
    pygame.display.flip()

pygame.quit()
