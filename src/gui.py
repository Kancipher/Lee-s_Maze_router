import routing
import pygame

# Read grid from C++
routing.readfile("input.txt")
grid = routing.get_grid()[0]  # Visualize Layer 1

# Set up display
cell_size = 40
rows, cols = len(grid), len(grid[0])
screen_width, screen_height = cols * cell_size, rows * cell_size

# Define colors for cell values
colors = {
    -1: (0, 0, 0),        # Obstacle: Black
     0: (255, 255, 255),  # Empty: White
     1: (255, 0, 0),      # Pin: Red
     2: (0, 255, 0),      # Routed Path: Green
}

# Initialize pygame
pygame.init()
screen = pygame.display.set_mode((screen_width, screen_height))
pygame.display.set_caption("Routing Grid Visualizer")

# Main loop
running = True
while running:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            running = False

    screen.fill((220, 220, 220))

    # Draw each cell
    for i in range(rows):
        for j in range(cols):
            value = grid[i][j]
            color = colors.get(value, (128, 128, 128))
            rect = pygame.Rect(j * cell_size, i * cell_size, cell_size - 2, cell_size - 2)
            pygame.draw.rect(screen, color, rect)

    pygame.display.flip()

pygame.quit()
