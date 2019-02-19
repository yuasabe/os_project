; haribote-ipl
; TAB=4

		ORG		0x7c00			; ‚±‚ÌƒvƒƒOƒ‰ƒ€‚ª‚Ç‚±‚É“Ç‚Ýž‚Ü‚ê‚é‚Ì‚©

; ˆÈ‰º‚Í•W€“I‚ÈFAT12ƒtƒH[ƒ}ƒbƒgƒtƒƒbƒs[ƒfƒBƒXƒN‚Ì‚½‚ß‚Ì‹Lq

		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		; u[gZN^Ì¼Oð©RÉ¢Äæ¢i8oCgj
		DW		512				; 1ZN^Ìå«³i512ÉµÈ¯êÎ¢¯È¢j
		DB		1				; NX^Ìå«³i1ZN^ÉµÈ¯êÎ¢¯È¢j
		DW		1				; FATªÇ±©çnÜé©iÊÍ1ZN^Ú©çÉ·éj
		DB		2				; FATÌÂi2ÉµÈ¯êÎ¢¯È¢j
		DW		224				; [gfBNgÌæÌå«³iÊÍ224GgÉ·éj
		DW		2880			; ±ÌhCuÌå«³i2880ZN^ÉµÈ¯êÎ¢¯È¢j
		DB		0xf0			; fBAÌ^Cvi0xf0ÉµÈ¯êÎ¢¯È¢j
		DW		9				; FATÌæÌ·³i9ZN^ÉµÈ¯êÎ¢¯È¢j
		DW		18				; 1gbNÉ¢­ÂÌZN^ª é©i18ÉµÈ¯êÎ¢¯È¢j
		DW		2				; wbhÌi2ÉµÈ¯êÎ¢¯È¢j
		DD		0				; p[eBVðgÁÄÈ¢ÌÅ±±ÍK¸0
		DD		2880			; ±ÌhCuå«³ðà¤êx­
		DB		0,0,0x29		; æ­í©çÈ¢¯Ç±ÌlÉµÄ¨­Æ¢¢çµ¢
		DD		0xffffffff		; ½Ôñ{[VAÔ
		DB		"HARIBOTEOS "	; fBXNÌ¼Oi11oCgj
		DB		"FAT12   "		; tH[}bgÌ¼Oi8oCgj
		RESB	18				; Æè ¦¸18oCg ¯Ä¨­

; vO{Ì

entry:
		MOV		AX,0			; WX^ú»
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

; fBXNðÇÞ

		MOV		AX,0x0820
		MOV		ES,AX
		MOV		CH,0			; V_0
		MOV		DH,0			; wbh0
		MOV		CL,2			; ZN^2

		MOV		AH,0x02			; AH=0x02 : fBXNÇÝÝ
		MOV		AL,1			; 1ZN^
		MOV		BX,0
		MOV		DL,0x00			; AhCu
		INT		0x13			; fBXNBIOSÄÑoµ
		JC		error

; ÇÝIíÁ½¯ÇÆè ¦¸âé±ÆÈ¢ÌÅQé

fin:
		HLT						; ½© éÜÅCPUðâ~³¹é
		JMP		fin				; ³À[v

error:
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; SIÉ1ð«·
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; ê¶\¦t@NV
		MOV		BX,15			; J[R[h
		INT		0x10			; rfIBIOSÄÑoµ
		JMP		putloop
msg:
		DB		0x0a, 0x0a		; üsð2Â
		DB		"load error"
		DB		0x0a			; üs
		DB		0

		RESB	0x7dfe-$		; 0x7dfeÜÅð0x00Åßé½ß

		DB		0x55, 0xaa
