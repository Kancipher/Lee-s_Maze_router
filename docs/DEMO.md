# Demo: weighted routing across two layers

[Overview](../README.md) · [Build](BUILD.md) · [Algorithm](ALGORITHM.md) · [Format](FORMAT.md) · [Testing](TESTING.md)

The examples below were executed on 30 October 2026 using the unchanged main implementation. The existing image is a real Pygame rendering, reproduced pixel-for-pixel during this pass. The CLI [transcript](demo/cli-transcript.txt) preserves actual commands, output and exit statuses.

## Rectangular multipin example

Build the CLI, then run from the repository root:

```bash
./build/maze-router tests/rectangular.txt build/presentation-rectangular.txt 10 10
```

Use a fresh output filename if that path already exists. Actual standard output:

```text
netA: ROUTED cost=51 cells=25
netB: ROUTED cost=23 cells=6
```

Exit status: `0`. The [input](../tests/rectangular.txt) is an 8-row, 20-column board with a three-cell obstacle, a three-pin net across layers and a second two-pin net.

![Actual layer-1 rendering of the rectangular fixture](demo.png)

Red cells show pins unless rendered as vias. Colored cells identify routed nets. Dark squares are obstacles; dark cells with white rings are vias. The image shows layer 1; press L or click the header in the viewer to inspect layer 2. Other controls are in [BUILD.md](BUILD.md#viewer-controls-and-legend).

### What the cost illustrates

`netB` connects two layer-2 pins separated by three horizontal steps. Staying on layer 2 would cost 3 × 10 = 30. Changing to layer 1, taking three preferred horizontal steps and changing back costs 10 + 3 + 10 = 23, matching the captured result.

This demonstrates the tradeoff between a via penalty and nonpreferred movement. The broader net/pin ordering remains a heuristic; it is not a global-optimality claim.

## Unreachable-net example

```bash
./build/maze-router tests/Testcase2/input.txt build/presentation-unroutable.txt 10 10
```

Actual standard output:

```text
net0: UNROUTABLE cost=0 cells=0
net2: ROUTED cost=39 cells=22
```

Exit status: `2`. This is a valid board with an unreachable net, not an input/parser failure. The output file identifies the failed net explicitly; its zero cost/cell summary means no track was committed for it. See [the fixture](../tests/Testcase2/input.txt) and [output semantics](FORMAT.md#output-semantics).

## Reproduce the image

After building the optional module:

```bash
PYTHONPATH=build SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  python src/gui.py tests/rectangular.txt screenshot-route.txt --screenshot router.png
```

This runs the native core and Pygame using SDL's dummy display. It is an authentic local renderer output, not an interactive-session recording. The image is retained at its original path, `docs/demo.png`, to preserve existing links. No application code, fixture, historical output or image bytes were changed for the presentation refresh.
