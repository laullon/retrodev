# Contributing to Retrodev

Retrodev is open to contributions of any kind. This document describes the ways you can help.

## How to submit a contribution

All contributions go through the project Git repository. Two workflows are available:

- **Bug reports and suggestions** — open an issue in the repository tracker. Describe what you observed, what you expected, and the steps to reproduce (for bugs) or the rationale (for suggestions).
- **Code and content contributions** — fork the repository, apply your changes on a feature branch, then open a pull request against `master`. Keep each pull request focused on a single topic and reference any related issue in the description.

---

## Report bugs and suggest features

The simplest contribution is using the tool and reporting what you find. If something does not work as documented, or you have an idea for an improvement, open an issue. Good reports include:

- The target system and mode you were working with.
- A minimal reproduction case (project file, source file, or screenshot).
- The exact error message or unexpected behaviour.

---

## Create and submit export scripts

Export scripts are AngelScript (`.as`) files that convert bitmaps, sprites, tiles, or maps into the byte format a specific system or loader expects. The SDK ships a set of them under `sdk/amstrad.cpc/exporters/`; adding your own script to that collection makes it available to every user.

See [Export Scripts](../usage/export-scripts.md) for the API and metadata tag format. When your script is ready, submit it via pull request to the `sdk/amstrad.cpc/exporters/` tree.

---

## Create and submit macros and library routines

Z80 assembler macros and reusable library routines live under `sdk/amstrad.cpc/macros/` and `sdk/amstrad.cpc/functions/`. If you write a routine that is general enough to be useful across projects — CPC interrupt setup, sprite unrolling, fixed-point maths, bank switching — it belongs there.

Submit additions via pull request.

---

## Port Retrodev to another platform

Retrodev already has Linux and macOS emulator-spawn implementations (`posix_spawn`). The main remaining work for a new platform port is the application entry point and the system linker library list in the build scripts. The Kombine build scripts automatically include or exclude platform-specific source files based on folder naming (`/win/`, `/lnx/`, `/osx/`), so adding a new platform requires no changes to the filtering logic.

See [Porting](porting.md) for the full breakdown of what exists and what is missing before opening a pull request.

---

## Extend Retrodev to support another system

New target systems can be added incrementally. The converter dispatcher, palette type registry, and mode table are all designed to accept new entries without touching unrelated code. The Amstrad CPC implementation is the reference; the ZX Spectrum, Commodore 64 and MSX slots are already stubbed in the dispatcher.

See [Adding a New System](adding-systems.md) for the step-by-step instructions before opening a pull request.
