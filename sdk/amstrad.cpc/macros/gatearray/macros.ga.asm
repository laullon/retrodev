;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC Gate Array macros.
;
; Purpose: define Gate Array ports, colours, and control helper macros.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

IFNDEF __MACROS_GA_ASM__
__MACROS_GA_ASM__ equ 1


; Gate Array (GA) helper macros for the Amstrad CPC.
;
; ============================================================
; PORT ADDRESSES
; ============================================================
;
; GA_PORT_CMD (#7F00) -- Gate Array command port (write only).
;   B must be #7F to address the GA. C carries the command byte.
;   The command type is determined by bits 7-6 of the byte written to C.
;
; GA_PORT_VSYNC (#F500) -- PPI Port A (read, when configured as input).
;   Bit 0 = VSYNC signal from CRTC (1 = VSYNC active).
;   Used to poll for vertical sync without involving interrupts.
;
GA_PORT_CMD		equ #7f00	; Gate Array command port (write, B=GA_PORT_CMD_B)
GA_PORT_CMD_B	equ #7f		; high byte of GA_PORT_CMD -- load into B for out (c),c writes
GA_PORT_VSYNC	equ #f500	; PPI Port A -- VSYNC status bit (read)
GA_PORT_VSYNC_B	equ #f5		; high byte of GA_PORT_VSYNC -- load into B for in a,(c) polling

; ============================================================
; COMMAND TYPE PREFIXES  (bits 7-6 of the command byte)
; ============================================================
;
; The GA decodes bits 7-6 of every byte written to #7Fxx to determine
; which of the four command groups the write belongs to.
;
GA_CMD_PEN_SELECT	equ #00		; bits 7-6 = 00 -- select active pen
GA_CMD_COLOR_SET	equ #40		; bits 7-6 = 01 -- set colour for selected pen
GA_CMD_MODE_ROM		equ #80		; bits 7-6 = 10 -- screen mode and ROM enable/disable
GA_CMD_RAM_BANK		equ #c0		; bits 7-6 = 11 -- RAM banking (6128 / CPC+ only)

; ============================================================
; PEN SELECT COMMAND  (GA_CMD_PEN_SELECT | pen_index)
; ============================================================
;
; Written as:  out (#7f), GA_CMD_PEN_SELECT | pen
; Selects which pen will be targeted by the next colour-set command.
; Must always be followed by a GA_CMD_COLOR_SET write.
;
; Pen indices:
;   0-15 = ink pens (screen colours in current mode)
;   16   = border pen
;
GA_PEN_0		equ GA_CMD_PEN_SELECT|0		; ink pen 0
GA_PEN_1		equ GA_CMD_PEN_SELECT|1		; ink pen 1
GA_PEN_2		equ GA_CMD_PEN_SELECT|2		; ink pen 2
GA_PEN_3		equ GA_CMD_PEN_SELECT|3		; ink pen 3
GA_PEN_4		equ GA_CMD_PEN_SELECT|4		; ink pen 4
GA_PEN_5		equ GA_CMD_PEN_SELECT|5		; ink pen 5
GA_PEN_6		equ GA_CMD_PEN_SELECT|6		; ink pen 6
GA_PEN_7		equ GA_CMD_PEN_SELECT|7		; ink pen 7
GA_PEN_8		equ GA_CMD_PEN_SELECT|8		; ink pen 8
GA_PEN_9		equ GA_CMD_PEN_SELECT|9		; ink pen 9
GA_PEN_10		equ GA_CMD_PEN_SELECT|10	; ink pen 10
GA_PEN_11		equ GA_CMD_PEN_SELECT|11	; ink pen 11
GA_PEN_12		equ GA_CMD_PEN_SELECT|12	; ink pen 12
GA_PEN_13		equ GA_CMD_PEN_SELECT|13	; ink pen 13
GA_PEN_14		equ GA_CMD_PEN_SELECT|14	; ink pen 14
GA_PEN_15		equ GA_CMD_PEN_SELECT|15	; ink pen 15
GA_PEN_BORDER	equ GA_CMD_PEN_SELECT|16	; border pen

; ============================================================
; COLOUR SET COMMAND  (GA_CMD_COLOR_SET | hw_color_index)
; ============================================================
;
; Written as:  out (#7f), GA_CMD_COLOR_SET | hw_color
; Sets the hardware colour for the pen selected by the most recent
; GA_CMD_PEN_SELECT write. Must always follow a pen-select command.
;
; Hardware colour indices are not in RGB order -- they are the physical
; register values of the GA ASIC. 27 colours are available (indices 0-26).
; See the SysToHw table in cpc.palette.syscolors.asm for the mapping
; from firmware colour numbers (0-26) to these hardware indices.
;
; GA_COLOR_n constants are pre-mixed GA_CMD_COLOR_SET|hw_index bytes.
GA_COLOR_0		equ #54						; Black
GA_COLOR_1		equ #44						; Blue
GA_COLOR_2		equ #55						; Bright Blue
GA_COLOR_3		equ #5C						; Red
GA_COLOR_4		equ #58						; Magenta
GA_COLOR_5		equ #5D						; Mauve
GA_COLOR_6		equ #4C						; Bright Red
GA_COLOR_7		equ #45						; Purple
GA_COLOR_8		equ #4D						; Bright Magenta
GA_COLOR_9		equ #56						; Green
GA_COLOR_10		equ #46						; Cyan
GA_COLOR_11		equ #57						; Sky Blue
GA_COLOR_12		equ #5E						; Yellow
GA_COLOR_13		equ #40						; White
GA_COLOR_14		equ #5F						; Pastel Blue
GA_COLOR_15		equ #4E						; Orange
GA_COLOR_16		equ #47						; Pink
GA_COLOR_17		equ #4F						; Pastel Magenta
GA_COLOR_18		equ #52						; Bright Green
GA_COLOR_19		equ #42						; Sea Green
GA_COLOR_20		equ #53						; Bright Cyan
GA_COLOR_21		equ #5A						; Lime
GA_COLOR_22		equ #59						; Pastel Green
GA_COLOR_23		equ #5B						; Pastel Cyan
GA_COLOR_24		equ #4A						; Bright Yellow
GA_COLOR_25		equ #43						; Pastel Yellow
GA_COLOR_26		equ #4B						; Bright White

; Named colour aliases for easier IntelliSense discovery.
GA_COLOR_BLACK			equ GA_COLOR_0
GA_COLOR_BLUE			equ GA_COLOR_1
GA_COLOR_BRIGHT_BLUE	equ GA_COLOR_2
GA_COLOR_RED			equ GA_COLOR_3
GA_COLOR_MAGENTA		equ GA_COLOR_4
GA_COLOR_MAUVE			equ GA_COLOR_5
GA_COLOR_BRIGHT_RED		equ GA_COLOR_6
GA_COLOR_PURPLE			equ GA_COLOR_7
GA_COLOR_BRIGHT_MAGENTA	equ GA_COLOR_8
GA_COLOR_GREEN			equ GA_COLOR_9
GA_COLOR_CYAN			equ GA_COLOR_10
GA_COLOR_SKY_BLUE		equ GA_COLOR_11
GA_COLOR_YELLOW			equ GA_COLOR_12
GA_COLOR_WHITE			equ GA_COLOR_13
GA_COLOR_PASTEL_BLUE	equ GA_COLOR_14
GA_COLOR_ORANGE			equ GA_COLOR_15
GA_COLOR_PINK			equ GA_COLOR_16
GA_COLOR_PASTEL_MAGENTA	equ GA_COLOR_17
GA_COLOR_BRIGHT_GREEN	equ GA_COLOR_18
GA_COLOR_SEA_GREEN		equ GA_COLOR_19
GA_COLOR_BRIGHT_CYAN	equ GA_COLOR_20
GA_COLOR_LIME			equ GA_COLOR_21
GA_COLOR_PASTEL_GREEN	equ GA_COLOR_22
GA_COLOR_PASTEL_CYAN	equ GA_COLOR_23
GA_COLOR_BRIGHT_YELLOW	equ GA_COLOR_24
GA_COLOR_PASTEL_YELLOW	equ GA_COLOR_25
GA_COLOR_BRIGHT_WHITE	equ GA_COLOR_26

; ============================================================
; SCREEN MODE AND ROM CONTROL COMMAND  (GA_CMD_MODE_ROM | flags)
; ============================================================
;
; Written as:  out (#7f), GA_CMD_MODE_ROM | mode | rom_flags | irq_flags
; Controls the screen mode, lower/upper ROM visibility, and the interrupt counter.
; This is the most frequently used GA command after palette writes.
;
; Bits 1-0 -- Screen mode:
;   GA_MODE_0 = 160x200, 16 colours (2 pixels per byte)
;   GA_MODE_1 = 320x200,  4 colours (4 pixels per byte)
;   GA_MODE_2 = 640x200,  2 colours (8 pixels per byte)
;   GA_MODE_3 = 160x200,  4 colours -- undocumented, not supported by firmware
;
; Bit 2 -- Lower ROM (#0000-#3FFF):
;   GA_LROM_ENABLE  = 0 -- lower ROM visible (firmware / BASIC)
;   GA_LROM_DISABLE = 1 -- lower ROM hidden, RAM at #0000-#3FFF
;
; Bit 3 -- Upper ROM (#C000-#FFFF):
;   GA_UROM_ENABLE  = 0 -- upper ROM visible (firmware / BASIC)
;   GA_UROM_DISABLE = 1 -- upper ROM hidden, RAM at #C000-#FFFF
;
; Bit 4 -- Interrupt counter reset:
;   GA_IRQ_RESET = sets this bit -- clears the GA's 52-HSYNC interrupt counter
;                  and cancels any pending interrupt request.
;                  Write it once; the bit auto-clears internally.
;                  Used in interrupt handlers that need to re-synchronise timing.
;
; Bits 7-5 = 100 (encoded in GA_CMD_MODE_ROM -- do not modify)
;
GA_MODE_0		equ GA_CMD_MODE_ROM|0		; Mode 0: 160x200, 16 colours
GA_MODE_1		equ GA_CMD_MODE_ROM|1		; Mode 1: 320x200,  4 colours
GA_MODE_2		equ GA_CMD_MODE_ROM|2		; Mode 2: 640x200,  2 colours
GA_MODE_3		equ GA_CMD_MODE_ROM|3		; Mode 3: undocumented -- avoid
GA_LROM_ENABLE	equ 0					; bit 2 = 0: lower ROM enabled
GA_LROM_DISABLE	equ #04					; bit 2 = 1: lower ROM disabled (RAM visible)
GA_UROM_ENABLE	equ 0					; bit 3 = 0: upper ROM enabled
GA_UROM_DISABLE	equ #08					; bit 3 = 1: upper ROM disabled (RAM visible)
GA_IRQ_RESET	equ #10					; bit 4 = 1: reset GA interrupt line counter

; ============================================================
; RAM BANKING COMMAND  (GA_CMD_RAM_BANK | page | config)  -- 6128 / CPC+ only
; ============================================================
;
; Written as:  out (#7f), GA_CMD_RAM_BANK | GA_RAM_PAGE_n | GA_RAM_CFG_n
; Has no effect on a 464 or 664 (no extra RAM fitted).
;
; The command byte breaks down as:
;   Bits 7-6 = 11          (GA_CMD_RAM_BANK prefix -- must not be modified)
;   Bits 5-3 = RAM page    (which 64K block of extra RAM to use -- see GA_RAM_PAGE_n)
;   Bits 2-0 = RAM config  (slot layout for the selected page -- see GA_RAM_CFG_n)
;
; ---- RAM PAGE (bits 5-3) ----------------------------------------
;
; Selects which 64K block of expanded RAM the config operates on.
; A stock 6128 has only one extra 64K block -- page 0.
; Pages 1-7 require additional hardware (DK'Tronics 256K, CPC+ ASIC, etc.).
;
GA_RAM_PAGE_0	equ #00		; page 0 -- the only extra 64K on a standard 6128
GA_RAM_PAGE_1	equ #08		; page 1 -- expanded hardware only
GA_RAM_PAGE_2	equ #10		; page 2 -- expanded hardware only
GA_RAM_PAGE_3	equ #18		; page 3 -- expanded hardware only
GA_RAM_PAGE_4	equ #20		; page 4 -- expanded hardware only
GA_RAM_PAGE_5	equ #28		; page 5 -- expanded hardware only
GA_RAM_PAGE_6	equ #30		; page 6 -- expanded hardware only
GA_RAM_PAGE_7	equ #38		; page 7 -- expanded hardware only

; ---- RAM CONFIG (bits 2-0) --------------------------------------
;
; Defines which physical 16K RAM chunks appear in each of the four
; address slots. The Z80 address space is split into four 16K slots:
;   Slot 0 = #0000-#3FFF    Slot 1 = #4000-#7FFF
;   Slot 2 = #8000-#BFFF    Slot 3 = #C000-#FFFF
;
; The base 6128 has four built-in RAM banks numbered 0-3 (one per slot
; at power-on). The extra 64K on a 6128 provides four more banks: 4-7.
;
; Config  Slot 0    Slot 1    Slot 2    Slot 3
; ------  --------  --------  --------  --------
;   0     bank 0    bank 1    bank 2    bank 3    normal map -- power-on default
;   1     bank 0    bank 1    bank 2    bank 7    slot 3 -> extra bank 7
;   2     bank 4    bank 5    bank 6    bank 7    all slots -> extra banks 4-7
;   3     bank 0    bank 3    bank 2    bank 7    slot 1 -> bank 3, slot 3 -> extra bank 7
;   4     bank 0    bank 4    bank 2    bank 3    slot 1 -> extra bank 4
;   5     bank 0    bank 5    bank 2    bank 3    slot 1 -> extra bank 5
;   6     bank 0    bank 6    bank 2    bank 3    slot 1 -> extra bank 6
;   7     bank 0    bank 7    bank 2    bank 3    slot 1 -> extra bank 7
;
; Notes:
;   Configs 4-7 are the primary way to page a single extra 16K bank
;   into slot 1 (#4000-#7FFF) while keeping the normal map elsewhere.
;   Config 2 is the only layout that puts all four extra banks in view
;   simultaneously (useful for fast block copies between extra RAM banks).
;   Config 3 is rarely used; it aliases bank 3 into slot 1 and brings
;   extra bank 7 into slot 3, displacing the normal upper RAM.
;   Slot 0 is always bank 0 in all configs -- the Z80 reset vector and
;   firmware at #0000 are never displaced by this register.
;
GA_RAM_CFG_0	equ 0		; normal: 0,1,2,3 (power-on default)
GA_RAM_CFG_1	equ 1		; slot 3 -> extra bank 7:  0,1,2,7
GA_RAM_CFG_2	equ 2		; all extra: 4,5,6,7
GA_RAM_CFG_3	equ 3		; slot 1->bank 3, slot 3->extra bank 7:  0,3,2,7
GA_RAM_CFG_4	equ 4		; slot 1 -> extra bank 4:  0,4,2,3
GA_RAM_CFG_5	equ 5		; slot 1 -> extra bank 5:  0,5,2,3
GA_RAM_CFG_6	equ 6		; slot 1 -> extra bank 6:  0,6,2,3
GA_RAM_CFG_7	equ 7		; slot 1 -> extra bank 7:  0,7,2,3

; ============================================================
; GA_SetRAMBank -- switch RAM banking configuration (6128 / CPC+ only).
;
; Writes a RAM banking command to the Gate Array, selecting which physical
; 16K RAM chunks appear in each address slot. Page selects which 64K block
; of extra RAM to use; Config selects the slot layout within that page.
;
; Usage:
;   GA_SetRAMBank GA_RAM_PAGE_0, GA_RAM_CFG_5   ; slot 1 -> extra bank 5
;   GA_SetRAMBank GA_RAM_PAGE_0, GA_RAM_CFG_0   ; restore normal map
;
; Has no effect on 464 or 664 hardware (no extra RAM fitted).
; Page values above 0 require expanded hardware (DK'Tronics, CPC+ ASIC, etc.).
;
; Destroys: B, C
;
; Speed: 10 + 12 = 22 T (constant, no branching).
;
macro GA_SetRAMBank Page, Config
	ld bc,GA_PORT_CMD+GA_CMD_RAM_BANK+{Page}+{Config}	; B=#7f, C=RAM bank command byte
	out (c),c											; send RAM bank command to Gate Array
endm

; ============================================================
; SPEED REFERENCE
; ============================================================
;
; Speed notes (Z80 at 4 MHz, 1 T-state = 0.25 uss, figures in T-states):
;   ld b,n      =  7 T  (2 NOPs)
;   ld bc,nn    = 10 T  (3 NOPs)
;   ld c,n      =  7 T  (2 NOPs)
;   in a,(c)    = 12 T  (3 NOPs)  -- may extend due to CRTC/contention on real hardware
;   rra         =  4 T  (1 NOP)
;   jr cc,rel   = 12 T taken / 8 T not taken
;   out (c),c   = 12 T  (3 NOPs)
;
; Polling loop cost per iteration (taken branch):
;   in a,(c) + rra + jr cc = 12+4+12 = 28 T = 7 uss
;   This is the minimum possible for a single-bit test on a Z80 --
;   'bit 0,a + jr z' would cost 8+12 = 20 T for the test alone vs rra(4)+jr(12) = 16 T,
;   so the rra+jr pattern is already optimal for speed.
;


; GA_VSyncWait -- wait for VSYNC to go inactive then active (full frame sync).
;
; Waits for the current VSYNC to end (@loop1 spins while VSYNC is active, i.e. carry set),
; then waits for the next VSYNC to begin (@loop2 spins while VSYNC is inactive, i.e. carry clear).
; This guarantees the code following this macro always starts at the very top of the VSYNC pulse,
; regardless of where in the frame the macro is called.
;
; If DebugVSync is defined, GA_SetBorder is called with Color between the two phases,
; painting the border for the inter-vsync window as a visual timing marker.
; B must not be relied upon after this macro -- it is left as #f5 on exit.
;
; Destroys: A, B, C, F
; Does not destroy: DE, HL
;
; Speed: non-deterministic (depends on frame position at call time).
;        Worst case ~ one full frame period (approximately 19968 T-states at 50 Hz).
;
macro GA_VSyncWait Color
	ld b,GA_PORT_VSYNC_B
@loop1:
	in a,(c)		; read PPI Port A -- bit 0 = VSYNC signal from CRTC
	rra				; rotate bit 0 into carry (carry set = VSYNC active)
	jr nc,@loop1	; loop while VSYNC is not yet active (wait for it to end if already on)
	ifdef DebugVSync
	GA_SetBorder {Color}	; paint border as visual marker (corrupts B and C)
	ld b,GA_PORT_VSYNC_B			; restore B for the port read below
	endif
@loop2:
	in a,(c)		; read PPI Port A again
	rra				; carry set = VSYNC still active
	jr c,@loop2		; loop until VSYNC goes inactive -> we are now at the VSYNC leading edge
endm


; GA_VSyncWaitON -- wait until VSYNC becomes active (rising edge).
;
; Spins until bit 0 of PPI Port A goes high (VSYNC pulse starts).
; Use this when you only need to synchronise to the start of VSYNC
; and are certain you are calling it outside an active VSYNC.
; If called during an active VSYNC the loop exits immediately.
;
; Destroys: A, B, C, F
;
; Speed: non-deterministic. Loop body = 28 T per iteration.
;
macro GA_VSyncWaitON
	ld b,GA_PORT_VSYNC_B
@loop:
	in a,(c)		; read PPI Port A
	rra				; carry = VSYNC bit
	jr nc,@loop		; loop while VSYNC is not active
endm


; GA_VSyncWaitOFF -- wait until VSYNC becomes inactive (falling edge).
;
; Spins until bit 0 of PPI Port A goes low (VSYNC pulse ends).
; Use this to wait for the end of the VSYNC window, for example to measure
; its duration or to synchronise to the exact line after blanking.
; If called outside an active VSYNC the loop exits immediately.
;
; Destroys: A, B, C, F
;
; Speed: non-deterministic. Loop body = 28 T per iteration.
;        VSYNC pulse lasts approximately 2 character rows = 16 scan lines ~ 1024 T.
;
macro GA_VSyncWaitOFF
	ld b,GA_PORT_VSYNC_B
@loop:
	in a,(c)		; read PPI Port A
	rra				; carry = VSYNC bit
	jr c,@loop		; loop while VSYNC is still active
endm


; GA_SetBorder -- set the CPC border colour via the Gate Array.
;
; Color must be a pre-mixed GA colour byte: GA_COLOR_n or GA_CMD_COLOR_SET|hw_index.
;
; Destroys: B, C
;
; Speed: 10 + 12 + 7 + 12 = 41 T (constant, no branching).
;
macro GA_SetBorder Color
	ld bc,GA_PORT_CMD+GA_PEN_BORDER		; B=#7f (GA port), C=GA_PEN_BORDER (border pen select)
	out (c),c							; send pen select to Gate Array
	ld c,{Color}						; C = pre-mixed GA colour byte (GA_COLOR_n)
	out (c),c							; send colour command to Gate Array
endm


; GA_SetInk -- set the colour for an arbitrary ink pen (0-15).
;
; Pen must be 0-15 (use GA_SetBorder for the border pen).
; Color must be a pre-mixed GA colour byte: GA_COLOR_n or GA_CMD_COLOR_SET|hw_index.
;
; Destroys: B, C
;
; Speed: 10 + 12 + 7 + 12 = 41 T (constant, no branching).
;
macro GA_SetInk Pen, Color
	ld bc,GA_PORT_CMD+GA_CMD_PEN_SELECT+{Pen}	; B=#7f (GA port), C=pen select command
	out (c),c									; send pen select to Gate Array
	ld c,{Color}								; C = pre-mixed GA colour byte (GA_COLOR_n)
	out (c),c									; send colour command to Gate Array
endm


; GA_SetMode -- set screen mode and ROM visibility in a single GA write.
;
; Writes one byte to the GA mode/ROM command group. The Mode parameter
; must be one of GA_MODE_0, GA_MODE_1, GA_MODE_2 (or GA_MODE_3 if needed).
; ROM and IRQ flags can be OR-ed in at the call site:
;
;   GA_SetMode GA_MODE_1                              ; mode 1, ROMs on, no IRQ reset
;   GA_SetMode GA_MODE_1|GA_UROM_DISABLE              ; mode 1, upper ROM off
;   GA_SetMode GA_MODE_0|GA_LROM_DISABLE|GA_IRQ_RESET ; mode 0, lower ROM off, reset IRQ counter
;
; Note: the full mode+ROM state is replaced on every write -- there is no
; read-modify-write on the GA. Always supply all desired flags together.
;
; Destroys: B, C
;
; Speed: 10 + 12 = 22 T (constant, no branching).
;
macro GA_SetMode Mode
	ld bc,GA_PORT_CMD+{Mode}	; B=#7f (GA port), C=mode/ROM command byte
	out (c),c					; send mode command to Gate Array
endm


; GA_SetLROM -- enable or disable lower ROM (#0000-#3FFF).
;
; Convenience wrapper around GA_SetMode for callers that only need to
; toggle lower ROM visibility without changing the screen mode or upper ROM.
; Provide the current mode and upper ROM state in ModeFlags so the write
; does not accidentally alter them:
;
;   GA_SetLROM GA_MODE_1, GA_LROM_DISABLE   ; mode 1, hide lower ROM
;   GA_SetLROM GA_MODE_1, GA_LROM_ENABLE    ; mode 1, show lower ROM
;
; Destroys: B, C
;
; Speed: 10 + 12 = 22 T (constant, no branching).
;
macro GA_SetLROM ModeFlags, LRomFlag
	ld bc,GA_PORT_CMD+{ModeFlags}+{LRomFlag}	; B=#7f, C=mode command with LROM bit
	out (c),c									; send mode command to Gate Array
endm


; GA_SetUROM -- enable or disable upper ROM (#C000-#FFFF).
;
; Convenience wrapper around GA_SetMode for callers that only need to
; toggle upper ROM visibility without changing the screen mode or lower ROM.
; Provide the current mode and lower ROM state in ModeFlags so the write
; does not accidentally alter them:
;
;   GA_SetUROM GA_MODE_1, GA_UROM_DISABLE   ; mode 1, hide upper ROM
;   GA_SetUROM GA_MODE_1, GA_UROM_ENABLE    ; mode 1, show upper ROM
;
; Destroys: B, C
;
; Speed: 10 + 12 = 22 T (constant, no branching).
;
macro GA_SetUROM ModeFlags, URomFlag
	ld bc,GA_PORT_CMD+{ModeFlags}+{URomFlag}	; B=#7f, C=mode command with UROM bit
	out (c),c									; send mode command to Gate Array
endm


ENDIF
