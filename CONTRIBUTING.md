# Contributing

Contributions should preserve the portable C11 production core and its frozen
algorithm behavior. Production code must remain heap-free, non-recursive,
deterministic, bounded and free of mutable global state.

Before submitting a change:

- add or update tests for new behavior;
- run the clean CMake build and CTest suite;
- use strict compiler warnings without hiding diagnostics;
- preserve the documented C-1.0, H-1.0 and H2-1.0 behavior;
- use a new algorithm version for intentional behavior changes.

Research code belongs in the opt-in research area and must not leak into the
default production library or public headers.
