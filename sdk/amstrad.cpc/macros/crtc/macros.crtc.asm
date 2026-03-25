;-----------------------------------------------------------------------
;
; Retrodev SDK -- Amstrad CPC CRTC macro definitions.
;
; Purpose: provide reusable macros to configure HD6845/compatible CRTC
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

IFNDEF __MACROS_CRTC_ASM__
__MACROS_CRTC_ASM__ equ 1


; CRTC port addresses
;
CRTC_PORT_ADDR		equ #bc00	; address port -- write register index here to select it
CRTC_PORT_DATA		equ #bd00	; data port    -- write value here after selecting a register

; CRTC register indices (HD6845 / compatible)
;
CRTC_R0_HTOT		equ 0		; Horizontal Total
CRTC_R1_HDIS		equ 1		; Horizontal Displayed
CRTC_R2_HSYNC		equ 2		; Horizontal Sync Position
CRTC_R3_SYNC_W		equ 3		; Sync Width (horizontal + vertical)
CRTC_R4_VTOT		equ 4		; Vertical Total
CRTC_R5_VTADJ		equ 5		; Vertical Total Adjust
CRTC_R6_VDIS		equ 6		; Vertical Displayed
CRTC_R7_VSYNC		equ 7		; Vertical Sync Position
CRTC_R8_INTERLACE	equ 8		; Interlace and Skew
CRTC_R9_MAXRAS		equ 9		; Maximum Raster Address
CRTC_R12_ADDR_H		equ 12		; Display Start Address (high byte)
CRTC_R13_ADDR_L		equ 13		; Display Start Address (low byte)

; Disable flags -- define any of these symbols before including this file to suppress
; the corresponding register write entirely (the macro will assemble to zero bytes).
;
; Example: to skip the VSYNC position write, add this before the include:
;   DisableVSyncPos equ 1
;
; Available disable symbols:
;   DisableHTOT    DisableHDIS         DisableHSyncPos    DisableSyncWidth
;   DisableVTOT    DisableVDIS         DisableVSyncPos    DisableMaxRaster
;   DisableInterlaceSkew               DisableAddress

; Horizontal Total (HTOT) Register 0
; Effect is immediate
; Default value is 63
; Represents the total scanline length in characters, including borders and hsync.
; Must be >= HDIS + HSYNC width. Typical CPC value: 63.
; Define DisableHTOT to suppress this macro.
;
macro CRTC_SetHTOT HTOT
	ifndef DisableHTOT
		ld bc,CRTC_PORT_ADDR+CRTC_R0_HTOT
		out (c),c
		ld bc,CRTC_PORT_DATA+{HTOT}
		out (c),c
	endif
endm

; Horizontal Displayed (HDIS) Register 1
; Effect is immediate
; Default value is 40
; Number of character columns visible on screen per scanline (excluding borders).
; Increasing this widens the display area. Typical CPC value: 40.
; Define DisableHDIS to suppress this macro.
;
macro CRTC_SetHDIS HDIS
	ifndef DisableHDIS
		ld bc,CRTC_PORT_ADDR+CRTC_R1_HDIS
		out (c),c
		ld bc,CRTC_PORT_DATA+{HDIS}
		out (c),c
	endif
endm

; Horizontal Sync Pulse Position (HSYNC) Register 2
; Effect is immediate
; Default value is 46
; Character position at which the HSYNC pulse begins.
; Must be greater than HDIS so the sync pulse falls outside the visible area.
; Define DisableHSyncPos to suppress this macro.
;
macro CRTC_SetHSyncPos HSyncPos
	ifndef DisableHSyncPos
		ld bc,CRTC_PORT_ADDR+CRTC_R2_HSYNC
		out (c),c
		ld bc,CRTC_PORT_DATA+{HSyncPos}
		out (c),c
	endif
endm

; Sync Pulse Width (horizontal + vertical) Register 3
; Effect is immediate
; Default value is #8e (8 lines of vsync, 14 chars of hsync)
; Bits 7-4: vertical sync width in scan lines (CRTC0 only; ignored on CRTC1/CRTC2 which fix it at 16).
;           A value of 0 means 16 lines of vsync pulse on CRTC0.
; Bits 3-0: horizontal sync width in characters. A value of 0 disables hsync.
; The value must be loaded into A by the caller before invoking this macro.
; Define DisableSyncWidth to suppress this macro.
;
macro CRTC_SetSyncWidth
	ifndef DisableSyncWidth
		ld bc,CRTC_PORT_ADDR+CRTC_R3_SYNC_W
		out (c),c
		ld bc,CRTC_PORT_DATA
		out (c),a
	endif
endm

; Vertical Total (VTOT) Register 4
; Effect is immediate (takes effect at the next frame)
; Default value is 38
; Total number of character rows per frame, including borders and vsync.
; Must be >= VDIS.
; Define DisableVTOT to suppress this macro.
;
macro CRTC_SetVTOT VTOT
	ifndef DisableVTOT
		ld bc,CRTC_PORT_ADDR+CRTC_R4_VTOT
		out (c),c
		ld bc,CRTC_PORT_DATA+{VTOT}
		out (c),c
	endif
endm

; Missing R5 -- Vertical Total Adjust (VTADJ)
; Fine-tunes the frame period by adding extra scan lines (0-31) after VTOT rows.
; Not implemented here; write directly using CRTC_R5_VTADJ if sub-row frame timing is required.

; Vertical Displayed (VDIS) Register 6
; Effect is immediate (takes effect at the next frame)
; Default value is 25
; Number of character rows visible on screen (excluding borders).
; Increasing this shows more rows. Must be <= VTOT.
; Define DisableVDIS to suppress this macro.
;
macro CRTC_SetVDIS VDIS
	ifndef DisableVDIS
		ld bc,CRTC_PORT_ADDR+CRTC_R6_VDIS
		out (c),c
		ld bc,CRTC_PORT_DATA+{VDIS}
		out (c),c
	endif
endm

; Vertical Sync Position (VSYNC) Register 7
; Effect is immediate (takes effect at the next frame)
; Default value is 30
; Character row at which the VSYNC pulse begins.
; Must be greater than VDIS so the sync pulse falls outside the visible area.
; Define DisableVSyncPos to suppress this macro.
;
macro CRTC_SetVSyncPos VSYNCPOS
	ifndef DisableVSyncPos
		ld bc,CRTC_PORT_ADDR+CRTC_R7_VSYNC
		out (c),c
		ld bc,CRTC_PORT_DATA+{VSYNCPOS}
		out (c),c
	endif
endm

; Interlace and Skew Register 8
; Effect is immediate
; Default value is #00 (no interlace, no skew -- CPC firmware default)
;
; Bit layout:
;   Bits 1-0 -- Interlace mode:
;     00 = Normal / non-interlace (default)
;     01 = Interlace Sync
;     10 = Normal (reserved, same as 00)
;     11 = Interlace Sync and Video
;   Bits 3-2 -- unused, must be 0
;   Bits 5-4 -- Display skew (delays DISPEN signal by N character clocks):
;     00 = no skew (default)
;     01 = 1 character clock delay
;     10 = 2 character clock delay
;     11 = display output disabled
;   Bits 7-6 -- Cursor skew (same encoding as bits 5-4, applies to cursor only)
;
; CRTC compatibility:
;   Type 0 (HD6845S)  -- fully supported: interlace + display skew + cursor skew
;   Type 1 (UM6845R)  -- interlace bits work; display and cursor skew bits are ignored
;   Type 2 (MC6845)   -- fully supported
;   Type 3 (CPC+ ASIC)-- R8 is ignored entirely; behaviour is fixed in hardware
;   Type 4 (MC6845B)  -- fully supported
;
; Practical notes for CPC development:
;   - The CPC firmware initialises R8 to #00 and most software never changes it.
;   - Interlace is rarely useful on CPC -- it halves the effective frame rate (25 Hz)
;     and is not supported by the GA palette or the firmware display routines.
;   - Display skew (bits 5-4) is relevant on CRTC type 0 when pushing horizontal
;     timing to the limit: if HDIS is set very close to HTOT, a skew of 1 can
;     prevent a single garbage pixel column at the right edge of the display.
;   - Changing R8 mid-frame is not recommended; apply it during VSYNC.
;
; The value to write must be built by the caller and passed as the macro parameter.
; Example -- 1-char display skew, no interlace: CRTC_SetInterlaceSkew #10
; Define DisableInterlaceSkew to suppress this macro.
;
macro CRTC_SetInterlaceSkew Value
	ifndef DisableInterlaceSkew
		ld bc,CRTC_PORT_ADDR+CRTC_R8_INTERLACE
		out (c),c
		ld bc,CRTC_PORT_DATA+{Value}
		out (c),c
	endif
endm

; Maximum Raster Address (MAXRAS) Register 9
; Effect is immediate (takes effect at the next frame)
; Default value is 7 (8 scan lines per character row on the CPC)
; Defines the number of scan lines per character row minus 1.
; Standard CPC values: 7 (Mode 0/1/2), 3 (hardware double-height trick).
; Define DisableMaxRaster to suppress this macro.
;
macro CRTC_SetMaxRaster Max
	ifndef DisableMaxRaster
		ld bc,CRTC_PORT_ADDR+CRTC_R9_MAXRAS
		out (c),c
		ld bc,CRTC_PORT_DATA+{Max}
		out (c),c
	endif
endm

; Display Start Address Register 12 (high byte) and Register 13 (low byte)
; Effect is latched at the next VSYNC -- change during the frame for smooth scrolling.
; The 14-bit address (bits 13-0) maps to the CRTC address space (each unit = 2 bytes on CPC).
; HL must be loaded with the target address by the caller before invoking this macro.
; H -> R12 (high 6 bits, bits 13-8), L -> R13 (low byte, bits 7-0).
; Define DisableAddress to suppress this macro.
;
macro CRTC_SetAddress
	ifndef DisableAddress
		ld bc,CRTC_PORT_ADDR+CRTC_R12_ADDR_H		; select R12 -- Display Start Address (high)
		out (c),c
		inc b						; switch to data port (CRTC_PORT_DATA)
		out (c),h					; write high byte
		dec b						; switch back to address port (CRTC_PORT_ADDR)
		inc c						; advance to R13 -- Display Start Address (low)
		out (c),c
		inc b						; switch to data port (CRTC_PORT_DATA)
		out (c),l					; write low byte
	endif
endm


ENDIF
