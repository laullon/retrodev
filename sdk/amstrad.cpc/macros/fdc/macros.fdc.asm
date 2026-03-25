;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC FDC macro definitions.
;
; Purpose: define uPD765 controller constants and helper macros.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

IFNDEF __MACROS_FDC_ASM__
__MACROS_FDC_ASM__ equ 1


; NEC uPD765 FDC definitions and helper macros for the Amstrad CPC.
;
; The NEC uPD765A (and compatible usPD765B) is the floppy disc controller
; fitted in the CPC 664, CPC 6128, and the external DDI-1 disc interface
; for the CPC 464. It is absent on a bare 464 without the disc interface.
;
; All FDC I/O goes through two ports:
;   Main Status Register (#FB7E, read-only)  -- handshake and busy state.
;   Data Register        (#FA7E, read/write) -- command, data, and result bytes.
; A separate motor-control latch at #FB7F (write-only) switches drive motors.
;
; Every command follows the same three-phase protocol:
;   Command phase  : CPU writes command byte(s) to the data register.
;   Execution phase: FDC performs the operation (seek, read, write, ...).
;   Result phase   : CPU reads result byte(s) from the data register.
; The Main Status Register must be polled before every byte transfer to
; ensure the FDC is ready and the data direction is correct.

; ============================================================
; PORT ADDRESSES
; ============================================================
;
; FDC_PORT_STATUS (#FB7E, read-only)
;   Main Status Register. Must be read before every data register access.
;   Bit 7 (RQM)  = 1: FDC ready for a data register transfer.
;   Bit 6 (DIO)  = 1: FDC->CPU (result/read); 0: CPU->FDC (command/write).
;   Bit 5 (NDM)  = 1: FDC in non-DMA execution phase.
;   Bit 4 (CB)   = 1: FDC busy (command in progress).
;   Bits 3-0     = drive 3-0 busy (seek in progress on that drive).
;
; FDC_PORT_DATA (#FA7E, read/write)
;   Data Register. Used for all command, data, and result byte transfers.
;   Read or write only when RQM=1 in the Main Status Register.
;   DIO determines direction: 0 = write to FDC; 1 = read from FDC.
;
; FDC_PORT_MOTOR (#FB7F, write-only)
;   Motor latch. Bit 0 = drive A motor; bit 1 = drive B motor.
;   Writing 0 stops both motors; writing FDC_MOTOR_A|FDC_MOTOR_B runs both.
;   Allow ~300 ms after motor-on before issuing any read/write command.
;
FDC_PORT_STATUS		equ #fb7e		; main status register (read-only)
FDC_PORT_STATUS_B	equ #fb		    ; high byte of FDC_PORT_STATUS
FDC_PORT_DATA		equ #fa7e		; data register (read/write)
FDC_PORT_DATA_B		equ #fa		    ; high byte of FDC_PORT_DATA
FDC_PORT_MOTOR		equ #fb7f		; motor control latch (write-only)
FDC_PORT_MOTOR_B	equ #fb		    ; high byte of FDC_PORT_MOTOR

; ============================================================
; MAIN STATUS REGISTER BITS  (read from FDC_PORT_STATUS)
; ============================================================
FDC_MSR_RQM			equ #80		; bit 7: Request for Master -- FDC ready for transfer
FDC_MSR_DIO			equ #40		; bit 6: Data I/O -- 1=FDC->CPU, 0=CPU->FDC
FDC_MSR_NDM			equ #20		; bit 5: Non-DMA mode execution phase active
FDC_MSR_CB			equ #10		; bit 4: FDC busy (command in progress)
FDC_MSR_D3B			equ #08		; bit 3: drive 3 seek in progress
FDC_MSR_D2B			equ #04		; bit 2: drive 2 seek in progress
FDC_MSR_D1B			equ #02		; bit 1: drive 1 seek in progress
FDC_MSR_D0B			equ #01		; bit 0: drive 0 seek in progress

; ============================================================
; MOTOR CONTROL BITS  (write to FDC_PORT_MOTOR)
; ============================================================
FDC_MOTOR_A			equ #01		; bit 0: drive A motor on
FDC_MOTOR_B			equ #02		; bit 1: drive B motor on
FDC_MOTOR_OFF		equ #00		; all motors off

; ============================================================
; COMMAND BYTES
; ============================================================
;
; Each command is sent as one or more bytes to FDC_PORT_DATA during the
; command phase. Bit 7 of the first byte is the MFM flag on read/write
; commands; bit 6 is the SK (skip deleted data) flag.
;
; SPECIFY (#03) -- set step rate, head load/unload timing, and DMA mode.
;   Byte 0: #03
;   Byte 1: (SRT << 4) | HUT   -- step rate time | head unload time
;   Byte 2: (HLT << 1) | ND    -- head load time | non-DMA flag
;   No result phase.
;
; SENSE DRIVE STATUS (#04) -- read drive/head status into ST3.
;   Byte 0: #04
;   Byte 1: (HD << 2) | DS    -- head select | drive select
;   Result: ST3 (1 byte)
;
; RECALIBRATE (#07) -- seek to track 0.
;   Byte 0: #07
;   Byte 1: DS                 -- drive select (0 or 1)
;   No result phase. Poll FDC_MSR_D0B/D1B until clear, then SENSE INTERRUPT.
;
; SENSE INTERRUPT STATUS (#08) -- read ST0 + PCN after SEEK or RECALIBRATE.
;   Byte 0: #08
;   Result: ST0 (1 byte), PCN (1 byte -- present cylinder number)
;   Must be issued after every SEEK or RECALIBRATE command.
;
; SEEK (#0F) -- seek to a specific track.
;   Byte 0: #0F
;   Byte 1: (HD << 2) | DS    -- head select | drive select
;   Byte 2: NCN                -- new cylinder number
;   No result phase. Poll busy bit, then SENSE INTERRUPT.
;
; READ DATA (#46 MFM, #66 MFM+SK) -- read one or more sectors.
;   Byte 0: #46 or #66         -- command (#40=MFM, #20=SK)
;   Byte 1: (HD << 2) | DS
;   Byte 2: C   -- cylinder
;   Byte 3: H   -- head
;   Byte 4: R   -- sector ID (#C1-#C9 on standard CPC disc)
;   Byte 5: N   -- sector size code (2 = 512 bytes)
;   Byte 6: EOT -- last sector number on track
;   Byte 7: GPL -- gap length (#2A standard)
;   Byte 8: DTL -- data length (#FF when N>0)
;   Result: ST0, ST1, ST2, C, H, R, N (7 bytes)
;
; WRITE DATA (#45 MFM) -- write one or more sectors.
;   Same 9-byte command phase as READ DATA, command byte #45.
;   Result: ST0, ST1, ST2, C, H, R, N (7 bytes)
;
; FORMAT TRACK (#4D MFM) -- format a full track.
;   Byte 0: #4D
;   Byte 1: (HD << 2) | DS
;   Byte 2: N   -- sector size code
;   Byte 3: SC  -- sectors per track
;   Byte 4: GPL -- gap length
;   Byte 5: D   -- fill byte
;   Then CPU writes (SC * 4) ID bytes during execution phase.
;   Result: ST0, ST1, ST2, C, H, R, N (7 bytes)
;
; READ ID (#4A MFM) -- read the next sector ID from the current track.
;   Byte 0: #4A
;   Byte 1: (HD << 2) | DS
;   Result: ST0, ST1, ST2, C, H, R, N (7 bytes)
;
FDC_CMD_SPECIFY			equ #03		; set step rate and head timing
FDC_CMD_SENSE_DRIVE		equ #04		; sense drive status -> ST3
FDC_CMD_RECALIBRATE		equ #07		; seek to track 0
FDC_CMD_SENSE_INT		equ #08		; sense interrupt status -> ST0, PCN
FDC_CMD_SEEK			equ #0F		; seek to cylinder
FDC_CMD_READ_ID			equ #4A		; read next sector ID (MFM flag not included)
FDC_CMD_READ			equ #46		; read data (MFM, no skip)
FDC_CMD_READ_SKIP		equ #66		; read data (MFM, skip deleted)
FDC_CMD_WRITE			equ #45		; write data (MFM)
FDC_CMD_FORMAT			equ #4D		; format track (MFM)
; ---- Command flag bits (OR into command byte where applicable) ----
FDC_FLAG_MFM			equ #40		; bit 6: MFM encoding (always set for CPC)
FDC_FLAG_SK				equ #20		; bit 5: skip deleted-data address marks

; ============================================================
; STATUS REGISTER 0 (ST0) BITS
; ============================================================
;
; Returned as the first result byte of READ, WRITE, FORMAT, READ ID,
; and SENSE INTERRUPT STATUS commands.
;
FDC_ST0_IC1				equ #80		; bit 7: interrupt code MSB (see IC values below)
FDC_ST0_IC0				equ #40		; bit 6: interrupt code LSB
FDC_ST0_IC_MASK			equ #C0		; bits 7-6: interrupt code field
FDC_ST0_IC_NORMAL		equ #00		; IC=00: normal termination
FDC_ST0_IC_ABNORMAL		equ #40		; IC=01: abnormal termination (error)
FDC_ST0_IC_INVALID		equ #80		; IC=10: invalid command
FDC_ST0_IC_READY_CHANGE	equ #C0		; IC=11: drive ready changed during operation
FDC_ST0_SE				equ #20		; bit 5: seek end -- head positioned successfully
FDC_ST0_EC				equ #10		; bit 4: equipment check -- track 0 not reached
FDC_ST0_NR				equ #08		; bit 3: not ready -- drive not ready
FDC_ST0_HD				equ #04		; bit 2: head address at time of interrupt
FDC_ST0_DS1				equ #02		; bit 1: drive select bit 1
FDC_ST0_DS0				equ #01		; bit 0: drive select bit 0

; ============================================================
; STATUS REGISTER 1 (ST1) BITS
; ============================================================
;
; Returned as the second result byte of READ, WRITE, FORMAT, and READ ID.
;
FDC_ST1_EN				equ #80		; bit 7: end of cylinder -- sector ID > EOT
FDC_ST1_CRC				equ #20		; bit 5: CRC error in ID or data field
FDC_ST1_OR				equ #10		; bit 4: overrun -- CPU too slow during transfer
FDC_ST1_ND				equ #04		; bit 2: no data -- sector ID not found
FDC_ST1_NW				equ #02		; bit 1: not writable -- write-protect tab active
FDC_ST1_MA				equ #01		; bit 0: missing address mark

; ============================================================
; STATUS REGISTER 2 (ST2) BITS
; ============================================================
;
; Returned as the third result byte of READ, WRITE, FORMAT, and READ ID.
;
FDC_ST2_CM				equ #40		; bit 6: control mark -- deleted data address mark found
FDC_ST2_CRC				equ #20		; bit 5: CRC error in data field only
FDC_ST2_WC				equ #10		; bit 4: wrong cylinder -- found C != expected C
FDC_ST2_SN				equ #08		; bit 3: scan not satisfied
FDC_ST2_SH				equ #04		; bit 2: scan equal hit
FDC_ST2_BC				equ #02		; bit 1: bad cylinder -- C field = #FF
FDC_ST2_MD				equ #01		; bit 0: missing data address mark

; ============================================================
; STATUS REGISTER 3 (ST3) BITS
; ============================================================
;
; Returned as the result byte of SENSE DRIVE STATUS (#04).
;
FDC_ST3_FT				equ #80		; bit 7: fault signal from drive (not used on CPC)
FDC_ST3_WP				equ #40		; bit 6: write protect -- disc tab covered
FDC_ST3_RDY				equ #20		; bit 5: ready -- drive is ready
FDC_ST3_T0				equ #10		; bit 4: track 0 -- head is at track 0
FDC_ST3_TS				equ #08		; bit 3: two-side -- disc is double-sided
FDC_ST3_HD				equ #04		; bit 2: head select -- currently selected head
FDC_ST3_DS1				equ #02		; bit 1: drive select bit 1
FDC_ST3_DS0				equ #01		; bit 0: drive select bit 0

; ============================================================
; STANDARD CPC DISC GEOMETRY
; ============================================================
;
; Standard CPC Data format (single-sided, double-density MFM):
;   40 tracks, 1 head, 9 sectors per track, 512 bytes per sector.
;   Sector IDs: #C1-#C9 on every track (AMSDOS convention).
;   Gap lengths: GPL=#2A (read/write), GPL=#52 (format).
;   Blocks: 1K = 2 consecutive sectors; 0-1 = directory, 2+ = data.
;
FDC_DISC_TRACKS			equ 40		; tracks per side
FDC_DISC_HEADS			equ 1		; heads (sides) -- standard CPC is single-sided
FDC_DISC_SECTORS		equ 9		; sectors per track
FDC_DISC_SECTOR_SIZE	equ 512		; bytes per sector
FDC_DISC_SECTOR_N		equ 2		; sector size code (N=2 -> 128 << 2 = 512 bytes)
FDC_DISC_SECTOR_BASE	equ #C1		; first sector ID on each track
FDC_DISC_GPL_RW			equ #2A		; gap length for read/write commands
FDC_DISC_GPL_FMT		equ #52		; gap length for format command
FDC_DISC_DTL			equ #FF		; DTL value when N > 0 (ignored by FDC)
FDC_DISC_FILL			equ #E5		; format fill byte (CP/M standard)

; ============================================================
; SPECIFY COMMAND PARAMETERS  (standard CPC settings)
; ============================================================
;
; Byte 1: (SRT << 4) | HUT
;   SRT (Step Rate Time)    = 6  ->  6 ms per step at 4 MHz (bits 7-4)
;   HUT (Head Unload Time)  = 15 -> 240 ms               (bits 3-0)
;   Byte 1 = (#6 << 4) | #F = #6F
;
; Byte 2: (HLT << 1) | ND
;   HLT (Head Load Time)    = 2  ->  4 ms                (bits 7-1)
;   ND  (Non-DMA)           = 1  -> always 1 on CPC      (bit 0)
;   Byte 2 = (#2 << 1) | 1 = #05
;
FDC_SPECIFY_SRT_HUT		equ #6F		; SPECIFY byte 1: SRT=6ms, HUT=240ms
FDC_SPECIFY_HLT_ND		equ #05		; SPECIFY byte 2: HLT=4ms, ND=1 (non-DMA)

; ============================================================
; MOTOR SPIN-UP DELAY
; ============================================================
;
; The drive motor requires at least 300 ms to reach full speed after
; being switched on. The loop constant below targets ~320 ms at 4 MHz.
; Outer * (256 inner * 13 T djnz + ~22 T overhead) ~ 8.4 M T ~ 320 ms.
;
FDC_MOTOR_SPINUP_LOOPS	equ 2500	; outer loop count for ~320 ms spin-up delay

; ============================================================
; FDC_WaitRQM -- spin until the FDC Main Status Register shows RQM=1.
;
; Polls FDC_PORT_STATUS until bit 7 (RQM) is set, indicating the FDC is
; ready for the next data register transfer. Must be called before every
; read or write of the data register during command and result phases.
;
; Does not check DIO -- the caller is responsible for transferring in the
; correct direction after this macro returns.
;
; Destroys: A, B
; Preserves: C, DE, HL, IX, IY
;
; Speed: non-deterministic. Each poll = 12 T (in a,(c)) + 4 T (rla) + branch.
macro FDC_WaitRQM
    ld b,FDC_PORT_STATUS_B
@wait:
    in a,(c)
    rla
    jr nc,@wait
endm

; ============================================================
; FDC_WaitRQM_Read -- spin until FDC is ready AND DIO=1 (FDC->CPU).
;
; Like FDC_WaitRQM but also confirms the transfer direction is FDC->CPU
; before returning. Use before reading a result byte from FDC_PORT_DATA.
;
; Destroys: A, B
; Preserves: C, DE, HL, IX, IY
;
; Speed: non-deterministic. Each poll ~ 28 T.
macro FDC_WaitRQM_Read
    ld b,FDC_PORT_STATUS_B
@wait:
    in a,(c)
    and FDC_MSR_RQM|FDC_MSR_DIO
    cp FDC_MSR_RQM|FDC_MSR_DIO
    jr nz,@wait
endm

; ============================================================
; FDC_WaitRQM_Write -- spin until FDC is ready AND DIO=0 (CPU->FDC).
;
; Like FDC_WaitRQM but also confirms the transfer direction is CPU->FDC
; before returning. Use before writing a command or data byte to FDC_PORT_DATA.
;
; Destroys: A, B
; Preserves: C, DE, HL, IX, IY
;
; Speed: non-deterministic. Each poll ~ 28 T.
macro FDC_WaitRQM_Write
    ld b,FDC_PORT_STATUS_B
@wait:
    in a,(c)
    and FDC_MSR_RQM|FDC_MSR_DIO
    cp FDC_MSR_RQM
    jr nz,@wait
endm

; ============================================================
; FDC_WriteByte -- wait for RQM (CPU->FDC) then write one byte to the data register.
;
; Combines FDC_WaitRQM_Write and the data register write into one macro.
; Value is the byte to send; it must be an 8-bit immediate or register expression
; that can be loaded into A.
;
; Destroys: A, B, C
; Preserves: DE, HL, IX, IY
;
; Speed: poll loop (non-deterministic) + 7 T (ld c) + 12 T (out).
macro FDC_WriteByte Value
    FDC_WaitRQM_Write
    ld b,FDC_PORT_DATA_B
    ld c,{Value}
    out (c),c
endm

; ============================================================
; FDC_ReadByte -- wait for RQM (FDC->CPU) then read one byte from the data register.
;
; Combines FDC_WaitRQM_Read and the data register read into one macro.
; Result lands in A.
;
; Destroys: A, B, C
; Preserves: DE, HL, IX, IY
;
; Speed: poll loop (non-deterministic) + 7 T (ld b,DATA_B) + 12 T (in a,(c)).
macro FDC_ReadByte
    FDC_WaitRQM_Read
    ld b,FDC_PORT_DATA_B
    in a,(c)
endm

; ============================================================
; FDC_MotorOn -- switch the motor on for the selected drive and wait for spin-up.
;
; Drive must be 0 (drive A) or 1 (drive B). After writing the motor latch
; the macro busy-waits for FDC_MOTOR_SPINUP_LOOPS iterations (~320 ms).
; This is the minimum safe delay before issuing any read or write command.
;
; Destroys: A, B, C, HL (used as outer loop counter)
; Preserves: DE, IX, IY
;
; Speed: ~320 ms at 4 MHz (deterministic delay, not bus-dependent).
macro FDC_MotorOn Drive
    ld bc,FDC_PORT_MOTOR
    if {Drive} == 0
        ld a,FDC_MOTOR_A
    else
        ld a,FDC_MOTOR_B
    endif
    out (c),a
    ld hl,FDC_MOTOR_SPINUP_LOOPS
@outer:
    ld b,0
@inner:
    djnz @inner
    dec hl
    ld a,h
    or l
    jr nz,@outer
endm

; ============================================================
; FDC_MotorOff -- switch all drive motors off.
;
; Destroys: A, B, C
; Preserves: DE, HL, IX, IY
macro FDC_MotorOff
    ld bc,FDC_PORT_MOTOR
    xor a
    out (c),a
endm

; ============================================================
; FDC_Specify -- issue the SPECIFY command with standard CPC timing values.
;
; Sends FDC_CMD_SPECIFY, FDC_SPECIFY_SRT_HUT, FDC_SPECIFY_HLT_ND.
; This configures:  SRT=6ms, HUT=240ms, HLT=4ms, ND=1 (non-DMA mode).
; Must be called once after motor-on and before any seek or read/write.
;
; Destroys: A, B, C
; Preserves: DE, HL, IX, IY
macro FDC_Specify
    FDC_WriteByte FDC_CMD_SPECIFY
    FDC_WriteByte FDC_SPECIFY_SRT_HUT
    FDC_WriteByte FDC_SPECIFY_HLT_ND
endm

; ============================================================
; FDC_Recalibrate -- seek drive to track 0 and confirm with SENSE INTERRUPT.
;
; Issues RECALIBRATE for the given drive, polls until the drive is no longer
; busy, then issues SENSE INTERRUPT STATUS and discards the result bytes.
; After this macro the head is at track 0.
;
; Drive must be 0 or 1.
;
; Destroys: A, B, C
; Preserves: DE, HL, IX, IY
macro FDC_Recalibrate Drive
    FDC_WriteByte FDC_CMD_RECALIBRATE
    FDC_WriteByte {Drive}
    ld b,FDC_PORT_STATUS_B
@busy:
    in a,(c)
    and FDC_MSR_CB
    jr nz,@busy
    FDC_WriteByte FDC_CMD_SENSE_INT
    FDC_ReadByte
    FDC_ReadByte
endm

; ============================================================
; FDC_Seek -- seek to a specific cylinder and confirm with SENSE INTERRUPT.
;
; Issues SEEK for the given drive/head/cylinder, polls until the drive busy
; bit clears, then issues SENSE INTERRUPT STATUS. ST0 is left in A on exit;
; the caller can test FDC_ST0_IC_MASK and FDC_ST0_SE to verify success.
;
; Drive must be 0 or 1. Head must be 0 (CPC standard discs are single-sided).
; Cylinder is the target track number (0-39 on a standard CPC disc).
;
; Destroys: A, B, C
; Preserves: DE, HL, IX, IY
;
; Exit: A = ST0 (caller checks FDC_ST0_IC_MASK == FDC_ST0_IC_NORMAL and FDC_ST0_SE set)
macro FDC_Seek Drive, Head, Cylinder
    FDC_WriteByte FDC_CMD_SEEK
    FDC_WriteByte ({Head} << 2) | {Drive}
    FDC_WriteByte {Cylinder}
    ld b,FDC_PORT_STATUS_B
@busy:
    in a,(c)
    and FDC_MSR_D0B << {Drive}
    jr nz,@busy
    FDC_WriteByte FDC_CMD_SENSE_INT
    FDC_ReadByte
endm


ENDIF
