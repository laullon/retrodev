
EnableVSyncPos		equ 1
EnableVTOT			equ 1
;EnableSyncWidth		equ 1

; Horizontal Total (HTOT) Register 0
; Effect is inmediate
; Default value is 63
; Represent the lenght in chars for a complete scanline including borders and hsync size
;
macro CRTC_SetHTOT HTOT
	ld bc,#bc00
	out (c),c
	ld bc,#bd00+{HTOT}
	out (c),c
endm

; Horizontal Displayed (HDIS) Register 1
; Effect is inmediate
; Default value is 40
; Represent the lenght in chars for the displayable characters in a scanline. 
;
macro CRTC_SetHDIS HDIS
	ld bc,#bc01
	out (c),c
	ld bc,#bd00+{HDIS}
	out (c),c
endm

; Horizontal Sync Pulse Position (HSYNCP) Register 2
; Effect is inmediate
; Default value is 46
; Represents the position to start the HSYNC pulse in characters for the current scanline
; Should be greather than HDIS (since HSYNC should start after HDIS finished)
;
macro CRTC_SetHSyncPos HSyncPos
	ld bc,#bc02
	out (c),c
	ld bc,#bd00+{HSyncPos}
	out (c),c
endm

; Sync pulse width (horizontal / vertical)
; Effect is inmediate
; Default value is #8e
; This register uses 4bits for each component but vertical one cannot  be changed on CRTC1 and CRTC2. 
; A value of 0 in vertical has special meaning (16 lines of vsync pulse)
; Value to set should be provided in A
macro CRTC_SetSyncWidth
	ifdef EnableSyncWidth
		ld bc,#bc03
		out (c),c
		ld bc,#bd00
		out (c),a
	endif
endm



; Vertical Total (VTOT) Register 4
;
;
macro CRTC_SetVTOT VTOT
	ifdef EnableVTOT
		ld bc,#bc04			
		out (c),c
		ld bc,#bd00+{VTOT}
		out (c),c
	endif
endm

; Missing R5, vertical raster adjust

;
;
macro CRTC_SetVDIS VDIS
	ld bc,#bc06	
	out (c),c		
	ld bc,#bd00+{VDIS}
	out (c),c	
endm

macro CRTC_SetVSyncPos VSYNCPOS
	ifdef EnableVSyncPos
		ld bc,#bc07			
		out (c),c
		ld bc,#bd00 + {VSYNCPOS}
		out (c),c
	endif
endm

; Missing R8 (interlace + skew)

macro CRTC_SetMaxRaster Max
	ld bc,#bc09
	out (c),c
	ld bc,#bd00 + {Max}
	out (c),c
endm


macro CRTC_SetAddress:
	ld bc,#bc0c			;Display Start Address (H)
	out (c),c
	inc b
	out (c),h
	dec b				;Display Start Address (L)
	inc c
	out (c),c
	inc b
	out (c),l
endm

