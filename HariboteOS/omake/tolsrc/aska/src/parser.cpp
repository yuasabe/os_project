#include <string.h>
#include "parser.h"
#ifdef LINUX
#define ltoa(n,b,l) (sprintf(b, "%ld", n), b)
#define itoa(n,b,l) (sprintf(b, "%d", n), b)
#endif

Parser::Parser(void){
	lpLogFP=stderr;
	nErrorCount=0;
	LoopLabelPoint=0;
	StructAlignCount = 0;
	defaultlocal.paramtype = P_MEM;
	defaultlocal.seg = generator.FindRegisterList("SS");
	defaultlocal.base = generator.FindRegisterList("EBP");
	defaultsegment = defaultdatasegment =NULL;
}


// Parameter追加系の処理関数
HRESULT	Parser::Sizeof(Parameter& param){
	TagList*	tag;
	int			size;
	if(scanner.PeekToken() == TK_SIZEOF) scanner.GetToken();
	if(scanner.GetToken() != TK_LPR){
		Error("Missing ¥"(¥"");
		return 2;
	}
	switch(scanner.PeekToken()){
	  case TK_UNSIGNED: case TK_SIGNED: scanner.GetToken(); break;
	}
	scanner.GetToken();
//	if(scanner.PeekToken() == TK_STRUCT) scanner.GetToken(); // 「struct タグ名」を許可するための苦肉の策
	tag = generator.FindTagList(scanner.GetLabel());
	if(tag == NULL){
		Error("Invalid type");
		return 3;
	}
	size = tag->size;
	switch(scanner.PeekToken()){
	  case TK_NEAR:
		if(generator.seg->use == TK_USE32) size = 4; else size = 2;
		scanner.GetToken();						// 次へ進める
		if(scanner.GetToken() != TK_MUL){		// 今は*がいくつも続くのをかけない
			Error("Missing ¥"*¥" after near");
			return 4;
		}
		break;
	  case TK_FAR:
		if(generator.seg->use == TK_USE32) size = 6; else size = 4;
		scanner.GetToken();						// 次へ進める
		if(scanner.GetToken() != TK_MUL){		// 今は*がいくつも続くのをかけない
			Error("Missing ¥"*¥" after far");
			return 4;
		}
		break;
	  case TK_MUL:
		if(generator.seg->use == TK_USE32) size = 6; else size = 4;
		scanner.GetToken();
		break;
	}
	param.ndisp += size;
	if(scanner.GetToken() != TK_RPR){
		Error("Missing ¥")¥"");
		return 5;
	}
	return 0;
}

// Parameter追加系の処理関数
HRESULT	Parser::Address(Parameter& param){
	LabelList* label;
	Parameter	selparam;			// Selector()はparamが破壊されるから
	if(scanner.PeekToken() == TK_AND) scanner.GetToken();
	if(scanner.GetToken() != TK_LABEL){
		Error("Invalid value after ¥"&¥"");
		return 2;
	}
	label = generator.FindLabelList(scanner.GetLabel());
	if(label == NULL){
		Error("Invalid value after ¥"&¥"");
		return 2;
	}
	if(Selector(selparam, label) != 0) return 1;
	if(selparam.bLabel == false){
		Error("Cannot use  ¥"&¥" for register, array, and immediate operand");
		return 3;
	}
	selparam.disp = "offset " + selparam.disp;
	Param2Param(param, selparam);
	return 0;
}

// Parameter追加系の処理関数
// &&のチェック機構が甘い。default == のため。
HRESULT	Parser::LocalAddress(Parameter& param){
	LabelList*	label;
	Parameter	selparam;			// Selector()はparamが破壊されるから
	char		buf[256];
	if(scanner.PeekToken() == TK_DAND) scanner.GetToken();
	if(scanner.PeekToken() != TK_LABEL) return 0;
	scanner.GetToken();
	label = generator.FindLocalLabelList(scanner.GetLabel());
	if(label == NULL){
		Error("Invalid local value after ¥"&&¥"");
		return 3;
	}
	if(label->bAlias == false){
		Error("Cannot use ¥"&&¥" for address label");
		return 4;
	}
	if(Selector(selparam, label) != 0) return 1;
	if(label->bArray == true || label->alias.paramtype != P_MEM){
		Error("Cannot use  ¥"&¥" for register, array, and immediate operand");
		return 5;
	}
	param.disp += ltoa(selparam.ndisp + label->nLocalAddress, buf, 10);
	return 0;
}

// paramの中身は壊される
HRESULT	Parser::Selector(Parameter& param, LabelList* label){
	char	buf[256];
	string	dummy;
	strcpy(buf, "");
	param.bSigned=label->bSigned; param.type=label->type; param.ptype =label->ptype;
	param.pdepth =label->pdepth;  param.size=label->size; param.bArray=label->bArray;
	if(label->bAlias == true){
		param.ndisp = label->alias.ndisp; param.bLabel = false;
		param.seg   = label->alias.seg;   param.base = label->alias.base;
		param.index = label->alias.index; param.disp = label->alias.disp;
		param.paramtype = label->alias.paramtype;
	}else{
		param.paramtype = P_MEM;
		if(generator.FindLocalLabelList(scanner.GetLabel()) != NULL){
			if(label->bStatic == true){
				strcat(buf, "__");
				strcat(buf, lpFunctionName);
			}else{
				strcat(buf, "#");
			}
		}
		strcat(buf, scanner.GetLabel());
		if(param.bArray == true || param.type->bStruct == true){
			param.bLabel = false; param.disp = dummy + "offset " + buf;
		}else{
			param.bLabel = true;  param.disp = buf;
		}
	}
	if(Selector2(param) != 0) return 1;
	if(param.ndisp != 0){
		if(param.disp == ""){
			param.disp += ltoa(param.ndisp, buf, 10);
		}else{
			param.disp += dummy + "+" + ltoa(param.ndisp, buf, 10);
		}
		param.ndisp = 0;
	}
	return 0;
}

HRESULT	Parser::Selector2(Parameter& param){
	MemberList*	member;
	switch(scanner.PeekToken()){
	  case TK_LSQ:
		if(param.pdepth == 0 && param.bArray == false){
			Error("Only pointer value avalible before ¥"[¥"");
			return 2;
		}
		if(Array(param) != 0) return 1;
		if(Selector2(param) != 0) return 1;		//再帰をかける
		break;
	  case TK_DOT:
		if(param.type->bStruct == false){

#ifdef WINVC
			Error("Cannot use ¥".¥" not for structure");
#else
			Error("Cannot use ¥".¥" not for structure");
#endif

			return 3;
		}
		if(param.pdepth != 0){
			Error("Cannot use ¥".¥" for pointer");
			return 4;
		}
		scanner.GetToken();			// 次に進める
		scanner.GetToken();
		member = param.type->FindMemberList(scanner.GetLabel());
		if(member == NULL){
			Error("Invalid member after ¥".¥"");
			return 5;
		}
		Member2Param(param, member);
		if(Selector2(param) != 0) return 1;		//再帰をかける
		break;
	  case TK_MEMBER:
		if(param.type->bStruct == false){

#ifdef WINVC
			Error("Not for structure, cannot use ¥"->¥"");
#else
			Error("Not for structure, cannot use ¥"->¥"");
#endif

			return 6;
		}
		if(param.pdepth == 0){
			Error("Not for pointer, cannot use ¥"->¥"");
			return 7;
		}
		if(param.bLabel == true){
			Error("Can use ¥"->¥" only for alias");
			return 8;
		}
		scanner.GetToken();			// 次に進める
		scanner.GetToken();
		member = param.type->FindMemberList(scanner.GetLabel());
		if(member == NULL){
			Error("Invalid member after ¥"->¥"");
			return 9;
		}
		Member2Param(param, member);
		if(Selector2(param) != 0) return 1;		//再帰をかける
		break;
	}
	return 0;
}

void	Parser::Member2Param(Parameter& param, MemberList* member){
	param.bSigned  = member->bSigned;
	param.type     = member->type;
	param.ptype    = member->ptype;
	param.pdepth   = member->pdepth;
	param.bArray   = member->bArray;
	param.size     = member->size;
	param.ndisp   += member->offset;
}

HRESULT	Parser::Param2Param(Parameter& to, Parameter& from){
	switch(to.paramtype){
	  case P_MEM:
		if(to.seg != NULL && from.seg != NULL){
			Error("Two more items in Seg:Base");
			return 2;
		}else if(to.seg == NULL && from.seg != NULL){
			if(to.base != NULL){
				if(to.base->bIndex == false || to.index != NULL){
					Error("Multiple base registers not allowed");
					return 3;
				}
				to.index = to.base;
			}
			to.seg  = from.seg;
			to.base = from.base;
		}else if(to.seg == NULL && from.base != NULL){
			if(to.base != NULL){
				if(to.base->bIndex == false || to.index != NULL){
					Error("Multiple base registers not allowed");
					return 3;
				}
				to.index = to.base;
			}
			to.base = from.base;
		}
		if(to.scale != 1 && from.scale != 1){
			Error("Two more items in Index*Scale");
			return 4;
		}else if(to.scale == 1 && from.scale != 1){
			if(to.index != NULL){
				if(to.index->bBase == false || to.base != NULL){
					Error("Multiple index registers not allowed");
					return 5;
				}
				to.base = to.index;
			}
			to.index = from.index;
			to.scale = from.scale;
		}else if(to.scale == 1 && from.index != NULL){
			if(to.index != NULL){
				if(to.index->bBase == false || to.base != NULL){
					Error("Multiple index registers not allowed");
					return 5;
				}
				to.base = to.index;
			}
			to.index = from.index;
		}
		to.disp  += from.disp;
		to.ndisp += from.ndisp;
		break;
	  case P_ERR: to = from; break;
	  case P_REG:
		if(to.base != NULL){
			Error("Register already exists");
			return 6;
		}
		to.base = from.base;
		break;
	  case P_IMM:
		if(to.disp != ""){
			to.disp += "+" + from.disp;
		}else{
			to.disp += from.disp;
		}
		to.ndisp += from.ndisp;
		break;
	  default:
		Error("Internal error on parameter");
		return 7;
	}
	return 0;
}

// offsetキャストやsegmentキャストは指定できない
HRESULT	Parser::Array(Parameter& param){
	Token			ope = TK_PLUS;
	int				PRnest, SQnest;
	Parameter		selparam, castparam;			// Selector()はparamが破壊されるから
	SegmentList*	seg;
	RegisterList*	reg;
	LabelList*		label;
	switch(scanner.GetToken()){
	  case TK_LPR: PRnest=1; SQnest=100; break;	// おそらく関係ない方は0にならない
	  case TK_LSQ: SQnest=1; PRnest=100; break;	// おそらく関係ない方は0にならない
	  default: return 0;
	}
	param.paramtype = P_MEM;
	if(param.disp != "") param.disp += "+";
	while(true){
		switch(scanner.GetToken()){
		  case TK_LABEL:
			seg   = generator.FindSegmentList(scanner.GetLabel());
			if(seg != NULL){
				if(ope != TK_PLUS){
					Error("Cannot use operator except for ¥"+¥", only for immediate operand");
					return 2;
				}
				if(ArrayReg(param, seg->assume) != 0) return 1;	// ?
				break;
			}
			reg   = generator.FindRegisterList(scanner.GetLabel());
			if(reg != NULL){
				if(ope != TK_PLUS){
					Error("Cannot use operator except for ¥"+¥", only for immediate operand");
					return 2;
				}
				if(ArrayReg(param, reg) != 0) return 1;			// ?
				break;
			}
			label = generator.FindLabelList(scanner.GetLabel());
			if(label != NULL){
				if(Selector(selparam, label) != 0) return 1;	// ?
				if(selparam.bLabel == true){
					Error("Not allowed to be followed by value");
					return 3;
				}
				if(selparam.base  != NULL) PointerCheck(selparam.base);
				if(selparam.index != NULL) PointerCheck(selparam.index);
				Param2Param(param, selparam);
				if(castparam.type != NULL){
					selparam.type =castparam.type;  selparam.bSigned=castparam.bSigned;
					selparam.ptype=castparam.ptype; selparam.pdepth =castparam.pdepth;
				}
				if(param.pdepth != 0){
					if(param.type == NULL){
						param.type =selparam.type;  param.bSigned=selparam.bSigned;
						param.ptype=selparam.ptype; param.pdepth =selparam.pdepth;
					}else if(param.type != selparam.type || param.bSigned != selparam.bSigned
							|| param.ptype != selparam.ptype || param.pdepth != selparam.pdepth){
						Error("Cannot add different type pointer");
						return 4;
					}
				}
				castparam.type = NULL;
				break;
			}
			Error("Invalid register or value");
			return 5;
		  case TK_NUM:   param.disp+=scanner.GetLabel();       castparam.type=NULL; break;
		  case TK_SIZEOF:if(Sizeof(param) != 0)  return 1;     castparam.type=NULL; break;
		  case TK_AND:   if(Address(param) != 0) return 1;     castparam.type=NULL; break;
		  case TK_DAND:  if(LocalAddress(param) != 0) return 1;castparam.type=NULL; break;
		  case TK_PLUS:  param.disp+="+";                      castparam.type=NULL; continue;	// 単項演算子+
		  case TK_MINUS: param.disp+="-";     ope=TK_MINUS;    castparam.type=NULL; continue;	// 単項演算子-
		  case TK_CPL:   param.disp+=" NOT "; ope=TK_CPL;      castparam.type=NULL; continue;	// 単項演算子‾
		  case TK_LPR:
			if(scanner.PeekToken() >= TK_DWORD && scanner.PeekToken() <= TK_SEGMENT){
				if(Cast(castparam) != 0) return 1;		// ?
			}else{
				PRnest++; param.disp += "(";
			}
			continue;
		  default:
			Error("Operator not properly used nor correct operator");
			return 6;
		}
		ope = scanner.GetToken();
		switch(ope){
		  case TK_PLUS:  if(param.disp != "") param.disp+="+";     break;
		  case TK_MINUS: param.disp+="-";                          break;
		  case TK_MUL:   param.disp+="*";                          break;
		  case TK_DIV:   param.disp+="/";                          break;
		  case TK_REM:   param.disp+=" MOD ";                      break;
		  case TK_AND:   param.disp+=" AND ";                      break;
		  case TK_OR:    param.disp+=" OR ";                       break;
		  case TK_XOR:   param.disp+=" XOR ";                      break;
		  case TK_SHR:   param.disp+=" SHR ";                      break;
		  case TK_SHL:   param.disp+=" SHL ";                      break;
		  case TK_RPR:   if((--PRnest) != 0) param.disp += ")";    break;
		  case TK_RSQ:   SQnest--;                                 break;
		  default:
			Error("Operator not properly used nor correct operator");
			return 6;
		}
		if(PRnest == 0 || SQnest == 0) break;
	}
	if(param.type == NULL){
		param.size=0; param.bArray=false; param.pdepth=0;
	}else{
		param.size=param.type->size; param.bArray=false;
		if(param.pdepth > 0) param.pdepth--;	// 臨時。ホントはif節だけでよいはず
	}
	return 0;
}

void	Parser::PointerCheck(RegisterList* reg){
	if(reg == NULL) return;
	if(generator.seg == NULL) return;
}

HRESULT	Parser::ArrayReg(Parameter& param, RegisterList* reg){
	Parameter	param2;
	LabelList*	label;
	if(reg == NULL) return 0;
	switch(reg->type){
	  case R_SEGREG:
		param2.seg = reg;
		if(scanner.GetToken() != TK_COLON){
			Error("Segment register must be followed by ¥":¥"");
			return 2;
		}
		switch(scanner.GetToken()){
		  case TK_NUM:
			param2.disp = scanner.GetLabel();
			break;
		  case TK_LABEL:
			reg = generator.FindRegisterList(scanner.GetLabel());
			label = generator.FindLabelList(scanner.GetLabel());
			if(reg != NULL){
				PointerCheck(reg);
				if(reg->bBase == false){
					Error("Invalid Seg:Base");
					return 4;
				}
				param2.base = reg;
			}else if(label != NULL){
				if(label->bAlias == false){
					Error("Invalid alias");
					return 7;
				}
				switch(label->alias.paramtype){
				  case P_IMM:
					param2.disp = label->alias.disp;
					param2.ndisp = label->alias.ndisp;
				  case P_REG:
					PointerCheck(reg);
					if(reg->bBase == false){
						Error("Invalid Seg:Base");
						return 4;
					}
					param2.base = label->alias.base;
					break;
				  case P_MEM:
					Error("Memory alias not allowed to be specified");
					return 8;
				}
			}else{
				Error("Invalid register");
				return 3;
			}
			break;
		}
		break;
	  case R_GENERAL:
		reg = generator.FindRegisterList(scanner.GetLabel());
		if(reg == NULL){
			Error("Invalid register");
			return 3;
		}
		PointerCheck(reg);
		if(reg->bIndex == true){
			if(scanner.PeekToken() == TK_MUL){
				scanner.GetToken();					// 次へ進める
				if(scanner.GetToken() != TK_NUM){
					Error("Must be Index*{1,2,4,8}");
					return 5;
				}
				if(scanner.GetNum() != 1 && scanner.GetNum() != 2
						&& scanner.GetNum() != 4 && scanner.GetNum() != 8){
					Error("Must be Index*{1,2,4,8}");
					return 5;
				}
				param2.index = reg;
				param2.scale = scanner.GetNum();
			}else{
				if(param.base == NULL && reg->bBase == true) param2.base = reg;
				else param2.index = reg;
			}
			break;
		}
		if(reg->bBase == true){
			param2.base = reg;
		}
		Error("Invalid base register or index register");
		return 6;
	  default:
		Error("Invalid base, index, or segment register");
		return 6;
	}
	Param2Param(param, param2);
	return 0;
}

HRESULT	Parser::Pointer(Parameter& param){
	Parameter	selparam;				// Selector()はparamが破壊されるから
	LabelList*	label;
	if(scanner.PeekToken() == TK_MUL) scanner.GetToken();
	switch(scanner.PeekToken()){
	  case TK_LABEL:
		scanner.GetToken();				//次に進める
		label = generator.FindLabelList(scanner.GetLabel());
		if(label == NULL){
			Error("Value not defined");
			return 2;
		}
		if(Selector(selparam, label) != 0) return 1;
		if(selparam.pdepth == 0){
			Error("Invalid pointer");
			return 3;
		}
		if(selparam.bLabel == true){
			Error("Value not expected to be pointer");
			return 4;
		}
		Param2Param(param, selparam);
		param.bSigned=selparam.bSigned;    param.ptype =selparam.ptype;
		param.size   =selparam.type->size; param.bArray=false;
		param.pdepth--;
		break;
	  case TK_LPR:
		if(Array(param) != 0) return 1;
		break;
	}
	return 0;
}

HRESULT	Parser::GetParameter(Parameter& param){
	string			dummy;
	char			*buf, *initialize;
	SegmentList*	seg;
	LabelList*		stlabel;
	RegisterList*	reg;
	LabelList*		label;
	Parameter		selparam, castparam;
	if(scanner.PeekToken() == TK_LPR){
		scanner.GetToken();
		if(scanner.PeekToken() >= TK_DWORD && scanner.PeekToken() <= TK_SEGMENT){
			if(Cast(castparam) != 0) return 1;
		}else{
			Error("¥"(etc)¥" not allowed in unnecessary position");
			return 2;
		}
	}
	switch(scanner.PeekToken()){
	  case TK_MUL:
		if(Pointer(param) != 0) return 1;
		if(castparam.type != NULL){
			param.type   =castparam.type;    param.ptype =castparam.ptype;
			param.size   =castparam.size;    param.pdepth=castparam.pdepth;
			param.bSigned=castparam.bSigned;
		}
		break;
	  case TK_LSQ:
		if(Array(param) != 0) return 1;
		if(castparam.type != NULL){
			param.type   =castparam.type;    param.ptype =castparam.ptype;
			param.size   =castparam.size;    param.pdepth=castparam.pdepth;
			param.bSigned=castparam.bSigned;
		}
		break;
	  case TK_SIZEOF: case TK_AND: case TK_DAND: case TK_NUM: case TK_MINUS:
		if(Immedeate(param) != 0) return 1;
		break;
	  case TK_WQUOTE:
		scanner.GetToken();
		if(defaultdatasegment == NULL){
			Error("Missing default data segment");
		}
		buf = new char[256];
		sprintf(buf, "LL%04X", LocalLabelCounter);
		strcat(buf, "__");
		strcat(buf, lpFunctionName);
		LocalLabelCounter++;
		stlabel = new LabelList;
		stlabel->bArray = true;
		stlabel->pdepth = 1;
		stlabel->bStatic = true;
		stlabel->segment = defaultdatasegment;
		stlabel->size = strlen(scanner.GetLabel()) - 2 + 1;
		stlabel->type = generator.FindTagList("char");
		initialize = generator.ConstString(scanner.GetLabel());
		defaultdatasegment->AddStaticData(buf, stlabel, initialize);
		param.paramtype = P_MEM; param.bLabel = true; param.disp = dummy + "offset " + buf;
		param.pdepth = 1; param.type = stlabel->type; param.ptype = TK_NEAR;
		if(generator.seg->use == TK_USE32) param.size = 4; else param.size = 2;
		break;
	  case TK_LABEL:
		scanner.GetToken();
		seg = generator.FindSegmentList(scanner.GetLabel());
		if(seg != NULL){
			if(scanner.PeekToken() == TK_DCOLON){
				scanner.GetToken();
				if(scanner.GetToken() != TK_WQUOTE){
					Error("Must be Segment::¥"〜¥"");
					return 8;
				}
				buf = new char[256];
				sprintf(buf, "LL%04X", LocalLabelCounter);
				strcat(buf, "__");
				strcat(buf, lpFunctionName);
				LocalLabelCounter++;
				stlabel = new LabelList;
				stlabel->bArray = true;
				stlabel->pdepth = 1;
				stlabel->bStatic = true;
				stlabel->segment = seg;
				stlabel->size = strlen(scanner.GetLabel()) - 2 + 1;
				stlabel->type = generator.FindTagList("char");
				initialize = generator.ConstString(scanner.GetLabel());
				seg->AddStaticData(buf, stlabel, initialize);
				param.paramtype = P_MEM; param.bLabel = true; param.disp = dummy + "offset " + buf;
				param.pdepth = 1; param.type = stlabel->type; param.ptype = TK_NEAR;
				if(generator.seg->use == TK_USE32) param.size = 4; else param.size = 2;
				break;
			}else{
				reg = seg->assume;
				if(reg == NULL){
					Error("Followed by segment name independently");
					return 8;
				}
			}
		}
		reg = generator.FindRegisterList(scanner.GetLabel());
		if(reg != NULL){
			switch(reg->type){
			  case R_SEGREG:
				selparam.seg = reg;
				if(scanner.PeekToken() != TK_COLON){
					if(seg != NULL){
						scanner.GetToken();	// 「:」をとばす
						Error("Followed by segment name independently");
						return 8;
					}else{
						selparam.base=reg;	param.bLabel=false; param.paramtype=P_REG;
						param.pdepth =0;	param.size  =reg->size;
					}
				}else{
					scanner.GetToken();	// 「:」をとばす
					scanner.GetToken();
					reg = generator.FindRegisterList(scanner.GetLabel());
					if(reg == NULL){
						Error("Invalid register");
						return 4;
					}
					PointerCheck(reg);
					if(reg->bBase == false){
						Error("Must be Seg:Base");	// segreg:constは書けない
						return 5;
					}
					selparam.base=reg; param.bLabel=false;  param.paramtype=P_MEM;
					param.pdepth =1;   param.ptype =TK_FAR; param.size     =reg->size + 2;
					param.bSigned=true;
					if(castparam.type != NULL){
						param.type   =castparam.type;    param.ptype =castparam.ptype;
						param.size   =castparam.size;    param.pdepth=castparam.pdepth;
						param.bSigned=castparam.bSigned;
					}else if(castparam.ptype == TK_OFFSET || castparam.ptype == TK_SEGMENT){
						if(param.ptype == TK_NEAR){
							Error("Not for far pointer, segment and offset casted");
							return 6;
						}
						param.ptype=castparam.ptype; param.size=castparam.size;
					}
				}
				break;
			  default:
				selparam.base=reg; param.bLabel=false;	param.paramtype=P_REG;
				param.pdepth =0;	param.size  =reg->size;
				if(castparam.type != NULL){
					param.type   =castparam.type;    param.ptype =castparam.ptype;
					param.size   =castparam.size;    param.pdepth=castparam.pdepth;
					param.bSigned=castparam.bSigned;
				}else if(castparam.ptype == TK_OFFSET || castparam.ptype == TK_SEGMENT){
					if(param.ptype == TK_NEAR){
						Error("Not for far pointer, segment and offset casted");
						return 6;
					}
					param.ptype=castparam.ptype; param.size=castparam.size;
				}
				break;
			}
			Param2Param(param, selparam);
			break;
		}
		label = generator.FindLabelList(scanner.GetLabel());
		if(label != NULL){
			if(Selector(selparam, label) != 0) return 1;
			Param2Param(param, selparam);
			param.bLabel   =selparam.bLabel;    param.bSigned=selparam.bSigned;
			param.paramtype=selparam.paramtype; param.pdepth =selparam.pdepth;
			param.ptype    =selparam.ptype;     param.size   =selparam.size;
			param.bArray   =selparam.bArray;    param.type   =selparam.type;
			if(castparam.type != NULL){
				param.type   =castparam.type;    param.ptype =castparam.ptype;
				param.size   =castparam.size;    param.pdepth=castparam.pdepth;
				param.bSigned=castparam.bSigned;
			}else if(castparam.ptype == TK_OFFSET || castparam.ptype == TK_SEGMENT){
				if(param.ptype == TK_NEAR){
					Error("Not for far pointer, segment and offset casted");
					return 6;
				}
				param.ptype=castparam.ptype; param.size=castparam.size;
				param.pdepth=castparam.pdepth; param.paramtype=castparam.paramtype; // Castにより追加
				//! offsetが数値のエイリアスを動作させるためのあやしい追加
				if(param.base == NULL) param.paramtype = P_IMM;
			}
			break;
		}
		// 変数でもレジスタでもない場合はアドレスラベルと見なす
		param.type =generator.FindTagList("dword");
		param.ptype=TK_NEAR; param.pdepth=1; param.size=4; param.bLabel=true;
		param.disp =scanner.GetLabel(); param.paramtype=P_MEM;
		break;
	  default:
		Error("Invalid register, value, or number");
		return 7;
	}
	return 0;
}

// rep, true, falseなどのコマンドキャストはどうやって渡すのか？
// (の次のトークンから渡す
HRESULT	Parser::Cast(Parameter& param){
	TagList*	tag;
	Token		token;
	switch(scanner.PeekToken()){
	  case TK_SIGNED:   param.bSigned=true;  scanner.GetToken(); break;
	  case TK_UNSIGNED: param.bSigned=false; scanner.GetToken(); break;
	}
	switch(scanner.PeekToken()){
	  case TK_OFFSET: 
		scanner.GetToken(); param.ptype=TK_OFFSET;
		if(generator.seg->use == TK_USE32) param.size=4; else param.size=2;
//		if(param.seg != NULL){			// あやしい追加分
			param.paramtype = P_REG;
			param.pdepth = 0;
//		}
		if(scanner.GetToken() != TK_RPR){
			Error("Missing ¥")¥"");
			return 4;
		}
		return 0;
	  case TK_SEGMENT:
		scanner.GetToken(); param.ptype=TK_SEGMENT; param.size=2;
//		if(param.seg != NULL){			// あやしい追加分
			param.paramtype = P_REG;
			param.base = param.seg;
			param.pdepth = 0;
//		}
		if(scanner.GetToken() != TK_RPR){
			Error("Missing ¥")¥"");
			return 4;
		}
		return 0;
	}
	scanner.GetToken();
//	if(scanner.PeekToken() == TK_STRUCT) scanner.GetToken(); // 「struct タグ名」を許可するための苦肉の策
	tag = generator.FindTagList(scanner.GetLabel());
	if(tag == NULL){
		Error("Invalid type for cast or invalid type");
		return 3;
	}
	param.type = tag;
	param.size = param.type->size;
	token = scanner.GetToken();
	switch(token){
	  case TK_NEAR: case TK_FAR:
		if(param.ptype != TK_OFFSET && param.ptype !=TK_SEGMENT) param.ptype = token;
		if(scanner.GetToken() != TK_MUL){
			Error("Cannot find ¥"*¥" after near, far keywords");
			return 2;
		}
		// 本当はここに*がいくつ続いてもいいようにする
		param.pdepth = 1;
		if(param.ptype == TK_NEAR){
			if(generator.seg->use == TK_USE32) param.size = 4; else param.size = 2;
		}else{
			if(generator.seg->use == TK_USE32) param.size = 6; else param.size = 4;
		}
		break;
	  case TK_MUL:
		// 本当はここに*がいくつ続いてもいいようにする
		param.pdepth = 1;
		if(param.ptype == TK_NEAR){
			if(generator.seg->use == TK_USE32) param.size = 4; else param.size = 2;
		}else{
			if(generator.seg->use == TK_USE32) param.size = 6; else param.size = 4;
		}
		break;
	  case TK_RPR:
		// 臨時デバッグ。ここは(int)等の時に通る
		return 0;
	}
	if(scanner.GetToken() != TK_RPR){
		Error("Missing ¥")¥"");
		return 4;
	}
	return 0;
}

HRESULT	Parser::Immedeate(Parameter& param){
	bool			bFlag = false;
	int				PRnest = 0;
	LabelList*		label;
	
	param.paramtype=P_IMM; param.pdepth=0;     param.type  =NULL;
	param.bSigned  =false; param.bLabel=false; param.bArray=false;
	
	while(bFlag == false){
		switch(scanner.GetToken()){
		  case TK_AND:   if(Address(param) != 0) return 1;      break;
		  case TK_DAND:  if(LocalAddress(param) != 0) return 1; break;
		  case TK_SIZEOF:if(Sizeof(param) != 0) return 1;       break;
		  case TK_NUM:   param.disp+=scanner.GetLabel();        break;
		  case TK_PLUS:  param.disp+="+";                       continue;	// 単項演算子+
		  case TK_MINUS: param.disp+="-";                       continue;	// 単項演算子-
		  case TK_CPL:   param.disp+=" NOT ";                   continue;	// 単項演算子‾
		  case TK_LABEL:
			label = generator.FindLabelList(scanner.GetLabel());
			if(label == NULL || label->alias.paramtype != P_IMM) return 1;
			param.disp += label->alias.disp;
			param.ndisp += label->alias.ndisp;
			break;
		  case TK_LPR:
			if(scanner.PeekToken() >= TK_DWORD && scanner.PeekToken() <= TK_SEGMENT){
				if(Cast(param) != 0) return 1;
			}else{
				PRnest++; param.disp += "(";
			}
			continue;
		  default:
			Error("Operator not properly used nor correct operator");
			return 2;
		}
		switch(scanner.PeekToken()){
		  case TK_PLUS:  if(param.disp != "") param.disp+="+"; scanner.GetToken(); break;
		  case TK_MINUS: param.disp+="-";                      scanner.GetToken(); break;
		  case TK_MUL:   param.disp+="*";                      scanner.GetToken(); break;
		  case TK_DIV:   param.disp+="/";                      scanner.GetToken(); break;
		  case TK_REM:   param.disp+=" MOD ";                  scanner.GetToken(); break;
		  case TK_AND:   param.disp+=" AND ";                  scanner.GetToken(); break;
		  case TK_OR:    param.disp+=" OR ";                   scanner.GetToken(); break;
		  case TK_XOR:   param.disp+=" XOR ";                  scanner.GetToken(); break;
		  case TK_SHR:   param.disp+=" SHR ";                  scanner.GetToken(); break;
		  case TK_SHL:   param.disp+=" SHL ";                  scanner.GetToken(); break;
		  case TK_RPR:
			  if((--PRnest) < 0){
				  bFlag = true;
			  }else{
				  param.disp+=")"; scanner.GetToken();
			  }
			  break;
		  default: bFlag=true; break;
		}
	}
	return 0;
}

void	Parser::Expression(void){
	Parameter	param1, param2;
	Token		command, tmp;
	if(GetParameter(param1) != 0) return;		// skipline
	command = scanner.GetToken();
	tmp = scanner.PeekToken();
	if(tmp != TK_DLM && tmp != TK_RPR) if(GetParameter(param2) != 0) return;	// skipline
	switch(command){
	  case TK_BECOME:
		generator.Op2(command, param1, param2);
		break;
	  case TK_ADD:  case TK_SUB: case TK_MULA: case TK_DIVA:
	  case TK_ANDA: case TK_ORA: case TK_XORA: case TK_SHLA: cas