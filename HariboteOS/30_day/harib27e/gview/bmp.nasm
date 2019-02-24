; BMP decode routine by I.Tak. 2003

section .text align=1
[bits 32]
;BMP File Structure (I can't understand MS.)

	struc BMP
		;FILE HEADER
.fType:		resw 1	;BM
.fSize:		resd 1	;whole file size
		resd 1	;reserved
.fOffBits:	resd 1	;offset from file top to image
		;INFO HEADER
.iSize:		resd 1	;INFO HEADER size
.iWidth:	resd 1	;Image Width in pixels
.iHeight:	resd 1	;Image Height in pixels
.iPlanes:	resw 1	;must be 1
.iBitCount:	resw 1	;BitPerPixel 1, 4, 8, 24 (and 15,16 for new OS/2 ?)
.iCompression:	resd 1	;Compress Type. 0 for none, then SizeImage=0
.iSizeImage:	resd 1	;Image Size(compressed)
.iXPPM:		resd 1	;X Pixel Per Meter
.iYPPM:		resd 1
.iClrUsed:	resd 1	;Number of used ColorQuad (0 for whole Quad)
.iClrImportant:	resd 1	;Number of Important ColorQuad.
	endstruc

	struc BMPOS2
		;FILE HEADER
.fType:		resw 1	;BM
.fSize:		resd 1	;whole file size
		resd 1	;reserved
.fOffBits:	resd 1	;offset from file top to image
		;CORE HEADER
.iSize:		resd 1	;CORE HEADER size
.iWidth:	resw 1	;Image Width in pixels
.iHeight:	resw 1	;Image Height in pixels
.iPlanes:	resw 1	;must be 1
.iBitCount:	resw 1	;BitPerPixel 1, 4, 8, 24 (and 15,16 for new OS/2 ?)
	endstruc

; B/W bmp can also have palettes. The first for 0, second for 1.

	struc CQuad
.b:	resb 1
.g:	resb 1
.r:	resb 1
	resb 1	;reserved
	endstruc


%if 0
int info_BMP(struct DLL_STRPICENV *env, int *info, int size, UCHAR *fp);
/* ﾀｮｸｷ､ｿ､