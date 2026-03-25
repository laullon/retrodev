;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC CRTC detection routines.
;
; Purpose: detect CRTC type and expose compatible timing behaviour flags.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

IFNDEF __CPC_DETECT_CRTC_ASM__
__CPC_DETECT_CRTC_ASM__ equ 1

; Require macros.crtc.asm to be included before this file.
IFNDEF __MACROS_CRTC_ASM__
FAIL 'cpc.detect.crtc.asm requires macros.crtc.asm to be included first'
ENDIF

; CPC_DetectCRTC
;
; Five CRTC variants were used across CPC models. They differ in which internal
; registers can be read back and in how they handle reads of write-only registers.
; This routine uses those differences to uniquely identify the chip type.
;
; CRTC types and their CPC usage:
;   Type 0 -- HD6845S    : early CPC 464 / 664; status register present; R12 readable,
;                         returns bits 5-0 only (bits 7-6 always read as 0)
;   Type 1 -- UM6845R    : most CPC 464 / 664 / some 6128; no status register;
;                         R12 readable, returns full 8 bits
;   Type 2 -- MC6845     : some CPC 464 / 664; no status register; R12 not readable,
;                         read always returns #FF
;   Type 3 -- AMS40489   : CPC+ / GX4000 ASIC; R12 not readable, read returns #00;
;                         VSYNC width programmable; Status register not present.
;   Type 4 -- AMS40226   : some 6128 Plus boards; behaves like type 3 but minor diffs.
;
; Detection algorithm:
;
;   Step 1 -- distinguish types 0/1 from types 2/3/4.
;     Write #FF to CRTC R12 (display start high), then read it back.
;     Types 0 and 1 return a non-FF value (bits masked or full byte).
;     Types 2, 3, 4 either return #FF (type 2) or #00 (types 3/4).
;
;   Step 2 -- separate type 2 from types 3/4.
;     Type 2 reads as #FF; types 3/4 read as #00.
;     (Tested in the same read from Step 1.)
;
;   Step 3 -- separate type 0 from type 1.
;     Type 0 (HD6845S) has a status register at the address port (read side).
;     Reading the CRTC address port on a type 0 returns a status byte with
;     bits 7-6 potentially set (VSYNC and LPEN flags). On types 1/2/3/4 the
;     address port returns #FF on read (floating bus or pull-up).
;     We write #00 to R12 first so bits 5-0 of the type-0 read are known,
;     then read the address port: if the result is not #FF it is type 0.
;
;   Step 4 -- separate type 3 from type 4.
;     Both return #00 on R12 reads. Type 4 has a readable status register
;     at the address port (like type 0) whereas type 3 does not.
;     Same address-port read used in Step 3 differentiates them.
;
; The routine saves and restores R12 so the display start address is not affected.
;
; Return values (A on exit):
;   CRTC_TYPE_0 = 0   HD6845S
;   CRTC_TYPE_1 = 1   UM6845R
;   CRTC_TYPE_2 = 2   MC6845
;   CRTC_TYPE_3 = 3   AMS40489 (CPC+ / GX4000)
;   CRTC_TYPE_4 = 4   AMS40226 (some 6128 Plus)
;
; Entry:
;   none
; Exit:
;   A = CRTC type (0-4); see CRTC_TYPE_n equs below
; Destroys: AF, BC, IXL
; Preserves: DE, HL, IXH, IY
; Requirements: none (safe to call with interrupts enabled, though DI is recommended
;               to prevent the firmware from reprogramming R12 during the test)

CRTC_TYPE_0		equ 0		; HD6845S  -- early 464 / 664
CRTC_TYPE_1		equ 1		; UM6845R  -- most 464 / 664 / some 6128
CRTC_TYPE_2		equ 2		; MC6845   -- some 464 / 664
CRTC_TYPE_3		equ 3		; AMS40489 -- CPC+ / GX4000
CRTC_TYPE_4		equ 4		; AMS40226 -- some 6128 Plus

; CPC_DetectCRTC -- identify the CRTC chip type fitted in the CPC (types 0-4).
; Entry: none
; Exit:  A = CRTC type: CRTC_TYPE_0..CRTC_TYPE_4
; Destroys: AF, BC, IXL  Preserves: DE, HL, IXH, IY
CPC_DetectCRTC:
    ; Select R12 (display start high) via the CRTC address port.
    ; R12 stays selected at the address register for the remainder of the R12 test.
    ld bc,CRTC_PORT_ADDR+CRTC_R12_ADDR_H
    out (c),c
    ; Read the current value of R12 via the data port and save it in IXL for later restore.
    ld b,#bd
    in a,(c)
    ld ixl,a
    ; Write #FF to R12 -- the most discriminating test value.
    ld a,#ff
    out (c),a
    ; Read R12 back -- result in A reveals the CRTC type.
    in a,(c)
    ; Restore the original R12 value immediately (R12 is still selected).
    ld c,ixl
    out (c),c
    ; Analyse the read-back:
    ;   #FF -> type 2 (MC6845 -- registers not readable, bus floats high)
    ;   #00 -> types 3 or 4 (AMS ASIC -- registers not readable, returns #00)
    ;   other -> types 0 or 1 (register is readable; bits may be masked)
    cp #ff
    jr z,.type2
    or a
    jr z,.type3_or_4
    ; Types 0 or 1: distinguish via the CRTC address-port status register.
    ; On type 0 (HD6845S) reading the address port returns a live status byte;
    ; bit 5 = VSYNC, bit 6 = LPEN -- not all bits are simultaneously set.
    ; On type 1 (UM6845R) the address port read returns #FF (bus pull-up).
    ; Select R0 first to leave R12 undisturbed by the address-port read.
    ld bc,CRTC_PORT_ADDR+CRTC_R0_HTOT
    out (c),c
    ; Read the address port (B is already #BC from the ld bc above).
    in a,(c)
    ; #FF = type 1; anything else = type 0.
    cp #ff
    jr z,.type1
    ld a,CRTC_TYPE_0
    ret
.type1:
    ld a,CRTC_TYPE_1
    ret
.type2:
    ld a,CRTC_TYPE_2
    ret
.type3_or_4:
    ; Both type 3 (AMS40489) and type 4 (AMS40226) return #00 on R12 reads.
    ; Type 4 has a readable address-port status register like type 0;
    ; type 3 returns #FF on address-port reads (no status register).
    ld bc,CRTC_PORT_ADDR+CRTC_R0_HTOT
    out (c),c
    in a,(c)
    cp #ff
    jr z,.type3
    ld a,CRTC_TYPE_4
    ret
.type3:
    ld a,CRTC_TYPE_3
    ret


ENDIF
