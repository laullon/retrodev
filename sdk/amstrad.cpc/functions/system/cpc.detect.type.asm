;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC machine type detection routines.
;
; Purpose: identify target machine/model characteristics for runtime setup.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

IFNDEF __CPC_DETECT_TYPE_ASM__
__CPC_DETECT_TYPE_ASM__ equ 1

; Require macro files to be included before this file.
IFNDEF __MACROS_CRTC_ASM__
FAIL 'cpc.detect.type.asm requires macros.crtc.asm to be included first'
ENDIF
IFNDEF __MACROS_GA_ASM__
FAIL 'cpc.detect.type.asm requires macros.ga.asm to be included first'
ENDIF
IFNDEF __MACROS_PSG_ASM__
FAIL 'cpc.detect.type.asm requires macros.psg.asm to be included first'
ENDIF

; CPC_DetectType
;
; Discriminates between the six main machine variants by probing hardware
; differences in a fixed order: Plus/ASIC presence first, then GX4000 keyboard
; absence, then ROM version for classic models, then RAM for Plus sub-types.
;
; Returned model constants (A on exit):
;   CPC_MODEL_464    = 0   CPC 464  -- 64K RAM,  CRTC type 0/1/2, ROM version 0
;   CPC_MODEL_664    = 1   CPC 664  -- 64K RAM,  CRTC type 0/1/2, ROM version 1, disc
;   CPC_MODEL_6128   = 2   CPC 6128 -- 128K RAM, CRTC type 0/1/2, ROM version 3
;   CPC_MODEL_464P   = 3   CPC 464+ -- 128K RAM, ASIC (type 3/4), no 6128 extra page
;   CPC_MODEL_6128P  = 4   CPC 6128+-- 192K RAM, ASIC (type 3/4), 6128 extra page present
;   CPC_MODEL_GX4000 = 5   GX4000   -- 64K RAM,  ASIC (type 3/4), no keyboard matrix
;
; Detection strategy (executed in this order):
;
;   Step 1 -- Plus / ASIC detection.
;     The ASIC chip (AMS40489 / AMS40226) present in CPC+ and GX4000 uses
;     CRTC type 3 or 4 (see cpc.detect.crtc.asm). Classic CPCs always have
;     CRTC type 0, 1, or 2. Testing the CRTC type is non-destructive and
;     requires no unlock sequence.
;     CRTC type 3 or 4 -> go to Step 2 (Plus/GX4000 branch).
;     CRTC type 0, 1, or 2 -> go to Step 3 (classic branch).
;
;   Step 2 -- GX4000 vs CPC 464+ / 6128+.
;     The GX4000 has no keyboard connector. All 10 keyboard matrix rows always
;     read as #FF (no keys wired, pull-up on the column lines). On a 464+ or
;     6128+ at least one row will differ from #FF even with no keys held (the
;     matrix has real keys whose contacts pull lines low on specific hardware
;     revisions), but more critically the keyboard scan rows can be distinguished
;     because the GX4000 has no physical matrix -- the PPI row-select lines are
;     not connected to anything.
;
;     Method: assert PPI row 0 and read the column byte. Then assert row 6 and
;     read again. On a real CPC keyboard both reads can differ (different rows
;     of keys). On the GX4000 both reads return #FF. A second confirmation is
;     performed by OR-ing all 10 rows: if every row returns #FF the machine is
;     the GX4000.
;
;     All rows #FF -> CPC_MODEL_GX4000.
;     At least one row non-#FF -> go to Step 2b.
;
;   Step 2b -- CPC 464+ vs CPC 6128+.
;     Both have the ASIC. The 6128+ contains the standard 6128 extra 64K RAM
;     page on top of the ASIC's own extra page. Testing whether GA_RAM_PAGE_1
;     holds distinct RAM (the same write/read-back test used in cpc.detect.ram)
;     distinguishes them: page 1 present -> 6128+; absent -> 464+.
;
;   Step 3 -- Classic CPC: 464 vs 664 vs 6128.
;     The Locomotive BASIC / firmware ROM stores a version byte at address
;     #B9C5 inside the lower ROM. This byte is:
;       #00 -> CPC 464
;       #01 -> CPC 664
;       #03 -> CPC 6128
;     The lower ROM must be visible during the read (it is, unless explicitly
;     disabled with GA_LROM_DISABLE -- the firmware itself runs from it).
;     Any unknown value is treated as CPC_MODEL_464 (safe fallback).
;
; Register use note: this routine calls the logic from CPC_DetectCRTC inline
; (not via CALL) to avoid stack and register conflicts, and performs its own
; keyboard scan using direct PPI access identical to cpc.keyboard.asm.
;
; Entry:
;   none
; Exit:
;   A = model constant (CPC_MODEL_464 .. CPC_MODEL_GX4000)
; Destroys: AF, BC, D, E, IXL
; Preserves: HL, IXH, IY
; Requirements: interrupts must be disabled (DI) before calling.
;               The lower ROM must be enabled (GA Mode/ROM byte bit 2 = 0).

CPC_MODEL_464		equ 0		; CPC 464
CPC_MODEL_664		equ 1		; CPC 664
CPC_MODEL_6128		equ 2		; CPC 6128
CPC_MODEL_464P		equ 3		; CPC 464+
CPC_MODEL_6128P		equ 4		; CPC 6128+
CPC_MODEL_GX4000	equ 5		; GX4000

; CPC_DetectType -- identify the specific CPC model (464, 664, 6128, 464+, 6128+, GX4000).
; Entry: none  (interrupts must be disabled; lower ROM must be enabled)
; Exit:  A = model constant: CPC_MODEL_464..CPC_MODEL_GX4000
; Destroys: AF, BC, DE, IXL  Preserves: HL, IXH, IY
CPC_DetectType:
    ; -------------------------------------------------------
    ; Step 1: probe CRTC type to detect Plus/ASIC hardware.
    ; -------------------------------------------------------
    ; Select CRTC R12 via the address port (#BC).
    ld bc,CRTC_PORT_ADDR+CRTC_R12_ADDR_H
    out (c),c
    ; Read the current R12 value via the data port (#BD) and stash in IXL.
    ld b,#bd
    in a,(c)
    ld ixl,a
    ; Write #FF to R12 -- the value that discriminates between CRTC types.
    ld a,#ff
    out (c),a
    ; Read R12 back.
    in a,(c)
    ; Restore R12 immediately (R12 still selected, B = #BD).
    ld c,ixl
    out (c),c
    ; A = #00 -> CRTC type 3 or 4 (AMS ASIC) -> Plus / GX4000.
    ; A = #FF -> CRTC type 2 (MC6845) -> classic.
    ; A = other -> CRTC type 0 or 1 -> classic.
    or a
    jr z,.plus_branch
    ; -------------------------------------------------------
    ; Step 3: classic CPC -- read the ROM version byte at #B9C5.
    ; -------------------------------------------------------
    ; The lower ROM must be visible (firmware runs from it, so it is).
    ; #B9C5 holds: #00 = 464, #01 = 664, #03 = 6128.
    ld a,(#b9c5)
    cp #01
    jr z,.is_664
    cp #03
    jr z,.is_6128
    ; Unknown or #00 -- treat as 464.
    ld a,CPC_MODEL_464
    ret
.is_664:
    ld a,CPC_MODEL_664
    ret
.is_6128:
    ld a,CPC_MODEL_6128
    ret
    ; -------------------------------------------------------
    ; Step 2: Plus / GX4000 branch.
    ; -------------------------------------------------------
.plus_branch:
    ; Scan all 10 keyboard matrix rows using direct PPI access.
    ; Setup: configure PPI for keyboard scan (mirrors cpc.keyboard.asm setup).
    ;   PPI control word PSG_PPI_PORTA_OUT: Port A = output, Port C = output.
    ld bc,PSG_PPI_CTRL_B*256+PSG_PPI_PORTA_OUT
    out (c),c
    ; Place AY register index PSG_R14_PORT_A (R14 = keyboard row-select) on Port A.
    ld bc,PSG_PPI_DATA_B*256+PSG_R14_PORT_A
    out (c),c
    ; Latch R14 as the active AY register (BDIR=1, BC2=1 = PSG_BUS_ADDR on Port C).
    ld bc,PSG_PPI_PORTC_B*256+PSG_BUS_ADDR
    out (c),c
    ; Return AY to inactive (PSG_BUS_INACTIVE).
    ld bc,PSG_PPI_PORTC_B*256+PSG_BUS_INACTIVE
    out (c),c
    ; PPI control word PSG_PPI_PORTA_IN: Port A = input, Port C = output.
    ld bc,PSG_PPI_CTRL_B*256+PSG_PPI_PORTA_IN
    out (c),c
    ; Scan all 10 rows, accumulating the AND of all column bytes into D.
    ; All rows #FF -> no keyboard matrix wired -> GX4000.
    ; Any row with a 0 bit -> key contacts wired -> CPC 464+ / 6128+.
    ld d,#ff
    ; E = current row selector: PSG_BUS_READ | row (BDIR=0, BC2=1 = AY read, bits 3-0 = row).
    ; D holds the AND accumulator. B is set to the required port high byte each step.
    ld e,PSG_BUS_READ
.kb_scan:
    ; Drive PPI Port C with the row selector (B = PSG_PPI_PORTC_B).
    ld b,PSG_PPI_PORTC_B
    out (c),e
    ; Read column bits from PPI Port A (B = PSG_PPI_DATA_B).
    ld b,PSG_PPI_DATA_B
    in a,(c)
    ; AND into accumulator -- any 0 bit means a key is physically wired.
    and d
    ld d,a
    ; Advance to the next row and loop until all 10 rows are done.
    inc e
    ld a,e
    cp PSG_BUS_READ+10
    jr nz,.kb_scan
    ; Restore PPI Port C to inactive state (no row selected, AY inactive).
    ld bc,PSG_PPI_PORTC_B*256+PSG_BUS_INACTIVE
    out (c),c
    ; If all rows returned #FF, no keyboard matrix present -> GX4000.
    ld a,d
    cp #ff
    jr z,.is_gx4000
    ; -------------------------------------------------------
    ; Step 2b: keyboard present -- 464+ vs 6128+.
    ; Test whether GA extra RAM page 1 exists (6128+ has it; 464+ does not).
    ; -------------------------------------------------------
    ; Map extra page 1 (GA_RAM_PAGE_1), config 1 (slot 3 -> extra bank 7)
    ; into slot 3 at #C000.
    ld bc,GA_PORT_CMD+GA_CMD_RAM_BANK+GA_RAM_PAGE_1+GA_RAM_CFG_1
    out (c),c
    ; Save the byte at #C000 in IXL.
    ld a,(#c000)
    ld ixl,a
    ; Write test byte and read back.
    ld (#c000),#aa
    ld a,(#c000)
    ; Restore original byte.
    ld (#c000),ixl
    ; Restore normal RAM map.
    ld bc,GA_PORT_CMD+GA_CMD_RAM_BANK+GA_RAM_CFG_0
    out (c),c
    ; If read-back matched #AA, extra page 1 is present -> 6128+.
    cp #aa
    jr z,.is_6128p
    ld a,CPC_MODEL_464P
    ret
.is_6128p:
    ld a,CPC_MODEL_6128P
    ret
.is_gx4000:
    ld a,CPC_MODEL_GX4000
    ret


ENDIF
