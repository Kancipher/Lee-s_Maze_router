"""Bounded viewport for the native two-layer router. Importing has no side effects."""
import argparse
import sys


def render(screen, pygame, layers, names, layer, cell, scroll):
    screen.fill((230, 233, 239))
    width, height = screen.get_size()
    font = pygame.font.Font(None, 24)
    screen.blit(font.render(f"Layer {layer+1} | L/click: layer | arrows: pan | +/-: zoom | Q: quit", True, (20, 25, 35)), (10, 12))
    palette = [(30, 150, 80), (40, 110, 215), (170, 80, 190), (215, 145, 30)]
    cols, rows = len(layers[layer]), len(layers[layer][0])
    sx, sy = scroll
    # Draw only the visible region; never allocate a grid-sized virtual surface.
    screen.set_clip(pygame.Rect(0, 45, width, height-45))
    for x in range(sx//cell, min(cols, (sx+width)//cell+1)):
        for y in range(sy//cell, min(rows, (sy+height-45)//cell+1)):
            value, owner = layers[layer][x][y], names[layer][x][y]
            color = (250, 250, 250)
            if owner:
                color = palette[(int(owner)-1) % len(palette)]
            if value == -1:
                color = (35, 40, 50)
            elif value == 1:
                color = (220, 45, 60)
            elif value == 3:
                color = (15, 15, 20)
            rect = pygame.Rect(x*cell-sx, y*cell-sy+45, max(1, cell-1), max(1, cell-1))
            pygame.draw.rect(screen, color, rect)
            if value == 3 and cell >= 8:
                pygame.draw.circle(screen, (255, 255, 255), rect.center, max(1, cell//5), 1)
    screen.set_clip(None)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input_file')
    parser.add_argument('output_file', help='Must not already exist')
    parser.add_argument('via', nargs='?', default='via=10')
    parser.add_argument('direction', nargs='?', default='direction=10')
    parser.add_argument('--screenshot', help='Render one frame to this PNG and exit')
    args = parser.parse_args(argv)
    try:
        def cost(value, key):
            prefix, value = value.split('=', 1)
            if prefix != key or not value.isdigit() or not 1 <= int(value) <= 1000000:
                raise ValueError(f'{key} must be 1..1000000')
            return int(value)
        import routing
        routing.set_costs(cost(args.via, 'via'), cost(args.direction, 'direction'))
        routing.readfile(args.input_file, args.output_file)
        layers, names = routing.get_grid(), routing.get_net_name_grid()
        results = routing.get_results()
    except (ImportError, ValueError, RuntimeError, OSError) as error:
        parser.exit(1, f'Error: {error}\n')
    import pygame
    pygame.init()
    try:
        screen = pygame.display.set_mode((900, 650), pygame.RESIZABLE)
        pygame.display.set_caption("Lee's Maze Router")
        cols, rows = len(layers[0]), len(layers[0][0])
        cell, layer, sx, sy = max(2, min(40, 900//cols, 600//rows)), 0, 0, 0
        clock = pygame.time.Clock()
        running = True
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1 and event.pos[1] < 45:
                    layer = 1-layer
                elif event.type == pygame.KEYDOWN:
                    if event.key in (pygame.K_ESCAPE, pygame.K_q):
                        running = False
                    elif event.key == pygame.K_l:
                        layer = 1-layer
                    elif event.key in (pygame.K_PLUS, pygame.K_EQUALS):
                        cell = min(80, cell+2)
                    elif event.key == pygame.K_MINUS:
                        cell = max(2, cell-2)
                    elif event.key == pygame.K_RIGHT:
                        sx += cell*4
                    elif event.key == pygame.K_LEFT:
                        sx -= cell*4
                    elif event.key == pygame.K_DOWN:
                        sy += cell*4
                    elif event.key == pygame.K_UP:
                        sy -= cell*4
            w, h = screen.get_size()
            sx, sy = max(0, min(sx, cols*cell-w)), max(0, min(sy, rows*cell-(h-45)))
            render(screen, pygame, layers, names, layer, cell, (sx, sy))
            pygame.display.flip()
            if args.screenshot:
                pygame.image.save(screen, args.screenshot)
                break
            clock.tick(30)
    finally:
        pygame.quit()
    return 0 if all(r['routed'] for r in results) else 2


if __name__ == '__main__':
    sys.exit(main())
