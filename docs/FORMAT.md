# Input, output and operating limits

[Overview](../README.md) · [Build](BUILD.md) · [Demo](DEMO.md) · [Algorithm](ALGORITHM.md) · [Testing](TESTING.md)

## Input example

The executed [rectangular fixture](../tests/rectangular.txt):

```text
8x20
OBS (8,2)
OBS (8,3)
OBS (8,4)
netA (1,1,2)(1,18,2)(2,18,6)
netB (2,2,6)(2,5,6)
```

- The first nonempty line is `rows x columns`. Here x ranges from 0 to 19 and y from 0 to 7.
- `OBS (x,y)` blocks the position on both layers.
- Pins use `(layer,x,y)`, with layers 1 and 2. Every net requires at least two distinct pins.
- Names begin with `net`, optionally followed by letters, digits, underscores or hyphens; maximum total length is 64 characters.
- Empty lines and whitespace are accepted. Unknown records/text, malformed coordinates, duplicate net names, overlapping pins and obstacle/pin conflicts are rejected.

## Resource and cost bounds

| Item | Limit |
|---|---:|
| Input file | 1 MiB |
| Line length, checked before trimming | 4,096 bytes, excluding the line-feed delimiter |
| Grid | 1–100,000 cells per layer |
| Nets | 256 |
| Total pins | 2,048 |
| Via / nonpreferred cost | 1–1,000,000 |
| Preferred planar step | Cost 1 |

The CLI syntax is:

```text
maze-router INPUT OUTPUT [VIA_COST NONPREFERRED_COST]
```

The two optional costs must be provided together; defaults are 10 and 10. The GUI uses `via=10 direction=10` positional strings for the same settings. Choose a fresh output path; existing files and the input file are rejected as output destinations.

## Output semantics

Each line begins with the original net name. A routed net lists its unique connected cells as `(layer,x,y)`, sorted by layer, x and y. All connected pins are included. A multipin tree can branch, so the list is a cell set rather than an ordered walk.

An unreachable net is explicit:

```text
net0 UNROUTABLE
```

The CLI also prints a per-net status, accumulated branch cost and cell count. A failed net reports zero committed route cost/cells; its original pins remain reserved in the grid. A zero here does not indicate a successful zero-cost connection.

| Exit status | Meaning |
|---|---|
| `0` | All nets routed |
| `2` | Valid board with at least one unreachable net; output identifies the failed net(s) |
| `1` | Invalid input/costs, wrong arguments or I/O failure |

Historical `tests/Testcase*/output.txt` files and `src/output.txt` remain source material. They are not correctness oracles and are not overwritten by the demonstration commands. The earlier review records unreachable nets in Testcases 2 and 8; the presentation demo reran Testcase 2 and captured exit status 2.

## Local-tool security boundary

Validation and positive costs constrain native memory/indexing risks; they do not isolate a hostile process or guarantee a runtime limit. Congested boards can still require substantial CPU time. Apply an external time budget when running untrusted batch inputs.

The extension uses process-wide state and is not designed for concurrent routing calls. Output handling assumes a trusted local filesystem: the existence check is not race-resistant sandbox enforcement, and a write failure may leave an incomplete new output file. This tool does not implement authentication or network protocol handling.
