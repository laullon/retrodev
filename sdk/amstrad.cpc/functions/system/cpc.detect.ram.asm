;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC RAM detection routines.
;
; Purpose: detect memory expansion and report available RAM configuration.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

IFNDEF __CPC_DETECT_RAM_ASM__
__CPC_DETECT_RAM_ASM__ equ 1

; Require macros.ga.asm to be included before this file.
IFNDEF __MACROS_GA_ASM__
FAIL 'cpc.detect.ram.asm requires macros.ga.asm to be included first'
ENDIF

; CPC_DetectRAM
;
; Tests for the presence of the 64K extra RAM bank by writing a test value to
; the first byte of extra bank 7 (mapped into slot 3 at #C000 via GA config 1),
; then reading it back. If the written value is returned, extra RAM is fitted and
; the machine has 128K or more. If the read-back does not match, only 64K is present.
;
; Machines with more than 128K (DK'Tronics 256K, CPC+ ASIC with 128K) can be
; distinguished by iterating through GA_RAM_PAGE_1..7 in the same way, testing
; each extra 64K block in turn and counting the confirmed pages.
;
; The total RAM size returned is:
;   64   -- 64K base RAM only (CPC 464, 664, or 6128 with no extra RAM detected)
;   128  -- 128K: one extra 64K page confirmed (standard 6128 / CPC+)
;   192  -- 192K: two extra 64K pages confirmed
;   256  -- 256K: three extra 64K pages confirmed
;   320  -- 320K: four extra 64K pages confirmed
;   384  -- 384K: five extra 64K pages confirmed
;   448  -- 448K: six extra 64K pages confirmed
;   512  -- 512K: seven extra 64K pages confirmed
;
; Test byte selection: the value #AA (#10101010) is used because it is a
; checkerboard pattern unlikely to be a false positive left by other code,
; and it catches wiring faults that affect only half the data lines.
;
; Important: this routine briefly maps extra RAM into slot 3 (#C000-#FFFF)
; displacing the upper ROM / normal bank 3 for a few instructions. Interrupts
; MUST be disabled before calling and re-enabled after if required.
; The CRTC display start address should be pointing at #4000-#7FFF (slot 1)
; or #8000-#BFFF (slot 2) during the test -- not #C000 -- to avoid visual glitches.
;
; Entry:
;   none
; Exit:
;   HL = total RAM size in kilobytes (64, 128, 192, 256, 320, 384, 448, or 512)
; Destroys: AF, BC, D, E, HL, IXL
; Preserves: IXH, IY
; Requirements: interrupts must be disabled (DI) before calling.

; CPC_DetectRAM -- detect total RAM fitted (64K..512K) by probing extra GA RAM pages.
; Entry: none  (interrupts must be disabled)
; Exit:  HL = total RAM in kilobytes (64, 128, 192, 256, 320, 384, 448, or 512)
; Destroys: AF, BC, DE, HL, IXL  Preserves: IXH, IY
CPC_DetectRAM:
    ; Start with 64K base -- we will add 64 for each confirmed extra page.
    ; D = current page selector accumulator (bits 5-3, incremented by GA_RAM_PAGE_1=8 each step).
    ; E = base GA RAM bank command byte (GA_CMD_RAM_BANK | GA_RAM_CFG_1).
    ; HL = running total in kilobytes.
    ld hl,64
    ld d,0
    ld e,GA_CMD_RAM_BANK+GA_RAM_CFG_1
.page_loop:
    ; Build the GA RAM bank command: base | current page offset.
    ld a,e
    or d
    ; Switch extra RAM page into slot 3 (#C000-#FFFF) via B=#7F, C=command.
    ld c,a
    ld b,#7f
    out (c),c
    ; Save the current byte at #C000 in IX (safe, preserved across the test).
    ld a,(#c000)
    ld ixl,a
    ; Write the test pattern #AA and immediately read it back.
    ld (#c000),#aa
    ld a,(#c000)
    ; Restore the original byte before switching the bank back.
    ld (#c000),ixl
    ; Restore the normal RAM map (config 0 = default power-on layout, page irrelevant).
    ld bc,GA_PORT_CMD+GA_CMD_RAM_BANK+GA_RAM_CFG_0
    out (c),c
    ; If the read-back did not match #AA, this page is absent -- stop counting.
    cp #aa
    jr nz,.done
    ; This page is confirmed -- add 64K to the running total.
    ld bc,64
    add hl,bc
    ; Advance to the next page (each page step = +GA_RAM_PAGE_1 = +8 in bits 5-3).
    ld a,d
    add a,GA_RAM_PAGE_1
    ld d,a
    ; Stop after page 7 (adding 8 to #38 wraps bits 5-3 back to 0).
    cp GA_RAM_PAGE_0
    jr nz,.page_loop
.done:
    ret


ENDIF
