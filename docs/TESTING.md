# Testing and verification evidence

[Overview](../README.md) · [Build](BUILD.md) · [Demo](DEMO.md) · [Algorithm](ALGORITHM.md) · [Format and limits](FORMAT.md)

## Native CLI regressions

```bash
ctest --test-dir build --output-on-failure
```

CTest runs [tests/test_router.py](../tests/test_router.py) against the built CLI. It contains seven test methods; one method compares 80 seeded single-net boards against a separate Python Dijkstra oracle.

| Coverage | Evidence checked |
|---|---|
| Independent weighted-path oracle | Optimal cost, endpoint inclusion, connectedness, bounds and obstacle avoidance on 80 seeded boards |
| Input/cost validation | Invalid dimensions/layers/coordinates, overlaps, duplicate names, malformed text, long lines and out-of-range costs |
| Multipin/name association | Correct net identity after ordering, same-net tree reuse and no cross-net cell overlap in the targeted case |
| Failure behavior | An unreachable multipin net exports no partial track |
| Output preservation | An existing output file is not overwritten |
| Historical fixtures | Nine original input fixtures complete without crash; valid unreachable results are allowed |

The independent oracle verifies single-net cases. It does not prove global optimality for multipin/multinet routing or provide a production-scale benchmark. Historical output text is not used as the oracle.

## Extension-state regressions

After the module build:

```bash
PYTHONPATH=build python tests/test_binding.py
```

The two methods in [test_binding.py](../tests/test_binding.py) cover repeated successful calls with different board dimensions/names, invalid-input state preservation, invalid costs and an unreachable tree retaining only its original pins. These checks exercise the extension's persistent process state.

## Sanitizers and hosted checks

```bash
cmake -S . -B build-sanitize -DENABLE_SANITIZERS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-sanitize --parallel 2
ctest --test-dir build-sanitize --output-on-failure
```

The existing [Actions workflow](../.github/workflows/tests.yml) runs native builds with sanitizers OFF and ON, plus a separate Python-module job with extension tests and headless rectangular rendering. Compiler support enables AddressSanitizer and UndefinedBehaviorSanitizer. Repository permissions are read-only for these jobs.

The [24 October engineering review](REVIEW.md) records the earlier execution. Its local sanitizer run passed with leak detection disabled because that environment could not inspect `/proc`; it makes no local leak-free claim. Hosted Ubuntu sanitizer jobs also passed in that review. Consult the [current Actions page](https://github.com/Kancipher/Lee-s_Maze_router/actions) or the presentation PR's Checks tab for the latest run. No static passing/security badge is added.

## Presentation run — 30 October 2026

The unchanged implementation was rebuilt on Linux with g++ 13.3, Python 3.12, pybind11 3.1.0 and Pygame 2.6.1.

| Executed check | Result |
|---|---|
| CMake CLI and Python-module build | Passed |
| CTest regression entry point | Passed; seven methods including 80 seeded oracle cases |
| Python extension tests | Passed; two methods |
| Rectangular CLI demonstration | Both nets routed; exit 0 |
| Historical Testcase 2 demonstration | One net unreachable, one routed; exit 2 |
| Headless GUI rendering | Passed; pixels identical to the existing `docs/demo.png` |

The captured [CLI transcript](demo/cli-transcript.txt) and [demo walkthrough](DEMO.md) connect the displayed examples to executed behavior. Application sources, tests, dependencies, CMake and workflow definitions were not changed for this presentation pass. Local sanitizer tests were not rerun merely for Markdown changes; hosted CI exercises its existing sanitizer job.

Interactive keyboard behavior, Windows/macOS builds, electrical design rules, adversarial runtime isolation and large-scale performance remain outside this run's verified coverage. See [FORMAT.md](FORMAT.md#local-tool-security-boundary) for filesystem and concurrency limits.
