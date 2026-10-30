# Build and run

[Overview](../README.md) · [Demo](DEMO.md) · [Algorithm](ALGORITHM.md) · [Format](FORMAT.md) · [Testing](TESTING.md)

## Prerequisites

- A C++17 compiler and CMake 3.20+.
- Python 3.10+ for the default CTest harness. The CLI executable has no Python runtime dependency.
- For the optional viewer: the pinned pybind11 and Pygame packages in [requirements-gui.txt](../requirements-gui.txt), plus Python development headers/libraries for building the native extension.

The recorded presentation run used Linux, g++ 13.3 and Python 3.12. Windows/macOS equivalents are guidance; they were not executed in this pass.

## CLI build

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
./build/maze-router tests/rectangular.txt route-result.txt 10 10
ctest --test-dir build --output-on-failure
```

`10 10` specifies the via cost followed by the nonpreferred-direction cost. Both default to 10 if omitted. Use a new output filename for each run. See [format and exit statuses](FORMAT.md) to distinguish an unreachable net from an input error.

For an executable-only build without the Python test harness:

```bash
cmake -S . -B build-cli -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-cli --parallel 2
```

This optional configuration follows the existing CMake switches; the default build was the one executed for the presentation demo.

## Python extension and GUI

On Linux/macOS:

```bash
python -m venv .venv
. .venv/bin/activate
pip install -r requirements-gui.txt
cmake -S . -B build -DBUILD_PYTHON=ON \
  -Dpybind11_DIR="$(python -m pybind11 --cmakedir)" \
  -DPython_EXECUTABLE="$(which python)"
cmake --build build --parallel 2
PYTHONPATH=build python src/gui.py tests/rectangular.txt gui-result.txt via=10 direction=10
```

CMake discovers the installed pybind11 package. Initializing and copying the historical submodule is not required by this build; its pinned git reference is retained for provenance. Set `PYTHONPATH` to the directory containing the built `routing` module rather than copying a platform-specific binary into `src/`.

On Windows, a multi-configuration generator typically places the executable and `.pyd` under `build/Release/`. Build with `--config Release` and set `PYTHONPATH` to that module directory. These platform notes have not been validated on a Windows runner in this pass.

### Viewer controls and legend

| Control or cell | Meaning |
|---|---|
| L or click the header | Switch between layers |
| Arrow keys | Pan the viewport |
| + / - | Increase/decrease cell size |
| Q or Escape | Close the viewer |
| Red cell | Pin, unless the cell is rendered as a via |
| Colored cell | Routed track; the palette identifies the net |
| Dark square | Obstacle |
| Dark cell with white ring | Via connecting the layers |

The viewer draws only visible cells, bounds cell size and caps the loop at 30 FPS. The image shows one layer at a time. The alternative `GUI` branch contains an experimental animation implementation and remains separate from the main implementation.

## Headless rendering

After building the module:

```bash
PYTHONPATH=build SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  python src/gui.py tests/rectangular.txt screenshot-route.txt --screenshot router.png
```

This executes the native router and Pygame renderer using SDL's dummy display, saves one frame and exits. It verifies a rendering path, not interactive keyboard behavior or a hosted service. The existing [demo image](demo.png) was reproduced pixel-for-pixel in the 30 October presentation run.

## Development

Run the CLI and extension checks after a routing change. Add a fixture with an independently justified property for a new bug; avoid treating regenerated program output as its own correctness oracle. Keep generated builds and local outputs out of version control. Preserve the original report and team attribution. See [testing](TESTING.md) for coverage and verification limits.
