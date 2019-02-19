; hello-os
; TAB=4

		ORG		0x7b00	; このプログラムがどこに読み込まれるか、program origin

; 以下は標準的なFAT12フォーマットフロッピディスクのための記述
		JMP		entry
		DB		0x90
		DB		"HELLOIPL"		
		DW		512				
		DB		1				
		DW		1				
		DB		2				
		DW		224				
		DW		2880			
		DB		0xf0			
		DW		9				
		DW		18				
		DW		2				
		DD		0				
		DD		2880			
		DB		0,0,0x29		
		DD		0xffffffff		
		DB		"HELLO-OS   "	
		DB		"FAT12   "		
		RESB	18				



; プログラム本体

entry:
		MOV		AX,0			; レジスタ初期化、accumulator register
		MOV		SS,AX			; セグメントレジスタ stack segment
		MOV		SP,0x7c00		; stack pointer
		MOV		DS,AX			; data segment
		MOV		ES,AX			; extra segment

		MOV		SI,msg			; source index, 読み込みインデックス
putloop:
		MOV		AL,[SI]			; lower 8 bits of AX
		ADD		SI,1			; SIに1を足す
		CMP		AL,0			
		JE		fin
		MOV		AH,0x0e			; 1文字表示function 
		MOV		BX,15			; color code
		INT		0x10			; ビデオBIOS呼び出し
		JMP		putloop
fin:
		HLT						; CPUを停止
		JMP		fin				; 無限ループ

msg:
		DB		0x0a, 0x0a		; 改行２つ
		DB		"hello, world"
		DB		0x0a			; 改行
		DB		0

		RESB	0x7dfe-$		

		DB		0x55, 0xaa



		DB		0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
		RESB	4600
		DB		0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
		RESB	1469432
