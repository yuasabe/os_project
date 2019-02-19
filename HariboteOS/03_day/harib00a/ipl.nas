; haribote-ipl
; TAB=4

		ORG		0x7c00
		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		
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
		DB		"HARIBOTEOS "	
		DB		"FAT12   "		
		RESB	18				



entry:
		MOV		AX,0			
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX


; 書き足された部分
		MOV		AX,0x0820
		MOV		ES,AX		; バッファアドレス
		MOV		CH,0		; シリンダ番号	
		MOV		DH,0		; ヘッド番号
		MOV		CL,2		; セクタ番号	

		MOV		AH,0x02 	; 読み込み
		MOV		AL,1		; 処理するセクタ数
		MOV		BX,0		; バッファアドレス
		MOV		DL,0x00		; ドライブ番号
		INT		0x13		; ディスク関係
		JC		error	; jump if carry, CFが1であればエラー



fin:
		HLT						
		JMP		fin				

error:
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1			
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			
		MOV		BX,15			
		INT		0x10			
		JMP		putloop
msg:
		DB		0x0a, 0x0a		
		DB		"load error"
		DB		0x0a			
		DB		0

		RESB	0x7dfe-$		

		DB		0x55, 0xaa
