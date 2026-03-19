


macro GA_VSyncWait Color
	ld b,#f5	
@loop1:
	in a,(c)
	rra			
	jr nc,@loop1
	ifdef DebugVSync
	GA_SetBorder {Color}
	ld b,#f5
	endif
@loop2:
	in a,(c)
	rra
	jr c,@loop2
endm

macro GA_VSyncWaitON
	ld b,#f5	
@loop:
	in a,(c)
	rra			
	jr nc,@loop	
endm


macro GA_VSyncWaitOFF
	ld b,#f5
@loop:
	in a,(c)
	rra
	jr c,@loop
endm

;
;
;
;
macro GA_SetBorder Color
	ld bc,#7f10
	out (c),c
	ld c, {Color}
	out (c),c
endm


macro GA_DebugInterrupt Color
ifdef DebugInterrupt
	ld bc,#7f10
	out (c),c
	ld c, {Color}
	out (c),c
endif
endm


macro GA_DebugProcess Color
	ifdef DebugProcesses
		ld bc,#7f10
		out (c),c
		ld c, {Color}
		out (c),c
	endif
endm