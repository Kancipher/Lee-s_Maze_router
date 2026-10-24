# Engineering review — 24 October 2026

## Original assessment

Baseline: `main` at `b51b00bb90a859fc9af1b2ba5f4e90e113738742`. Reviewed the main C++ implementation, header, CLI, binding, GUI, CMake, README, fixtures and original report; also compared the alternative `GUI` branch. The animation branch uses queue-based exploration despite unequal movement costs, whereas main uses a priority queue. It is an experimental alternative and was not merged.

Strengths: an appropriate two-layer routing problem, weighted search, net ordering, pybind11 integration and useful visualization material. Weaknesses prevent treating the original CLI and exported results as reliable.

Scores are engineering/recruiter judgments on a 1–10 scale, not measurements or certification.

| Dimension | Original | Reviewed branch |
|---|---:|---:|
| Technical quality | 3 | 7 |
| Security/input robustness | 3 | 7 |
| Reproducibility | 2 | 8 |
| Testing | 1 | 8 |
| Documentation | 4 | 8 |
| Cybersecurity career relevance | 4 | 5 |
| Mean | 2.8 | 7.2 |

## Confirmed defects, in priority order

1. **Broken CLI build:** `main.cpp` called the two-argument `readfile` with one argument and included the implementation file. Reproduced with g++: compilation exit 1.
2. **Unchecked parsing and grid access:** malformed/out-of-bounds pins and overlapping cells were accepted without a validation boundary. Negative/overflowing costs invalidated shortest-path assumptions.
3. **Unreachable paths and state corruption:** predecessor reconstruction inferred a path from costs and could mark partial paths without proving source reachability. Globals retained nets between runs.
4. **Export identity errors:** per-segment paths were associated with original per-net names after routing reordering; source cells and multipin paths were incomplete.
5. **Shared-tree routing:** previous segments of the same net were treated as occupied; there was no explicit per-net transaction or consistent ownership model.
6. **Rectangular GUI and allocation:** x/y dimensions were reversed in main's renderer, and whole-grid surfaces and an uncapped draw loop consumed resources unnecessarily.
7. **Reproduction gaps:** no automated tests; stale declarations; platform-specific extension-copy instructions; no normal CMake CLI target; unchecked file operations.

Optional improvements: rip-up/reroute, animation from the experimental branch, physical design-rule checks, congestion-aware ordering, global optimization and benchmarking. These are not represented as repaired defects or implemented features.

## Implemented changes

- `src/functions.hpp`, `functions.cpp`: one consistent public contract, bounded line-aware parser, reserved pins/obstacles, ownership checks, 64-bit distances, deterministic multi-source Dijkstra, explicit predecessors, complete per-net exports, failure rollback and fresh state per successful run.
- `src/main.cpp`: separately linked CLI, validated costs, error/status reporting and exit codes.
- `src/binding.cpp`: existing entry points retained; `get_results` adds explicit name/status/cost/cells.
- `src/gui.py`: import-safe entry point, correct rectangular indexing, visible-cell rendering, bounded zoom and frame rate, deterministic screenshot mode.
- Root `CMakeLists.txt`, compatibility `src/CMakeLists.txt`, `requirements-gui.txt`: CLI/core/module build and CTest; no platform-specific binary copying.
- `tests/test_router.py`, `test_binding.py`, `rectangular.txt`: independent oracle and regression fixtures.
- `.github/workflows/tests.yml`: native, sanitizer, module and headless rendering jobs with read-only repository permissions.
- `README.md`, `docs/demo.png`, `.gitignore`: reproducible setup, authentic demonstration, limits, attribution and clean generated-file handling.

## Executed checks

Linux, g++ 13.3, Python 3.12.14, CMake installed from PyPI, pybind11 3.1.0 and Pygame 2.6.1.

| Check | Actual result |
|---|---|
| Original CLI compilation | Failed: missing readfile argument |
| CMake CLI + extension build | Passed |
| CTest | Passed: 7 Python test methods, including 80 seeded independent shortest-path cases |
| ASan + UBSan CTest | Passed with `ASAN_OPTIONS=detect_leaks=0` |
| LeakSanitizer | Could not run: environment denied `/proc` inspection; no leak-free claim |
| Extension state/rollback tests | Passed: 2 methods |
| Pygame rectangular screenshot mode | Passed; actual image in `docs/demo.png` |
| Hosted Ubuntu native, ASan/UBSan and Python/GUI CI | Passed on GitHub Actions |
| Nine historical input fixtures | All accepted; 2 and 8 each had one unreachable net; others routed completely |

The original output fixtures were not used as test oracles or replaced with self-generated expected answers. The single-net oracle checks optimal weighted cost, connectedness, bounds, obstacle avoidance and endpoint inclusion. Multipin/multinet tests check important properties; they do not prove global optimality. Interactive keyboard/fullscreen behavior, Windows/macOS builds and production-scale performance have not been fully verified here. CI status must be checked on the pull request separately; local passing checks are not hosted CI results.

## Recruiter use

Best fits: junior security engineering, secure C++ development, application-security tooling; supporting evidence for general software engineering. It provides little direct SOC, incident-response, cloud IAM or GRC evidence. Feature it as a secondary project when demonstrating algorithms/native-code robustness, alongside a more directly security-focused project.

Resume candidates (team-aware; use only after reviewing and understanding the assisted improvements):

1. Co-developed a two-layer C++ VLSI maze router with layer-dependent movement costs, via penalties and a Python/Pygame visualization interface.
2. Hardened routing input validation and result handling with bounded coordinates, 64-bit distances, explicit predecessor tracking and failure-safe per-net tree construction.
3. Added automated routing regressions using an independent shortest-path oracle across 80 seeded boards, native sanitizer checks and Python-extension state tests.

Interview explanation: “Our team modeled VLSI routing as a weighted graph over two layers. Each layer prefers a different direction, and changing layers has a via cost. Dijkstra connects pins around obstacles; the revised implementation attaches additional pins to the current tree and reserves other nets' cells. The difficult part was separating search results from committed board state so an unreachable pin could not create a false route. Tests compare single-net costs against a separate Python implementation. Sequential net ordering is a heuristic, so I would next add rip-up/reroute and benchmark congestion cases.” Explain your original component ownership separately; this review does not establish that you wrote every component unaided.
