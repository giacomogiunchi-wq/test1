# Duomec Platform

Duomec is an open-source, modular mechanical CAD/CAE application. This repository currently contains **Milestone 0 only**: stable native interfaces, core infrastructure, and an optional Qt 6/Open CASCADE desktop shell that displays and selects topology on a box.

## Build the CI-friendly core

```bash
cmake -S . -B build -DDUOMEC_BUILD_DESKTOP=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Build the desktop acceptance slice

Install the pinned Qt and OCCT development packages described in `DEPENDENCIES.md`, then:

```bash
cmake -S . -B build-desktop -DDUOMEC_BUILD_DESKTOP=ON
cmake --build build-desktop --parallel
./build-desktop/duomec
```

The viewport displays an OCCT box. Drag the middle button to pan, Shift+left-drag to orbit, use the wheel to zoom, and left-click a face or edge to display its topology type in the status bar.

## Design constraints

* C++20, SI units internally, deterministic numerical engines, and no owning raw pointers.
* Third-party engines exist only behind adapters; public interfaces use Duomec domain types.
* Later milestones are intentionally represented by empty package boundaries and disabled feature flags—not partial implementations.

See [`docs/architecture/architecture-brief.md`](docs/architecture/architecture-brief.md), [`docs/architecture/component-diagram.md`](docs/architecture/component-diagram.md), and [`docs/adr`](docs/adr).
