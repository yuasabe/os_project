#include "scanner.h"

#ifdef WINVC
HRESULT	Scanner::ReadFile(string& filename){
#else
HRESULT	Scanner::ReadFile(string filename){
#endif

	ScannerSub* scannersubx = new ScannerSub;
	files.push(scannersubx);
	files.top()->SetLogFile(lpLogFP);
	return files.top()->ReadFile(filename);
}

Token	Scanner::GetToken(void){
	Token token;
	while(true){
		token = files.top()->GetToken();
		if(token == TK_INCLUDE){
			nIncludeNest++;
			files.top()->GetToken();	// includeの次の"ファイル名"を取り出す
			ReadFile(string(files.top()->GetLabel()));	// 括り文字付きで渡してる
			continue;
		}
//		if(token == TK_DEFINE){
//			files.top()->GetToken();	// includeの次の"ファイル名"を取り出す
//			continue;
//		}
		if(token == TK_EOF){
			if(nIncludeNest == 0) break;		// 本当に終わり
			nErrorCount += files.top()->GetErrorCount();
			DELETE_SAFE(files.top());
			files.pop();
			nIncludeNest--;
			continue;
		}
		break;
	}
	return token;
}

Token	Scanner::PeekToken(void){
	Token token;
	while(true){
		token = files.top()->PeekToken();
		if(token == TK_INCLUDE){
			nIncludeNest++;
			files.top()->GetToken();	// 次に進める
			files.top()->GetToken();	// includeの次の"ファイル名"を取り出す
			ReadFile(string(files.top()->GetLabel()));	// 括り文字付きで渡してる
			continue;
		}
		if(token == TK_EOF){
			if(nIncludeNest == 0) break;	// 本当に終わり
			nErrorCount += files.top()->GetErrorCount();
			DELETE_SAFE(files.top());
			files.pop();
			nIncludeNest--;
			continue;
		}
		break;
	}
	return token;
}

bool	ScannerSub::IsToken(LPSTR &lp, LPSTR lp2){
	bool	bReserved = false;		// はじめの文字が演算子でなければ予約語と見なす
	unsigned char c;
	LPSTR	lpx = lp;
	LPSTR	lpt = labelbuf;

#ifdef WINVC
	if(IS_CHARACTOR(*lpx)) bReserved = true;
#else
	if(IS_CHARACTOR((unsigned char)*lpx)) bReserved = true;
#endif

	while(true){
		*(lpt++) = *lp2;
		if(*lp2 == '¥0'){
			c = *lpx;
			if(bReserved == true && IS_CHARACTOR(c)) return false;
			lp = lpx;		//tokenの終わりの位置までポインタを進める
			return true;
		}
		if(*(lpx++) != *(lp2++)) return false;
	}
}

// lpPosから次のトークンまでコピー
void	ScannerSub::CopyLabel(LPSTR& lpPos){
	unsigned char c;
	LPSTR lp = labelbuf;
	while(true){
		c = *lpPos;
		if(IS_KANJI1(c)){
			*(lp++) = c;
			*(lp++) = c;	//ほんとはここにsjisの2バイト目かの判別を入れる
			lpPos++;
		}else if(IS_ALNUM(c) || c == '#' || c == '@' || c == '$'){
			*(lp++) = c;
		}else{
			break;
		}
		lpPos++;
	}
	*lp = '¥0';
}

// 数値ならばnumbufに。そうでなければ戻り値：非0
// 数字もCopyLabel()と同様の機能が追加され、GetLabel()できるようになった
HRESULT	ScannerSub::NumCheck(LPSTR& lpPos){
	int num;
	unsigned char c;
	LPSTR lp = labelbuf;

	if(*lpPos < '0' || *lpPos > '9') return 1;
	if((*lpPos == '0') && (*(lpPos+1) == 'x')){
		num = 16;
		lpPos+=2;
	}else if((*lpPos == '0') && (*(lpPos+1) == 'b')){
		num = 2;
		lpPos+=2;
	}else{
		num = 10;
	}

	numbuf = 0;
	bool	first = true;
	while(true){
		c = *lpPos;
		switch(num){
		  case 10:		// 10進法
			if((c >= '0') && (c <= '9')){
				numbuf *= num;
				numbuf += (LONG)(c-'0');
			}else{
				*lp = '¥0';
				return 0;
			}
			break;
		  case 16:		// 16進法
			if((c >= '0') && (c <= '9')){
				numbuf *= num;
				numbuf += (LONG)(c-'0');
			}else if((c >= 'A') && (c <= 'F')){
				if(first == true) *lp++ = '0';
				numbuf *= num;
				numbuf += (LONG)(c-'A') + 10;
			}else if((c>='a') && (c<='f')){
				if(first == true) *lp++ = '0';
				numbuf *= num;
				numbuf += (LONG)(c-'a') + 10;
			}else{
				*lp++ = 'H';
				*lp = '¥0';
				return 0;
			}
			break;
		  case 2:		// 2進法
			if(c == '0' || c == '1'){
				numbuf *= num;
				numbuf += (LONG)(c-'0');
			}else{
				return 0;
			}
			*lp++ = 'B';
			*lp = '¥0';
			break;
		}
		lpPos++;
		*(lp++) = c;
		first = false;
	}
}

void	ScannerSub::GetQuotedLabel(LPSTR &p){
	unsigned char c = *(p++);			// 'か"がcに入る
	LPSTR x = labelbuf;
	*(x++) = c;
	while(true){
		if(IS_KANJI1(*((unsig