# Lee's Maze Router

## Input Format

The input to the router is a text file that specifies the size of the grid, the obstacles, and lists the nets to be routed.

### The input format follows the structure:

1. **Grid Size**: The first line specifies the size of the grid in the format `rows x cols`.  
   Example:
   ```
   100x200
   ```

2. **Obstacles**: Each obstacle is specified with the keyword `OBS`, followed by the coordinates of the obstacle in the format `(x, y)`. Obstacles exist on both routing layers.
   Example:
   ```
   OBS (33, 44)
   OBS (55, 77)
   ```

3. **Nets**: Each net is defined with the net name followed by pairs of coordinates for each pin. Each pin is specified as `(pin_layer, pin_x, pin_y)` where `pin_layer` is either 1 (for layer 1) or 2 (for layer 2).  
   Example:
   ```
   net1 (1, 10, 20) (2, 30, 50) (1, 5, 100)
   net2 (2, 100, 200) (1, 300, 50)
   net3 (1, 100, 50) (2, 300, 150) (2, 50, 50) (1, 2, 2)
   ```

### Example Input:
```
100x200
OBS (33, 44)
OBS (55, 77)
net1 (1, 10, 20) (2, 30, 50) (1, 5, 100)
net2 (2, 100, 200) (1, 300, 50)
net3 (1, 100, 50) (2, 300, 150) (2, 50, 50) (1, 2, 2)
```

