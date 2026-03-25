;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC premixed palette colours.
;
; Purpose: SDK module.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

IFNDEF __CPC_PALETTE_PREMIXED_ASM__
__CPC_PALETTE_PREMIXED_ASM__ equ 1


; Require macros.ga.asm to be included before this file.
IFNDEF __MACROS_GA_ASM__
FAIL 'cpc.palette.premixed.asm requires macros.ga.asm to be included first'
ENDIF

; GA_SetPalettePremixed -- set N consecutive ink pens from a table of pre-mixed GA colour bytes.
;
; This is the fastest
; contains the exact byte to write to the GA colour port (bit 6 set, bits 4-0 = hw colour index).
; No conversion is performed -- each byte is written directly to the Gate Array.
;
; Use this variant when your palette table is exported by the Retrodev export script using
; the "pre-mixed GA command" format, or when you have pre-computed the table yourself.
;
; Entry:
;   HL = pointer to table of pre-mixed GA colour bytes (one byte per pen)
;   C  = number of pens to set (1-17; pen 0 is the first ink, pen 16 is the border)
;
; Each table byte must have the form:  %01xhhhhh
;   bit 6    = 1 (GA colour command flag, must be set)
;   bits 4-0 = hardware colour index (0-26)
; Example: hardware colour 0 (white) -> #40 | 0 = #54... see SysToHw table for full mapping.
;
; Pens are set starting from pen 0 up to pen C-1, in order.
; To set a subset of pens, advance HL to the first desired pen and adjust C accordingly,
; but note that the pen index written to the GA always starts at 0 and increments -- to set
; arbitrary pens, use the GA_SetBorder macro or write your own indexed loop instead.
;
; Destroys: A, B, C, D, E, HL, F
; Preserves: IX, IY, BC', DE', HL'
;
; Speed (4 MHz Z80, T-states per iteration):
;   ld c,e       =  4  (load pen index into C)
;   out (c),c    = 12  (select pen -- GA pen-select command: C = pen index, bit 7:5 = 000)
;   ld c,(hl)    =  7  (load pre-mixed colour byte)
;   inc hl       =  6  (advance table pointer)
;   out (c),c    = 12  (write colour -- GA colour command: bit 6 set, bits 4-0 = hw colour)
;   inc e        =  4  (next pen index)
;   dec d        =  4  (decrement loop counter)
;   jr nz        = 12  (loop -- 7 T on final iteration)
;                ----
;                 61 T per iteration (56 T on last)
;
; For 16 pens: setup(~30 T) + 15*61 + 56 = 971 T ~ 243 uss
;
; Optimization note: B=GA_PORT_CMD_B is loaded once and held for the entire loop. The GA port address
; is #7fxx -- B carries the high byte, C carries the command byte. This avoids reloading BC
; from an immediate (10 T) on every iteration, saving 10 T * N compared to ld bc,nn in loop.

; GA_SetPalettePremixed -- set N consecutive pens from a table of pre-mixed GA colour bytes.
; Entry: HL = pointer to table of pre-mixed GA colour bytes (bit 6 set, bits 4-0 = hw index)
;        C  = number of pens to set (1-17; pen 0 = first ink, pen 16 = border)
; Exit:  pens 0..C-1 written to the Gate Array
; Destroys: AF, BC, DE, HL  Preserves: IX, IY
GA_SetPalettePremixed:
        ld d,c
        xor a			; A = 0 -- pen index starts at pen 0
        ld e,a			; E = pen index (preserved across loop; A is reused for table reads)
        ld b,GA_PORT_CMD_B	; B = GA port high byte; held for the entire loop
.loop:
        ld c,e			; C = current pen index (pen-select command: bits 7-5=000, bits 4-0=pen)
        out (c),c		; send pen-select command to Gate Array
        ld c,(hl)		; C = pre-mixed colour byte (#40 | hw_colour_index) from table
        inc hl			; advance table pointer to next entry
        out (c),c		; send colour command to Gate Array (bit 6 = 1 already set in table byte)
        inc e			; advance to next pen index
        dec d			; one fewer pen to set
        jr nz,.loop		; continue until all N pens are written
        ret


ENDIF
