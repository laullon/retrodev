# RASM Assembler Directives Reference

Source: http://rasm.wikidot.com/english-index:home  
Sections covered: File output → Unclassifiable directives (as listed on the index page).  
Command-line options are **not** included here; see `syntax:command-line` on the wiki.

---

## Table of Contents

1. [File Output](#1-file-output)
2. [DSK Management](#2-dsk-management)
3. [HFE Management](#3-hfe-management)
4. [General Syntax](#4-general-syntax)
5. [Expressions](#5-expressions)
6. [Data and Structures](#6-data-and-structures)
7. [Labels](#7-labels)
8. [Label / Alias / Filename Generation](#8-label--alias--filename-generation)
9. [Debug Directives](#9-debug-directives)
10. [Conditional Directives](#10-conditional-directives)
11. [Loop Directives](#11-loop-directives)
12. [Instruction Repetition](#12-instruction-repetition)
13. [File Import](#13-file-import)
14. [Memory Management Directives](#14-memory-management-directives)
15. [Relocatable Code Section](#15-relocatable-code-section)
16. [Crunched Code Sections](#16-crunched-code-sections)
17. [Unclassifiable Directives](#17-unclassifiable-directives)
18. [Macros](#18-macros)

---

## 1. File Output

### SAVE

Export binary data to a file. Many variants supported.

```asm
SAVE 'file.bin',start,size                        ; raw binary
SAVE 'file.bin',start,size,AMSDOS                 ; with AMSDOS header
SAVE 'file.bin',start,size,HOBETA                 ; HOBETA header (ZX Spectrum)
SAVE 'file.bin',start,size,TAPE,'tape.cdt'        ; CDT/TZX tape format
SAVE 'file.bin',start,size,DSK,'floppy.dsk'       ; file inside EDSK image
```

If the DSK does not exist it is created in DATA format. If it exists it is modified
(use `-eo` command-line option to overwrite files that already exist on the disk).

**Convert a raw binary to an AMSDOS file:**

```asm
org #100
run #100
incbin 'binary_without_header.bin'
save 'binary_with_header',#100,$-#100,AMSDOS
```

**Cartridge creation from ROM files:**

```asm
buildcpr
bank 0 : incbin 'binary_bank0.bin'
bank 1 : incbin 'binary_bank1.bin'
; ... up to bank 31
```

---

## 2. DSK Management

Full sector-level floppy disk management. Some functions execute immediately;
others are deferred until after assembly.

### Immediate execution functions

```asm
EDSK   CREATE,'filename.dsk',DATA|VENDOR|UNFORMATED,nbtracks[,INTERLACED][,OVERWRITE]
EDSK READSECT,'filename.dsk','location',<exactsize>
EDSK  UPGRADE,'filename.dsk','outputfilename.dsk'
EDSK READFILE,'filename.dsk:side','filename',offset,size
EDSK DELFILE,'filename.dsk:side','filename'
```

### Deferred execution functions

```asm
EDSK WRITESECT,'filename.dsk',<start_addr>,<length>,'location'
EDSK    GAPFIX,'filename.dsk',TRACK|ALLTRACKS,<track>
EDSK       MAP,'filename.dsk'
EDSK      DROP,'filename.dsk','location'
EDSK       ADD,'filename.dsk','location',<size>,...
EDSK    RESIZE,'filename.dsk','location',<size>,...
EDSK     MERGE,'filename.dsk:side','filename.dsk:side','outputfilename.dsk'
```

### `filename.dsk:side`

Append `:A`, `:B`, `:0` or `:1` to select a side. Default is the first side.

### Location format

Defines track(s) and optional sector(s). Separator is a space; `-` defines ranges.

```
'5'              => track 5
'5:#C2'          => sector #C2 on track 5
'0-5'            => tracks 0 to 5
'0-5:$C2-0xC9'  => sectors #C2–#C9 on tracks 0–5
'0:#C1 0:#C3'   => sectors #C1 and #C3 on track 0
```

---

## 3. HFE Management

Create and write HFE floppy image files (no edit/read support).

```asm
HFE INIT,'filename.hfe'      ; create/open an HFE file
HFE CLOSE                    ; close the current HFE file
HFE SIDE,face                ; select current side (0 or 1)
HFE TRACK,track              ; select current track
HFE ADD_TRACK_HEADER         ; write track header to current track
HFE ADD_SECTOR,track,side,ID,SectorSize,offset   ; add a full sector
HFE ADD_GAP,gap_size[,value] ; add gap bytes (default value 0x4E)
HFE COMPLETE_TRACK           ; pad current track to 6250 bytes
```

### Byte Mode

```asm
HFE START_CRC                ; begin CRC calculation (sector header or data)
HFE ADD_BYTE,byte,...        ; add raw byte(s); use SYNCHRO OR for sync bytes
HFE OUTPUT_CRC               ; write 2 CRC bytes to current track
```

---

## 4. General Syntax

- Not case-sensitive (all input converted to uppercase internally).
- Labels do not require `:` suffix (optional).
- Indentation is optional.
- Windows and Linux line endings both accepted.
- Comments: `;`, `//`, `/* ... */`.

### Z80 mnemonics

All documented and undocumented Z80 instructions are supported, including:

- 8-bit IX/IY sub-register access: `IXL`/`IXH`/`XL`/`XH` etc.
- Complex bit/rotate with index register: `RES 0,(IX+d),r`
- Undocumented: `OUT (<byte>),A`, `IN A,(<byte>)`, `IN 0,(C)`, `IN F,(C)`, `SLL r`, `SL1 r`

### Shorthand instructions

```asm
PUSH BC,DE,HL   => PUSH BC : PUSH DE : PUSH HL
POP  BC,DE,HL   => POP  BC : POP  DE : POP  HL
NOP 5           => five NOP instructions
LD BC,DE        => LD B,D : LD C,E
LD HL,(IX+n)    => LD H,(IX+n+1) : LD L,(IX+n)
LD HL,SP        => LD HL,0 : ADD HL,SP   ; flags modified!
EXA             => EX AF,AF'
SRL BC          => SRL B : RR C
SRL HL          => SRL H : RR L
; and many more 16-bit pseudo-ops
```

---

## 5. Expressions

### Aliases (EQU)

```asm
my_alias EQU #4000
```

Cannot be redefined after declaration.

### Dynamic variables

```asm
ang=0
repeat 256
  defb 127*sin(ang)
  ang=ang+360/256
rend
```

### Variable names with indexes

```asm
repeat 256,x
  myvariable{x}=x*5
rend
```

### Literal value formats

| Prefix/suffix | Base        |
|---------------|-------------|
| plain digits  | decimal     |
| `%`, `0b`, `b` suffix | binary |
| `@` prefix    | octal       |
| `#`, `$`, `0x`, `h` suffix | hexadecimal |
| `'c'` (single char) | ASCII value |
| `$` alone     | current instruction address |

> **Note:** `-amper` command-line flag enables `&` as hex prefix (e.g. `&1A2B`).

### Operators

| Operator | Meaning |
|----------|---------|
| `^` / `XOR` | exclusive OR |
| `%` / `MOD` | modulo |
| `&` / `AND` | bitwise AND |
| `&&` | boolean AND |
| `\|` / `OR` | bitwise OR |
| `\|\|` | boolean OR |
| `<<` | shift left |
| `>>` | shift right |
| `hi(n)` | high byte of 16-bit value |
| `lo(n)` | low byte of 16-bit value |
| `getnop(instr)` | number of NOP cycles for an instruction |

### Mathematical functions

`abs`, `sin`, `cos`, `asin`, `acos`, `int`, `ceil`, `floor`, `round`,
`exp`, `log`, `log2`, `log10`, `sqrt`, `min`, `max`.

---

## 6. Data and Structures

### ENUM

Declare a sequence of aliases with automatic increment.

```asm
ENUM [prefix[,start_value[,increment]]]
  name1
  name2
  name3
MEND
```

- Default: no prefix, start = 0, increment = 1.
- Force a counter value mid-enum: `name=value`.

### CHARSET

Remap ASCII character codes for string literals.

```asm
CHARSET                       ; reset all mappings to defaults
CHARSET 'string',value        ; map each char of string starting at value
CHARSET start,value           ; map from ASCII code start
CHARSET start,end,value       ; map range start..end
CHARSET 'string','string'     ; char-to-char mapping
```

### UTF8REMAP

```asm
UTF8REMAP 'é',50             ; map UTF-8 character to 8-bit code
```

Requires `-fq` command-line option.

### DB / DEFB / DM / DEFM

```asm
DEFB 'r'-'a'+'A','oudoud',#6F,hi(label)
DEFB 'string\xFF'             ; \xFF inserts hex byte
```

### STR

Like `DEFB` but ORs `#80` onto the last byte of each string (common CPC idiom).

```asm
STR 'hello'                   ; last byte = 'o' | #80
STR 'one','two'               ; two strings, each terminated with high-bit set
```

### DEFW / DW

Emit 16-bit words (little-endian).

### DEFI / DI

Emit 32-bit double words.

### DEFR / DR

Emit a floating-point value.

### DEFS / DS

```asm
DEFS count[,fill_value]       ; reserve bytes, filled with 0 by default
```

### STRUCT / ENDSTRUCT

```asm
STRUCT mystruct
  field1 DEFS 10
  field2 DEFW 0
ENDSTRUCT

STRUCT mystruct my_instance,1   ; declare 1 instance, optionally initialise
; access:  my_instance5  => address of 6th element
```

- `{sizeof}mystruct` gives the byte size of the structure.

---

## 7. Labels

### Global labels

Start with a letter or `_`. Accessible from anywhere.

```asm
monlabel1 jp monlabel2
```

### Proximity labels

Start with `.` — internally concatenated to the last global label.

```asm
label_global ld b,5
.loop        djnz .loop
```

Full reference from another scope: `label_global.loop`

### Local labels

Start with `@` — visible only within the enclosing loop or macro.

```asm
repeat 10
  @loop
  repeat 10
    djnz @loop       ; inner @loop
  rend
  djnz @loop         ; outer @loop
rend
```

### Modules

```asm
MODULE soft1
label_global          ; becomes soft1_label_global
MODULE soft2
label_global          ; becomes soft2_label_global
jp soft1_label_global
```

Module separator character defaults to `_`; change with `-msep` command-line option.

Module names can contain variable-generated strings: `truc{x}`.

### Export labels (ACE-DL)

```asm
LABEL LOCAL           ; following labels compiled at their compile-time address
LABEL GLOBAL          ; following labels in full 64K addressable space
LOCALISATION RAM,4    ; assign space labels to RAM page 4
LOCALISATION ROM,LOWER
LOCALISATION ROM,14
```

---

## 8. Label / Alias / Filename Generation

Generate numbered sequences inside loops.

### Label series

```asm
repeat 16,x
  inside{x} ldi
rend
```

### Alias series

```asm
repeat 4,x
  table{x} equ (x-1)*#4000
rend
```

### Filename series

```asm
repeat 10,x
  filename{x} incbin 'myfile{x}.bin'
rend
```

---

## 9. Debug Directives

### PRINT

```asm
PRINT 'string',variable,...
```

Display text and values at assembly time. Format prefixes:

| Prefix | Effect |
|--------|--------|
| `{hex}` | hexadecimal (auto 2/4 digits) |
| `{hex2}`, `{hex4}`, `{hex8}` | forced hex width |
| `{bin}` | binary (auto 8/16 bits) |
| `{bin8}`, `{bin16}`, `{bin32}` | forced binary width |
| `{int}` | rounded integer |

### DELAYED_PRINT

```asm
DELAYED_PRINT 'string',variable,...
```

Same as `PRINT` but output is deferred until the end of assembly, so variables not yet declared at call-time can be used. Avoid inside loops (all instances see the final variable value).

### STOP

```asm
STOP
```

Halt assembly immediately; no output file is produced.

### FAIL

```asm
FAIL 'string',variable,...
```

Print a message then stop assembly (`PRINT` + `STOP` combined).

### ASSERT

```asm
ASSERT condition
ASSERT condition,'message',...
```

Halt assembly if condition is false.

### NOEXPORT / ENOEXPORT

```asm
NOEXPORT                      ; suppress export of all following symbols
NOEXPORT label1,label2        ; suppress export of specific symbols only
ENOEXPORT                     ; re-enable symbol export
ENOEXPORT label1,label2       ; re-enable specific symbols
```

### BRK

```asm
BRK                           ; emit #ED,#FF (hard breakpoint)
```

Not handled by all emulators.

### REDEFINE_BRK

```asm
REDEFINE_BRK #CF,#CF,#CF      ; redefine the bytes emitted by BRK
```

### BREAKPOINT

```asm
BREAKPOINT                    ; soft exportable breakpoint at current address
BREAKPOINT label              ; soft breakpoint at label
```

Binary is not modified; breakpoints must be exported via command-line options
(`-ok`, `-eb`, `-sb`).

**Extended ACE-DL breakpoint syntax:**

```asm
BREAKPOINT [opt|opt|...]
```

Options (all optional, order-independent):

| Option | Meaning |
|--------|---------|
| `EXEC`, `MEM`, `IO` | breakpoint type |
| `READ`, `WRITE`, `RW`, `READWRITE` | access mode |
| `STOP`, `STOPPER`, `WATCH`, `WATCHER` | execution mode |
| `ADDR=address` | trigger address |
| `MASK=mask` | address mask (default `0xFFFF`) |
| `SIZE=size` | size for memory breakpoints |
| `VALUE=value` | trigger byte value |
| `VALMASK=mask` | value mask |
| `CONDITION='expr'` | ACE-DL condition string |
| `NAME='label'` | display name in emulator |

### NAMEBANK

```asm
NAMEBANK 0,'ROM Init'
NAMEBANK 1,'Serval screen'
```

Assign a display name to a non-temporary memory bank for cartridge/snapshot labelling.

---

## 10. Conditional Directives

### IF / IFNOT / ELSE / ELSEIF / ELSEIFNOT / ENDIF

```asm
IF condition
  ...
ELSEIF condition
  ...
ELSE
  ...
ENDIF
```

### IFDEF / IFNDEF

Conditional assembly based on existence of a variable, label, alias or macro.

```asm
IFDEF production
  or #80
ENDIF
```

### UNDEF

```asm
UNDEF variable                ; remove a variable (no effect if absent)
```

### IFUSED / IFNUSED

Conditional blocks based on whether a variable/label/alias has been referenced
at the time of the directive.

### SWITCH / CASE / DEFAULT / BREAK / ENDSWITCH

C-style switch. Multiple `CASE` blocks with the same value are allowed (partial
code sharing between cases). No implicit fall-through break.

```asm
SWITCH grouik
  CASE 3
    defb 'A'
  CASE 5
    defb 'B'
  CASE 7
    defb 'C'
    BREAK
  DEFAULT
    defb 'F'
ENDSWITCH
```

---

## 11. Loop Directives

### REPEAT / REND (and ENDREP / ENDREPEAT)

```asm
REPEAT nb[,counter_var[,start[,step]]]
  ...
REND
```

All parameters except `nb` are optional.

```asm
repeat 10           : ldi :          rend   ; 10× LDI
repeat 10,x         : ld a,x :       rend   ; LD A,1 … LD A,10
repeat 5,x,0        : print x :      rend   ; 0 1 2 3 4
repeat 5,x,0,2      : print x :      rend   ; 0 2 4 6 8
```

### UNTIL (conditional loop end)

```asm
cpt=90
repeat
  defb cpt
  cpt=cpt-2
until cpt>0
```

---

## 12. Instruction Repetition

Shorthand for repeating specific opcodes without a full `REPEAT` block.

```asm
ldi 16          ; equivalent to: repeat 16 : ldi : rend
nop 5           ; five NOPs
```

**Supported opcodes:**  
`NOP`, `LDI`, `LDD`, `RLCA`, `RRCA`, `INI`, `IND`, `OUTI`, `OUTD`, `HALT`

---

## 13. File Import

### INCLUDE

```asm
INCLUDE 'myfile.asm'
```

Inserts the file at the current source location. Path is relative to the including file.
Use absolute paths to anchor to source root. No recursion limit (guard with `IFDEF`).

Include guard pattern:

```asm
ifndef __SYMBOL__
__SYMBOL__ = 1
  ;; ... code ...
endif
```

### INCBIN

Import binary data into the assembled output.

```asm
INCBIN 'file.bin'
INCBIN 'file.bin',offset
INCBIN 'file.bin',offset,length
```

**Flags (can be combined, order-independent):**

| Flag | Effect |
|------|--------|
| `EXISTS` | only load if file exists |
| `OFF` | read into memory without overwrite check |
| `SKIPHEADER` | skip first 128 bytes if AMSDOS header found |
| `REVERT` | read data in reverse order |
| `REMAP,numColumn` | column remapping |
| `VTILES,numLines` | vertical tile re-ordering |
| `ITILES,width` | interleaved tile layout |
| `GTILES,width` | grouped tile layout |

**WAV import:**

```asm
INCBIN 'sound.wav',SMP        ; raw sample
INCBIN 'sound.wav',SM2        ; 2-bit sample
INCBIN 'sound.wav',SM4        ; 4-bit sample
INCBIN 'sound.wav',DMA,preamp,options,...
```

### INCL48 / INCLZX0 / INCLZSA1 / INCLZSA2 / INCLAPULTRA / ...

Include and decompress on-the-fly crunched binary files (see also Crunched Code Sections).

---

## 14. Memory Management Directives

### Build mode selection

| Directive | Effect |
|-----------|--------|
| `BUILDCPR [EXTENDED] [SYMBOLS] ["filename"]` | Cartridge mode (BANK 0–31, or more with `EXTENDED`) |
| `BUILDTAPE ["filename"]` | Tape CDT output mode |
| `BUILDSNA [V2] ["filename"]` | Snapshot mode (16K banks 0–259, or BANKSET 64K pages) |
| `BUILDZX` | ZX Spectrum snapshot mode (banks 0–7) |
| `BUILDROM [concat]` | ROM creation mode |
| `BUILDOBJ` | Binary object (link-editor prototype) |

### BANK

```asm
BANK n                        ; switch to memory bank n
```

Switches RASM to cartridge mode when used with a specific bank number.

### SNAPINIT / CPRINIT

```asm
SNAPINIT 'snapshot_file'      ; pre-load memory from a snapshot
CPRINIT  'cartridge_file'     ; pre-load memory from a cartridge
```

### NOCODE / CODE

```asm
NOCODE                        ; stop writing bytes; still advance address counter
CODE                          ; resume normal write mode
CODE SKIP                     ; resume at address reached during NOCODE
```

### RUN

```asm
RUN address[,ga_config]
```

Sets PC (entry point) in snapshot/AMSDOS exports and the start address in DSK headers.

### ORG

```asm
ORG logical_address[,physical_address]
```

Set the assembly address. Logical address controls label values; physical address
controls where bytes are actually written. Return to physical address with `ORG $`.

```asm
org #8000,$
ld hl,$         ; LD HL,#8000 (logical), but written at physical #8000
org $           ; re-sync logical = physical
```

### ALIGN

```asm
ALIGN limit[,fill_value]      ; advance address to next multiple of limit
```

No bytes produced by default; specify `fill_value` to fill the gap.

### CONFINE

```asm
CONFINE value                 ; ensure following data fits in one 256-byte page
```

Emits a warning (with byte count) if alignment padding is needed.

### LIMIT

```asm
LIMIT limit                   ; set assembly upper address limit
```

### PROTECT

```asm
PROTECT start_address,end_address   ; mark region as write-protected
```

Generates an error if any data is written into the region.

### SUMMEM

```asm
SUMMEM start_address,end_address    ; emit 1-byte checksum of range
```

### XORMEM

```asm
XORMEM start_address,end_address    ; emit 1-byte XOR checksum of range
```

Example usage:

```asm
xormem #0000,#1000            ; XOR all bytes in ROM and write result here
```

### SUM16

```asm
SUM16 start_address,end_address     ; emit 2-byte (word) checksum of range
```

---

## 15. Relocatable Code Section

> **Status:** Still under development; subject to change.

```asm
RELOCATION
  ; ... relocatable code ...
ENDRELOCATE
```

Constraints:
- No `MODULE` changes inside the section.
- No `ORG` changes inside the section.
- Variables modified inside must be initialised inside.
- Cannot be used inside crunched segments.

RASM assembles the block twice (offset by `#102`) to detect byte-weight changes
and produces a `.rel` file containing relocation tables:

```asm
relocation0:
  .reloc16 defw #0101,#0115   ; 16-bit offsets to patch
  .reloc8h defw #0104,#010E   ; high-byte offsets to patch
  .reloc8l defw #0109         ; low-byte offsets to patch
```

---

## 16. Crunched Code Sections

Assemble and compress code on-the-fly. Labels before the section are relocated
relative to the compressed result. Sections cannot be nested.

### Open a crunched section

| Directive | Compressor |
|-----------|-----------|
| `LZEXO` | Exomizer 2.0 |
| `LZX7` | ZX7 |
| `LZX0` | ZX0 (forward) |
| `LZX0B` | ZX0 (backward) |
| `LZAPU` | AP-Ultra |
| `LZSA1` | LZSA1 |
| `LZSA2` | LZSA2 |
| `LZ4` | LZ4 |
| `LZ48` | LZ48 |
| `LZ49` | LZ49 |

### LZCLOSE

```asm
LZCLOSE                       ; close the current crunched section
```

### Examples

```asm
jr later
lzx0
  defs 256,0
lzclose
later ret

; concatenate and crunch multiple binaries
bank
lzx0
  incbin 'first.bin'
  incbin 'second.bin'
  incbin 'third.bin'
lzclose
save 'combined.bin.lz',...
```

### Delayed crunched scripts (DELAYED_LZEXO etc.)

```asm
; DELAYED_ prefix defers compression until after full assembly
DELAYED_LZEXO script01
  ...
LZCLOSE
delayed_print '$=',$ ,'script01=',script01 ; after
```

Deployment example:

```asm
ld hl,script_list
ld b,0
add hl,bc : add hl,bc
ld a,(hl) : inc hl : ld h,(hl) : ld l,a  ; get crunched script address
ld de,script_addr
jp unzx0                                   ; decompress to DE
```

---

## 17. Unclassifiable Directives

### TIMESTAMP

```asm
TIMESTAMP "format_string"
```

Generates a string at assembly time using date/time codes:

| Code | Meaning |
|------|---------|
| `Y` or `YYYY` | year (4 digits) |
| `YY` | year (2 digits) |
| `M` | month (2 digits) |
| `D` | day (2 digits) |
| `h` | hour (2 digits) |
| `m` | minutes (2 digits) |
| `s` | seconds (2 digits) |

```asm
TIMESTAMP "[Y-M-D h:m]"       ; e.g. => [2024-06-15 14:30]
```

### TICKER

```asm
TICKER START,variable         ; begin T-state / NOP measurement
TICKER STOP,variable          ; end measurement; result stored in variable
```

Result is in NOPs by default; in T-states if Spectrum mode is active.
Typical use: synchronise code to a raster line.

```asm
TICKER START,measure
  repeat 8
    inc b : outi
  rend
TICKER STOP,measure
defs 64-getnop(dec a : jr nz)-measure
```

### PROCEDURE *(WIP)*

```asm
PROCEDURE label               ; declare procedure address for object export
```

### EXTERNAL *(WIP)*

```asm
EXTERNAL symbol1,symbol2,...  ; declare external symbols for import
```

### SUMMEM (unclassifiable variant)

```asm
SUMMEM start,end              ; emit 1-byte checksum of byte range
```

> Note: does not work inside crunched segments.

### XORMEM (unclassifiable variant)

```asm
XORMEM start,end              ; emit 1-byte XOR checksum of byte range
```

> Note: does not work inside crunched segments.

### SUM16 (unclassifiable variant)

```asm
SUM16 start,end               ; emit 2-byte unsigned checksum of byte range
```

> Note: does not work inside crunched segments.

---

## 18. Macros

### Declaration

```asm
MACRO name[,param1,param2,...]
  ; body — parameters referenced as {param}
MEND   ; or ENDM
```

Parameters use curly-brace substitution. Example:

```asm
macro LDIXREG register,dep
  if {dep}<-128 || {dep}>127
    push bc,ix
    ld bc,{dep}
    add ix,bc
    ld (ix+0),{register}
    pop ix,bc
  else
    ld (ix+{dep}),{register}
  endif
mend
```

### Macro call

```asm
mymacro param1,param2         ; normal call
mymacro (void)                ; call with no parameters — triggers error if name unknown
```

Using `(void)` is the recommended safe style. Enforced globally with `-void` option.

### Dynamic parameter evaluation

```asm
macro test myarg
  defb {myarg}
mend

repeat 2
  test {eval}repeat_counter   ; evaluate counter before passing to macro
rend
```

Without `{eval}`, the variable name is passed as a literal and evaluated lazily
inside the macro body.

### 16-bit register decomposition

Inside macros, `.low` and `.high` access the low/high byte of a 16-bit register:

```asm
macro add16,R1,R2
  ld a,{R1}.low
  add {R2}.low
  ld {R1}.low,a
  ld a,{R1}.high
  adc {R2}.high
  ld {R1}.high,a
mend
```
