;-----------------------------------------------------------------------
;
; Retrodev SDK -- CPC ROM file routines.
;
; Purpose: firmware ROM-based file load/save helpers.
;
; (c) TLOTB 2026
;
;-----------------------------------------------------------------------

IFNDEF __CPC_FILE_ROM_ASM__
__CPC_FILE_ROM_ASM__ equ 1

; Require macros.firmware.asm to be included before this file.
IFNDEF __MACROS_FIRMWARE_ASM__
FAIL 'cpc.file.rom.asm requires macros.firmware.asm to be included first'
ENDIF

; CPC_FileLoadROM
;
; Uses the CAS_IN_OPEN / CAS_IN_DIRECT / CAS_IN_CLOSE firmware vectors to read a file.
; On a machine with the AMSDOS disc ROM active (664 or 6128, or 464 with disc interface),
; the CAS_IN_OPEN vector is transparently redirected by AMSDOS to the disc subsystem.
; On a 464 without disc, the same vectors access the cassette. The function is therefore
; portable across all CPC configurations without any additional switching logic.
;
; Firmware vectors used:
;   FW_CAS_IN_OPEN    (#BC77)
;     In:  HL = 16-byte filename buffer (null-padded, upper-case)
;          DE = 64-byte header buffer (filled by AMSDOS on success)
;     Out: Carry set = file opened; A = file type
;          Carry clear = not found or error
;
;   FW_CAS_IN_DIRECT  (#BC83)
;     In:  HL = destination address, DE = byte count
;     Out: Carry set = all bytes read; Carry clear = EOF or error
;
;   FW_CAS_IN_CLOSE   (#BC7A)
;     In:  none
;     Out: Carry set = closed cleanly; Carry clear = checksum error
;
;   FW_CAS_IN_ABANDON (#BC7D)
;     In:  none -- call after a failed CAS_IN_DIRECT to release the stream
;
; AMSDOS header buffer layout (64 bytes, filled by CAS_IN_OPEN):
;   Bytes 1-8   : filename (space-padded)
;   Bytes 9-11  : extension
;   Byte  18    : file type (0=BASIC, 1=protected, 2=binary, #16=ASCII)
;   Bytes 21-22 : load address (LE16)
;   Bytes 24-25 : file length low 16 bits (LE16)
;
; Entry:
;   HL = pointer to filename string (null-terminated or space-padded, up to 16 bytes,
;        upper-case ASCII)
;   DE = destination address, or 0 to use the load address stored in the file header
;
; Exit:
;   Carry set   = success; HL = destination address used; BC = bytes loaded
;   Carry clear = failure (not found, read error, or close error); A = 0
;
; Destroys: AF, BC, DE, HL
; Preserves: IX, IY, AF', BC', DE', HL'
;
; Requirements:
;   Firmware ROM must be present. AMSDOS ROM must be active for disc access.
;   Interrupts must be enabled (the CAS_ routines require them).

; ============================================================
; Header buffer field offsets (relative to the 64-byte CAS_IN_OPEN output buffer)
; ============================================================
CASHDR_LOAD_LO		equ 21		; load address, low byte  (LE16 at bytes 21-22)
CASHDR_LOAD_HI		equ 22		; load address, high byte
CASHDR_LEN_LO		equ 24		; file length, low byte   (LE16 at bytes 24-25)
CASHDR_LEN_HI		equ 25		; file length, high byte

; ============================================================
; Static buffers
; ============================================================
;
; 16-byte buffer for the null-padded copy of the caller's filename.
CAS_NameBuf		ds 16
;
; 64-byte buffer filled by FW_CAS_IN_OPEN with the AMSDOS file header.
CAS_HeaderBuf	ds 64

; ============================================================
; Stack frame at the point of the CAS_IN_DIRECT call:
;
;   [pushed at entry]   IY (preserved for caller)
;   [pushed at entry]   IX (preserved for caller)
;   [pushed at entry]   dest_override (DE at entry; low=E high=D)
;   [pushed before read] load_addr (HL after we decide which dest to use)
;   [pushed before read] byte_count (BC from header)
;
; After CAS_IN_CLOSE succeeds:
;   pop BC  -> byte_count
;   pop HL  -> load address used
;   pop DE  -> dest_override (discard)
;   pop IX
;   pop IY
; ============================================================

; CPC_FileLoadROM -- load a named file from disc or tape using the firmware CAS_ vectors.
; Entry: HL = pointer to filename (null-terminated, up to 16 bytes, upper-case ASCII)
;        DE = destination address, or 0 to use the load address from the file header
; Exit:  Carry set = success; HL = destination address used; BC = bytes loaded
;        Carry clear = failure; A = 0
; Destroys: AF, BC, DE, HL  Preserves: IX, IY
CPC_FileLoadROM:
    push iy
    push ix
    ; Preserve caller's destination override on the stack.
    push de				; [stack: IY, IX, dest_override]
    ; Copy caller's filename (HL) into CAS_NameBuf, null-padding to 16 bytes.
    ld de,CAS_NameBuf
    ld b,16
.copy_name:
    ld a,(hl)
    or a
    jr z,.pad_name
    ld (de),a
    inc hl
    inc de
    djnz .copy_name
    jr .name_ready
.pad_name:
    xor a
.pad_loop:
    ld (de),a
    inc de
    djnz .pad_loop
.name_ready:
    ; Open the file: HL = name buffer, DE = header buffer.
    ld hl,CAS_NameBuf
    ld de,CAS_HeaderBuf
    call FW_CAS_IN_OPEN
    jr nc,.fail_open
    ; Read load address and file length out of the header buffer.
    ld hl,CAS_HeaderBuf + CASHDR_LOAD_LO
    ld e,(hl)
    inc hl
    ld d,(hl)			; DE = load address from header (LE16)
    ld hl,CAS_HeaderBuf + CASHDR_LEN_LO
    ld c,(hl)
    inc hl
    ld b,(hl)			; BC = file length from header (LE16)
    ; Decide on the destination address.
    ; Pop the dest_override to examine it, then re-push.
    pop hl				; HL = dest_override
    push hl				; restore dest_override to stack
    ld a,h
    or l
    jr z,.use_header_load
    ; Caller supplied a non-zero override -- use it as the destination.
    ; HL = caller's dest override (just peeked). Keep it in HL.
    jr .dest_decided
.use_header_load:
    ; Override is zero -- use the AMSDOS load address already in DE.
    ld h,d
    ld l,e				; HL = header load address
.dest_decided:
    ; HL = destination to use; BC = byte count.
    ; Push load_addr (HL) and byte_count (BC) for the return values.
    push hl				; [stack: IY, IX, dest_override, load_addr]
    push bc				; [stack: IY, IX, dest_override, load_addr, byte_count]
    ; Set up CAS_IN_DIRECT: HL = destination, DE = byte count.
    ; HL is the saved load_addr; restore it by peeking back (pop/push), then set DE from BC.
    ld d,b
    ld e,c				; DE = byte count
    ; Restore HL = destination from the load_addr slot on the stack.
    ; Stack top = byte_count, next = load_addr. Use a temp register exchange.
    pop bc				; BC = byte_count (temp -- pop to access load_addr below)
    pop hl				; HL = load_addr = destination
    push hl				; restore load_addr
    push bc				; restore byte_count
    ; HL = destination, DE = byte count -- correct for CAS_IN_DIRECT.
    call FW_CAS_IN_DIRECT
    jr nc,.fail_read
    ; Close the file.
    call FW_CAS_IN_CLOSE
    jr nc,.fail_close
    ; Success: pop return values and restore registers.
    pop bc				; BC = bytes loaded
    pop hl				; HL = load address / dest used
    pop de				; discard dest_override
    pop ix
    pop iy
    scf
    ret

.fail_read:
    ; CAS_IN_DIRECT failed -- release stream then return error.
    call FW_CAS_IN_ABANDON
    ; Fall through to common fail cleanup.
    jr .fail_cleanup_2

.fail_close:
    ; CAS_IN_CLOSE reported an error (tape checksum on last block, etc.).
    jr .fail_cleanup_2

.fail_cleanup_2:
    ; Stack: [IY, IX, dest_override, load_addr, byte_count] -- 3 extra entries.
    pop bc				; discard byte_count
    pop hl				; discard load_addr
    pop de				; discard dest_override
    pop ix
    pop iy
    xor a
    or a
    ret

.fail_open:
    ; CAS_IN_OPEN failed -- only dest_override is on the extra stack slot.
    pop de				; discard dest_override
    pop ix
    pop iy
    xor a
    or a
    ret


ENDIF
