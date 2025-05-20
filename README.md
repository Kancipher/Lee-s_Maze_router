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

## Output Format

The output from the router is a text file that lists the cells used by each net. Each line of this file follows the format:

```
Net_name (cell_1_layer, cell_1_x, cell_1_y) (cell_2_layer, cell_2_x, cell_2_y) ...
```

The cells represent the complete path taken by each net, starting from a pin and connecting to all other pins.

### Example Output:
```
net1 (1, 10, 20) (1, 11, 20) (1, 12, 20) ...
net2 (2, 100, 200) (2, 100, 201) (2, 100, 202) ...
```

## How to Use

### Setup

1. Clone the repository from GitHub:
   ```
   git clone https://github.com/Kancipher/Lee-s_Maze_router.git
   cd Lee-s_Maze_router
   ```

2. Install any required dependencies and build the project:
   ```bash
   # Make sure you have Python 3, CMake, and a C++ compiler installed
   # Initialize submodules (for pybind11)
   git submodule update --init --recursive

   # Build the Python extension module
   cd src
   mkdir -p build
   cd build
   cmake -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE=$(which python3) ..
   make

   # Copy the built .so file to the src directory for Python to use
   cp routing.cpython-*-darwin.so ../
   cd ../..
   ```

### Running the GUI Router

1. Create an input file using the format described above (e.g., `input.txt`).

2. Run the GUI with your input file, output file, and specify via and direction costs:
   ```bash
   cd src
   python3 gui.py ../tests/Testcase6/input.txt ../tests/Testcase6/output.txt via=500 direction=20
   ```
   - Replace `via=500` and `direction=20` with your desired costs.
   - If you do not specify `via=...` or `direction=...`, the defaults are 10 for both.
   - The GUI will visualize the routing with the specified costs.

