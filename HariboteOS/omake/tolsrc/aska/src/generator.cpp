#include "generator.h"
#ifdef LINUX
#define ltoa(n,b,l) (sprintf(b, "%ld", n), b)
#define itoa(n,b,l) (sprintf(b, "%d", n), b)
#endif

Generator::Generator(void){
	scanner=NULL;
	lpLogFP=stderr;
}


void	Generator::Param2LPSTR(LPSTR buf, Parameter& param){
	char	buf2[256];
	bool	first;
	if(param.pdepth != 0 && param.ptype == TK_SEGMENT){
		param.ndisp += 4;
		param.bLabel = false;
	}
	strcpy(buf, "");
	if(param.paramtype != P_REG){
		if(param.pdepth != 0){
			switch(param.size){
			  case 2: strcat(buf, "word ptr ");  break;
			  case 4: strcat(buf, "dword ptr "); break;
			  case 6: strcat(buf, "pword ptr "); break;
			  default: 
				  Error("Internal Error: Invalid size");
				  return;
			}
		}else{
			switch(param.size){
			  case 0: break;
			  case 1: strcat(buf, "byte ptr "); break;
			  case 2: strcat(buf, "word ptr "); break;
			  case 4: strcat(buf, "dword ptr "); break;
			  default: 
				  Error("Internal Error: Invalid size");
				  return;
			}
		}
	}
	switch(param.paramtype){
	  case P_REG: strcat(buf, param.base->name); break;
	  case P_IMM:
		strcat(buf, param.disp.c_str());
		if(param.ndisp != 0){
			if(buf[0] != 0) strcat(buf, "+");
			strcat(buf, ltoa(param.ndisp, buf2, 10));
		}
		break;
	  case P_MEM:
		if(param.bLabel == true){
			strcat(buf, param.disp.c_str());
		}else{
			if(param.seg != NULL){
				strcat(buf, param.seg->name);
				strcat(buf, ":[");
			}else{
				strcat(buf, "[");
			}
			first = true;
			if(param.base != NULL){
				strcat(buf, param.base->name);
				first = false;
			}
			if(param.index != NULL){
				if(first == false) strcat(buf, "+");
				strcat(buf, param.index->name);
				if(param.scale != 1){
					strcat(buf, "*");
					strcat(buf, ltoa(param.scale, buf2, 10));
				}
				first = false;
			}
			if(param.disp != ""){
				if(first == false && param.disp.at(0) != '+' && param.disp.at(0) != '-') strcat(buf, "+");
				strcat(buf, param.disp.c_str());
				first = false;
			}
			if(param.ndisp != 0){
				if(first == false) strcat(buf, "+");
				strcat(buf, ltoa(param.ndisp, buf2, 10));
			}
			strcat(buf, "]");
		}
		break;
	  default:
		Error("Internal Error: Invalid parameter type");
		return;
	}
}

void	Generator::FlushStaticData(void){
	MaplpSegmentList::iterator		itseg;
	ListStaticDataList::iterator	itstatic;
	for(itseg = SegmentData.mapsegment.begin(); itseg != SegmentData.mapsegment.end(); itseg++){
		if(itseg->second->liststatic.empty()) continue;
		OutputMASM("", "", "", "");		// １行改行する
		OpenSegment(itseg->second);
		for(itstatic = itseg->second->liststatic.begin(); itstatic != itseg->second->liststatic.end(); itstatic++){
			RegistVariable(itstatic->name, itstatic->label, itstatic->lpInit);
		}
		CloseSegment(itseg->second);
	}
}

void	Generator::OpenSegment(SegmentList* segment){
	char buf[256];
	strcpy(buf, "");
	switch(segment->align){
	  case TK_BYTE:   strcat(buf, "BYTE ");   break;
	  case TK_WORD:   strcat(buf, "WORD ");   break;
	  case TK_DWORD:  strcat(buf, "DWORD ");  break;
	  case TK_PARA:   strcat(buf, "PARA ");   break;
	  case TK_PAGE:   strcat(buf, "PAGE ");   break;
	  case TK_PAGE4K: strcat(buf, "PAGE4K "); break;
	  default: 
		Error("Internal Error: Invalid segment alignment");
		return;
	}
	switch(segment->combine){
	  case TK_PRIVATE:                          break;	// privateは指定なし
	  case TK_PUBLIC:  strcat(buf, "PUBLIC ");  break;
	  case TK_STACK:   strcat(buf, "STACK ");   break;
	  case TK_COMMON:  strcat(buf, "COMMON ");  break;
	  default: 
		Error("Internal Error: Invalid combination with segment alignment");
		return;
	}
	switch(segment->use){
	  case TK_USE32: strcat(buf, "USE32 "); break;
	  case TK_USE16: strcat(buf, "USE16 "); break;
	  default: 
		Error("Internal Error: Invalid use of segment");
		return;
	}
	switch(segment->access){
	  case TK_RO: strcat(buf, "RO "); break;
	  case TK_EO: strcat(buf, "EO "); break;
	  case TK_ER: strcat(buf, "ER "); break;
	  case TK_RW: strcat(buf