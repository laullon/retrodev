# RASM API Reference

Documentation for the public API defined in `ext/rasm/rasm.h`:
the assembler entry-point functions, the debug info structs returned by them,
and the `s_parameter` struct used to configure assembly behaviour.

---

## API Functions

All functions assemble Z80 source text supplied as an in-memory string.
They are safe to call from C++; the header guards the declarations with
`#ifndef INSIDE_RASM`.

### Return value convention

Every `RasmAssemble*` function returns:
- `0` — assembly succeeded with no errors.
- negative (`-1`) — assembly failed; one or more errors are present. Check `s_rasm_info.nberror` and `s_rasm_info.error[]` for details.

When called with `datain == NULL` and `lenin == 0` the function returns an internal
call counter (useful for diagnostics) and does nothing else.

---

### `RasmAssemble`

```c
int RasmAssemble(const char *datain, int lenin,
                 unsigned char **dataout, int *lenout);
```

Minimal entry point. Assembles `datain` (length `lenin`) with default settings.

| Parameter | Direction | Description |
|---|---|---|
| `datain` | in | Source text to assemble. |
| `lenin` | in | Byte length of `datain`. |
| `dataout` | out | Receives a pointer to the assembled binary. Caller must free. Pass `NULL` to discard. |
| `lenout` | out | Receives the byte length of `*dataout`. Pass `NULL` to discard. |

No error detail is available with this variant. Use `RasmAssembleInfo` if you need diagnostics.

---

### `RasmAssembleInfo`

```c
int RasmAssembleInfo(const char *datain, int lenin,
                     unsigned char **dataout, int *lenout,
                     struct s_rasm_info **debug);
```

Same as `RasmAssemble` but also returns a populated `s_rasm_info` struct with
error messages and exported symbols.

| Parameter | Direction | Description |
|---|---|---|
| `datain` | in | Source text to assemble. |
| `lenin` | in | Byte length of `datain`. |
| `dataout` | out | Receives a pointer to the assembled binary. Caller must free. |
| `lenout` | out | Receives the byte length of `*dataout`. |
| `debug` | out | Receives a pointer to a heap-allocated `s_rasm_info`. Must be freed with `RasmFreeInfoStruct`. |

---

### `RasmAssembleInfoParam`

```c
int RasmAssembleInfoParam(const char *datain, int lenin,
                          unsigned char **dataout, int *lenout,
                          struct s_rasm_info **debug,
                          struct s_parameter *param);
```

Full-featured entry point. Same as `RasmAssembleInfo` but accepts an
`s_parameter` struct to control every assembler option (output filenames,
compatibility modes, symbol export, etc.). See the `s_parameter` reference
below for all fields.

| Parameter | Direction | Description |
|---|---|---|
| `datain` | in | Source text to assemble. |
| `lenin` | in | Byte length of `datain`. |
| `dataout` | out | Receives a pointer to the assembled binary. Caller must free. |
| `lenout` | out | Receives the byte length of `*dataout`. |
| `debug` | out | Receives a pointer to a heap-allocated `s_rasm_info`. Must be freed with `RasmFreeInfoStruct`. |
| `param` | in | Assembler configuration. All pointer fields must be `NULL` when unused; all integer fields must be `0` for defaults. |

---

### `RasmAssembleInfoIntoRAM`

```c
int RasmAssembleInfoIntoRAM(const char *datain, int lenin,
                             struct s_rasm_info **debug,
                             unsigned char *emuram, int ramsize);
```

Assembles directly into a caller-supplied RAM buffer. Intended for emulator
integration: the assembled code is written into `emuram` at the addresses
specified by `ORG` directives in the source. No separate binary output pointer
is returned.

| Parameter | Direction | Description |
|---|---|---|
| `datain` | in | Source text to assemble. |
| `lenin` | in | Byte length of `datain`. |
| `debug` | out | Receives a pointer to a heap-allocated `s_rasm_info`. Must be freed with `RasmFreeInfoStruct`. |
| `emuram` | in/out | Caller-allocated buffer representing the emulator's RAM. Pre-populated with existing RAM contents; overwritten at assembled addresses. |
| `ramsize` | in | Byte size of `emuram`. The assembler maps 16 KB banks into this buffer. |

After assembly the buffer is read back into `s_rasm_info.emuram` / `s_rasm_info.lenram`.

---

### `RasmAssembleInfoIntoRAMROM`

```c
int RasmAssembleInfoIntoRAMROM(const char *datain, int lenin,
                                struct s_rasm_info **debug,
                                unsigned char *emuram, int ramsize,
                                unsigned char *emurom, int romsize);
```

Same as `RasmAssembleInfoIntoRAM` but also provides a ROM buffer for
cartridge / ROM bank output. The assembled code that targets ROM addresses is
written into `emurom`.

| Parameter | Direction | Description |
|---|---|---|
| `datain` | in | Source text to assemble. |
| `lenin` | in | Byte length of `datain`. |
| `debug` | out | Receives a pointer to a heap-allocated `s_rasm_info`. Must be freed with `RasmFreeInfoStruct`. |
| `emuram` | in/out | Emulator RAM buffer (see `RasmAssembleInfoIntoRAM`). |
| `ramsize` | in | Byte size of `emuram`. |
| `emurom` | in/out | Caller-allocated buffer representing the emulator's ROM / cartridge banks. |
| `romsize` | in | Byte size of `emurom`. |

---

### `RasmFreeInfoStruct`

```c
void RasmFreeInfoStruct(struct s_rasm_info *debug);
```

Frees all heap memory owned by the `s_rasm_info` struct (error messages,
symbol names, internal arrays) and the struct itself. Must be called for every
non-NULL pointer returned through the `debug` output parameter of the
`RasmAssemble*` functions.

---

## `s_rasm_info` — Assembly result and debug info

Returned by all `RasmAssemble*` variants that accept a `debug` parameter.
Populated after assembly completes regardless of success or failure.

```c
struct s_rasm_info {
    struct s_debug_error *error;
    int nberror, maxerror, warnerr;
    struct s_debug_symbol *symbol;
    int nbsymbol, maxsymbol;
    int run, start;
    unsigned char *emuram;
    int lenram;
    unsigned char *emurom;
    int lenrom;
};
```

### Error fields

| Field | Type | Description |
|---|---|---|
| `error` | `s_debug_error *` | Array of `nberror` error entries. `NULL` if no errors. |
| `nberror` | `int` | Number of errors (and warnings when `erronwarn` is set) produced during assembly. Non-zero means assembly failed. |
| `maxerror` | `int` | Allocated capacity of the `error` array (internal, do not use). |
| `warnerr` | `int` | Total warning+error count as recorded by the assembler internally (`ae->nberr`). Includes warnings that did not abort assembly. |

### Symbol fields

| Field | Type | Description |
|---|---|---|
| `symbol` | `s_debug_symbol *` | Array of `nbsymbol` exported symbol entries. Populated when any symbol export flag is set in `s_parameter`. |
| `nbsymbol` | `int` | Number of symbols in the `symbol` array. |
| `maxsymbol` | `int` | Allocated capacity of the `symbol` array (internal, do not use). |

### Execution / address fields

| Field | Type | Description |
|---|---|---|
| `run` | `int` | Z80 program counter entry-point address extracted from the snapshot `PC` register. `-1` if no snapshot run address was defined by the source. |
| `start` | `int` | Lowest assembled address (first byte of the binary image in memory). |

### Emulator memory fields

Only populated when using `RasmAssembleInfoIntoRAM` / `RasmAssembleInfoIntoRAMROM`.

| Field | Type | Description |
|---|---|---|
| `emuram` | `unsigned char *` | Pointer to the caller's RAM buffer (same pointer passed in). The assembler writes assembled code into it and stores the pointer here for reference. |
| `lenram` | `int` | Size of `emuram` in bytes. |
| `emurom` | `unsigned char *` | Pointer to the caller's ROM/cartridge buffer (`RasmAssembleInfoIntoRAMROM` only). |
| `lenrom` | `int` | Size of `emurom` in bytes. |

---

## `s_debug_error` — Single assembly error entry

One entry in the `s_rasm_info.error[]` array.

```c
struct s_debug_error {
    char *filename;
    int line;
    char *msg;
    int lenmsg, lenfilename;
};
```

| Field | Type | Description |
|---|---|---|
| `filename` | `char *` | Source file in which the error occurred. May be `"<active-document>"` when assembling from an in-memory string with no path set. |
| `line` | `int` | 1-based line number of the error within `filename`. |
| `msg` | `char *` | Human-readable error message string. |
| `lenmsg` | `int` | Byte length of `msg` (cached, internal). |
| `lenfilename` | `int` | Byte length of `filename` (cached, internal). |

---

## `s_debug_symbol` — Single exported symbol entry

One entry in the `s_rasm_info.symbol[]` array.

```c
struct s_debug_symbol {
    char *name;
    int v;
};
```

| Field | Type | Description |
|---|---|---|
| `name` | `char *` | Symbol name as it appears in the source (case may be normalised unless `enforce_symbol_case` is set). |
| `v` | `int` | Resolved integer value of the symbol (address, constant, or EQU value). |

---

## `s_parameter` — Assembler configuration

This struct is passed to `RasmAssembleInfoParam()` to control assembler behaviour programmatically,
mirroring the command-line options accepted by the `rasm` binary.

All integer fields are boolean flags (`0` = off, `1` = on) unless stated otherwise.

---

## Input / Output filenames

| Field | CLI flag | Type | Description |
|---|---|---|---|
| `filename` | *(positional arg)* | `char *` | Path to the main source file to assemble. Mutually exclusive with `inline_asm`. |
| `outputfilename` | `-o <radix>` | `char *` | Common radix (prefix) used for all generated output files (binary, symbol, snapshot…). |
| `automatic_radix` | `-oa` | `int` | Derive the output radix automatically from the input filename instead of requiring an explicit `-o`. |
| `binary_name` | `-ob <filename>` | `char *` | Full output path for the raw binary file. Overrides the radix for binary output only. |
| `rom_name` | `-or <filename>` | `char *` | Full output path for the ROM (`.xpr`) file. |
| `cartridge_name` | `-oc <filename>` | `char *` | Full output path for the CPC cartridge (`.cpr`/`.xpr`) file. |
| `snapshot_name` | `-oi <filename>` | `char *` | Full output path for the Amstrad CPC snapshot (`.sna`) file. |
| `tape_name` | `-ot <filename>` | `char *` | Full output path for the tape image (`.cdt`/`.tzx`) file. |
| `symbol_name` | `-os <filename>` | `char *` | Full output path for the symbol / label export file. |
| `breakpoint_name` | `-ok <filename>` | `char *` | Full output path for the breakpoint export file. |
| `cprinfo_name` | `-ol <filename>` | `char *` | Full output path for the ROM label (CPR info) file. |
| `inline_asm` | `-inline 'text'` | `char *` | Assemble the given text string directly instead of reading from a file. Mutually exclusive with `filename`. |

---

## Include paths

| Field | CLI flag | Type | Description |
|---|---|---|---|
| `pathdef` | `-I<path>` | `char **` | Dynamic array of include search paths. Use multiple `-I` entries to add more. |
| `npath` | *(internal)* | `int` | Number of entries currently stored in `pathdef`. |
| `mpath` | *(internal)* | `int` | Allocated capacity of the `pathdef` array. |

---

## Pre-defined symbols

| Field | CLI flag | Type | Description |
|---|---|---|---|
| `symboldef` | `-Dvariable=value` | `char **` | Dynamic array of pre-defined symbols injected before assembly starts (e.g. `"MYFLAG=1"`). Symbols are upper-cased internally. |
| `nsymb` | *(internal)* | `int` | Number of entries in `symboldef`. |
| `msymb` | *(internal)* | `int` | Allocated capacity of the `symboldef` array. |

---

## Symbol / label export

| Field | CLI flag | Type | Description |
|---|---|---|---|
| `export_sym` | `-s` / `-sz` / `-sp` / `-sw` / `-sc` | `int` | Export symbols. Value selects the format: `0` = none, `1` = default (`%s #%X B%d`), `2` = Pasmo, `3` = Winape, `4` = custom (see `flexible_export`), `5` = ZX emulator convention. |
| `export_local` | `-sl` / `-sa` | `int` | Also include local (non-global) labels in the symbol export. |
| `export_var` | `-sv` / `-sa` | `int` | Also include variable symbols in the export. |
| `export_equ` | `-sq` / `-sa` | `int` | Also include `EQU` symbols in the export. |
| `export_multisym` | `-sm` | `int` | Split symbol export into multiple files, one per memory bank. |
| `flexible_export` | `-sc <format>` | `char *` | Custom format string used when `export_sym == 4`. |
| `export_sna` | *(snapshot directive)* | `int` | Embed symbols inside the snapshot file (SYMB chunk, compatible with ACE). |
| `export_snabrk` | `-sb` | `int` | Embed breakpoints inside the snapshot (BRKS/BRKC chunks). |
| `export_brk` | `-eb` | `int` | Export breakpoints to a standalone file (path given by `breakpoint_name`). |
| `export_tape` | *(tape directive)* | `int` | Output a tape image file. Driven by `TAPE` assembler directives; `tape_name` sets the filename. |
| `export_rasmSymbolFile` | `-rasm` | `int` | Export a RASM super-symbol file for use with ACE-DL. |
| `cprinfoexport` | `-er` | `int` | Export ROM labels to the CPR info file. |
| `enforce_symbol_case` | `-ec` | `int` | Preserve the original capitalisation of labels in the output instead of normalising to upper case. |
| `warn_unused` | `-wu` | `int` | Emit a warning for every symbol (alias, variable or label) that is defined but never referenced. |
| `labelfilename` | `-l <labelfile>` | `char **` | Array of symbol/label import files (Winape, Pasmo or RASM format) loaded before assembly. |

---

## Snapshot options

| Field | CLI flag | Type | Description |
|---|---|---|---|
| `v2` | `-v2` | `int` | Write snapshot version 2 instead of the default version 3. |
| `export_snabrk` | `-sb` | `int` | See symbol export section above. |

---

## Warning and error control

| Field | CLI flag | Type | Description |
|---|---|---|---|
| `nowarning` | `-w` | `int` | Suppress all warnings. |
| `nocrunchwarning` | `-wc` | `int` | Suppress the "slow crunch in progress" performance warning only. |
| `erronwarn` | `-twe` | `int` | Treat all warnings as hard errors, aborting assembly. |
| `maxerr` | `-me <value>` | `int` | Maximum number of errors before assembly is aborted. `0` means no limit. |
| `extended_error` | `-xr` | `int` | Display extended error context (surrounding source lines) alongside each error message. |

---

## Compatibility modes

| Field | CLI flag | Type | Description |
|---|---|---|---|
| `as80` | `-ass` / `-uz` | `int` | AS80 / UZ80 compatibility. `1` = AS80 behaviour, `2` = UZ80 behaviour. Affects expression parsing and label conventions. |
| `dams` | `-dams` | `int` | DAMS compatibility: labels may start with a dot (`.label`). |
| `pasmo` | `-pasmo` | `int` | PASMO compatibility: aligns expression evaluation and directive handling with PASMO assembler behaviour. |
| `rough` | `-m` (Maxam mode) | `float` | Rounding bias applied to floating-point expression results before truncation to integer. `0.0` = Maxam-style truncation (floor), `0.5` = round-to-nearest. Default `0.0`; set to `0.5` in CPR/cartridge mode. |
| `noampersand` | `-amper` / `--noampersand` | `int` | Use `&` as the prefix for hexadecimal values (Amstrad BASIC / Maxam style) instead of the default `#`. |
| `freequote` | `-fq` | `int` | Disable special-character processing inside quoted strings — treat all bytes literally. |
| `utf8enable` | `-utf8` | `int` | Translate French and Spanish keyboard characters inside quoted strings to their UTF-8 equivalents. |
| `module_separator` | `-msep <char>` | `char` | Character used as the separator between module name and symbol name (e.g. `_`). Default is `.` |
| `remu` | *(internal / snapshot)* | `int` | Set internally when a `REMU` / snapshot memory block directive is encountered. Forces memory remapping output. |
| `xpr` | `-xpr` | `int` | Force extended cartridge (`.xpr`) output format. |

---

## Macro behaviour

| Field | CLI flag | Type | Description |
|---|---|---|---|
| `macrovoid` | `-void` | `int` | Force macros to accept being called without parameters even when they declare parameters (void-safe mode). |
| `macro_multi_line` | `-mml` | `int` | Allow macro invocations to span multiple lines, with parameters continued onto subsequent lines. |

---

## Output / EDSK options

| Field | CLI flag | Type | Description |
|---|---|---|---|
| `edskoverwrite` | `-eo` | `int` | Overwrite existing files on a DSK/EDSK disk image if a file with the same name already exists. |
| `checkmode` | `-no` / `-depend=…` | `int` | Run assembly in check-only mode: parse and validate but do not write any output files. Set automatically when dependency export is requested. |
| `dependencies` | `-depend=make` / `-depend=list` | `int` | Dependency export mode. `0` = none, `E_DEPENDENCIES_MAKE` = single-line Makefile format, `E_DEPENDENCIES_LIST` = one dependency per line. Also sets `checkmode`. |

---

## Diagnostics and verbose output

| Field | CLI flag | Type | Description |
|---|---|---|---|
| `display_stats` | `-v` | `int` | Print assembler statistics (pass count, binary size, timings) after a successful assembly. |
| `verbose_assembling` | `-map` | `int` | Display detailed information during early assembling stages (bank map, segment layout). |
| `cprinfo` | `-cprquiet` (disables) | `int` | Print detailed ROM/cartridge bank information during assembly. Enabled by default in cartridge mode; `-cprquiet` sets it to `0`. |

---

## Notes

- All pointer fields (`char *`, `char **`) must be `NULL` when not used. The assembler checks for `NULL` before dereferencing.
- `npath` / `mpath` and `nsymb` / `msymb` are managed internally by `FieldArrayAddDynamicValueConcat`; initialise them to `0`.
- The `rough` field doubles as the Maxam compatibility switch: if `rough == 0.0` then `maxam = 1` (Maxam expression rules); if `rough != 0.0` then `maxam = 0`.
- `remu` is not set by the command-line parser — it is activated internally by `REMU`, `SNA`, and snapshot output directives. You can pre-set it to force remapping.
- `export_tape` is driven by `TAPE` source directives and the presence of `tape_name`; it does not have a standalone CLI flag.
