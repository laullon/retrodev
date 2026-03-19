

; Just a tiny example function in SDK


Keymap  ds 10  ;map with 10*8 = 80 key status bits (bit=0 key is pressed)

KeyboardScanner:
		ld bc,#f782     ;PPI port A out /C out 
		ld hl,Keymap    
		xor a
		out (c),c       
		ld bc,#f40e     ; Select Ay reg 14 on ppi port A 
		ld e,b          
		out (c),c       
		ld bc,#f6c0     ;This value is an AY index (R14) 
		ld d,b          
		out (c),c       
		out (c),a;c     ;Validate!! out (c),0 
		ld bc,#f792     ; PPI port A in/C out 
		out (c),c       
		ld a,#40        
		ld c,#4a        
.KeyboardScanner_Loop
		ld b,d   		;d=#f6      
		out (c),a       ;4 select line
		ld b,e    		;e=#f4   
		ini             ;5 read bits and write into KEYMAP
		inc a           
		cp c            
		jp c,.KeyboardScanner_Loop       ;2/3 9*16+1*15=159
		ld bc,#f782     
		out (c),c       ; PPI port A out / C out 
		ret