# Source Code Layout

See the summary in [readme.md](../../readme.md#9-technical-information) for a quick overview.

## Top-level directories

| Directory | Contents |
|---|---|
| `src/` | Application source code |
| `ext/` | Third-party libraries (vendored) |
| `bld/` | Build scripts and code-generation helpers |
| `doc/` | Documentation |
| `sdk/` | Reusable Z80 macros and library code shipped with the tool |
| `examples/` | Ready-to-build example projects |

## `src/` in detail

### `src/gui/`

All ImGui-based UI code. Nothing in `src/gui/` has any direct dependency on hardware conversion logic — it calls into `src/lib/` for all data operations.

- `src/gui/app/` — application window and SDL lifecycle.
- `src/gui/views/bitmaps/` — bitmap conversion editor: conversion preview, parameter panels, export widget.
- `src/gui/views/build/`
- `src/gui/views/data/` — hex/data viewer using the embedded hex editor widget.
- `src/gui/views/image/` — image viewer with zoom and colour picker.
- `src/gui/views/maps/` — tile map editor: canvas, layers, tilesets, groups, scrollbars, aspect ratio preview.
- `src/gui/views/palette/` — palette solver editor.
- `src/gui/views/sprites/` — sprite extraction editor.
- `src/gui/views/text/` — code editor (wraps `ext/imgui.ext/imgui.text.editor`).
- `src/gui/views/tiles/` — tile extraction editor.
- `src/gui/views/main.view.document.*` — base class for all document views (tab, modified flag, name).
- `src/gui/views/main.view.documents.*` — document container (tab bar, open/close logic).

### `src/lib/`

Platform-independent core library. No ImGui, no SDL rendering — only SDL file I/O for cross-platform file access.

- `src/lib/assets/` — image, map, palette and source asset types.
- `src/lib/convert/` — bitmap / tile / sprite converter interfaces and per-system implementations. Adding a new system means adding a subfolder here.
- `src/lib/export/` — AngelScript export engine, script metadata parser, and script bindings for all context types.
- `src/lib/process/image/` — image processing passes: dithering, quantization, colour correction, resize.
- `src/lib/project/` — project load/save (Glaze JSON), build item management, project API used by the GUI.
- `src/lib/system/` — system-specific hardware definitions (modes, resolutions, palette types) and pixel aspect ratio data.
- `src/lib/log/` — lightweight logging facility.

## `ext/` in detail

| Directory | Library | Notes |
|---|---|---|
| `ext/imgui/` | Dear ImGui | Fetched dependency — do not modify |
| `ext/imgui.cfg/` | ImGui user config | Retrodev's `retrodev.imconfig.h` user configuration for ImGui |
| `ext/imgui.ext/` | ImGui extensions | Extensions created and modified for Retrodev |
| `ext/rasm/` | RASM Z80 assembler | Fetched dependency — do not modify |
| `ext/rasm.ext/` | RASM API wrapper | Retrodev's C++ wrapper around the RASM C API |
| `ext/ascript/` | AngelScript | Fetched dependency — do not modify |
| `ext/sdl/` | SDL3 | Fetched dependency (full source) — built as part of the dependency build step — do not modify |
| `ext/sdl.img/` | SDL3_image | Fetched dependency (full source) — built as part of the dependency build step — do not modify |
| `ext/freetype/` | FreeType | Fetched dependency (full source) — built as part of the dependency build step — do not modify |
| `ext/glaze/` | Glaze | Fetched dependency (header-only JSON library) — do not modify |
| `ext/ctre/` | CTRE | Fetched dependency (header-only regex) — do not modify |

## `sdk/` in detail

The SDK ships alongside the application binary (synced by the build step into the output directory). It is organised by target system. All paths within Retrodev are stored with a `$(sdk)/` prefix and resolved at runtime.

### `sdk/amstrad.cpc/`

| Directory | Contents |
|---|---|
| `sdk/amstrad.cpc/exporters/` | Ready-to-use AngelScript export scripts for CPC graphics data |
| `sdk/amstrad.cpc/macros/` | Z80 assembler macros for CPC hardware registers |
| `sdk/amstrad.cpc/functions/` | Reusable Z80 library routines |

**Exporters** (`sdk/amstrad.cpc/exporters/`):

| File | Description |
|---|---|
| `cpc.screen.as` | Exports bitmap data in CPC screen memory layout (chunky or native, with optional palette file) |
| `cpc.tiles.as` | Exports extracted tiles as packed CPC pixel bytes |
| `cpc.sprites.as` | Exports extracted sprite regions as packed CPC pixel bytes |
| `map.binary.as` | Exports map layers as binary cell arrays (optional width/height header) |
| `cpc.utils.as` | Shared utility functions (`EncodePixels`, `EncodeByte`) — tagged `@exporter util`, included by other scripts |

**Macros** (`sdk/amstrad.cpc/macros/`):

| File | Description |
|---|---|
| `macros/crtc/macros.crtc.asm` | CRTC register macros |
| `macros/gatearray/macros.ga.asm` | Gate Array register macros |

**Functions** (`sdk/amstrad.cpc/functions/`):

| File | Description |
|---|---|
| `functions/keyboard/cpc.keyboard.asm` | Keyboard scanning routine |

## `examples/` in detail

Ready-to-build example Retrodev projects. Each subdirectory is a self-contained project with source graphics, assembler source and pre-built output.

| Directory | Description |
|---|---|
| `examples/screen/` | Full-screen bitmap conversion and export to CPC screen memory |
| `examples/tiles/` | Tile extraction and binary tile export |
| `examples/sprites/` | Sprite extraction and binary sprite export |
| `examples/maps/` | Multi-layer map creation and binary map export |
| `examples/palette/` | Palette solver with multiple participants across zones |
