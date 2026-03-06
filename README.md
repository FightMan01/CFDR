# CFDR: CFD-Rendering Toolkit

![Build](https://github.com/sze-hidalgo-2/CFDR/actions/workflows/ci.yml/badge.svg)

CFDR is a scientific data-visualization program developed and maintained by Matyas Constans,
with the primary aim to visualize simulation results in different ways in order to get
a good understanding of the data.

It was built on top of the [ALICE Engine](https://github.com/matt-const/alice), by the same author.

It is available both as a native desktop application and web-based visualization tool that can be
integrated in other workflows.

Visualizations can scripted and configured through it's own language (.CFDR files).

It was developed during the [HidALGO 2 project](http://hidalgo2.eu/), funded by the EU.

## Build Instructions

### Windows
Requires `MSVC` to be installed (`cl`).
Build with ``./build.bat [build-flags]``

### Linux, Macos
Requires `clang` to be installed.
Build with ``./build.sh [build-flags]``

### Web (WASM + JS)
Requires `clang` and `lld` to be installed.
Build with: ``./build.sh web [build-flags]``

You can run the web-page locally, by running a simple http server,
by running:

``cd build/ && python -m http.server 8080``

Then accessing the URL ``localhost:8080`` in the browser of your choice.

### Optional Build Flags

`debug`: Run an un-optimized debug build (-O0, debug symbols).
`portal`: Build with support for the SZE-portal, using keycloak for authentification.
