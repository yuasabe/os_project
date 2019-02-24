/*
	データテーブルクラス　〜table.h + table.cpp〜
*/
#ifndef	__TABLE_H
#define	__TABLE_H

#pragma warning(disable:4786)

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <list>
#include <map>
#include <stack>

using namespace std;


#include "macro.h"
#include "module.h"
#include "scanner.h"

enum RegisterType{
	R_GENERAL,		// 汎用レジスタ
	R_SEGREG,		// セグメントレジスタ
	R_CTRL,			// コントロールレジスタ
	R_DEBUG,		// デバッグレジスタ
	R_TEST,			// テストレジスタ
	R_FLOAT,		// 浮動小数点レジスタ
	R_MMX,			// MMXレジスタ
};

class RegisterList{
  public:
	LPSTR			name;	// レジスタ名
	RegisterType	type;	// レジスタタイプ
	int				size;	// レジスタサイズ
	bool			bBase;	// ベースレジスタであるか
	bool			bIndex;	// インデックスレジスタであるか

	RegisterList(LPSTR n, RegisterType t, int s, bool b, bool i)
			{ name=n; type=t; size=s; bBase=b; bIndex=i; }
};

typedef map<string, RegisterList*> MaplpRegisterList;

class Register{
	MaplpRegisterList	mapregister;
	
  public:
	Register(void);
	‾Register();
	
	void			Add(LPSTR key, RegisterList* r){ string k=key; mapregister[k]=r; }
	RegisterList*	Find(LPSTR key){
		string k = key;
		MaplpRegisterList::iterator it = mapregister.find(k);
		if(it == mapregister.end()) return NULL; else return it->second;
	}
};

class LabelList;

class StaticDataList{
  public:
	LPSTR			name;
	LabelList*		label;
	LPSTR			lpInit;
};

typedef list<StaticDataList> ListStaticDataList;

class SegmentList{
  public:
	ListStaticDataList liststatic;

	LPSTR			name;			// セグメントネーム
	Token			align;			// アライン属性
	Token			combine;		// コンバイン属性
	Token			use;			// USE属性
	Token			access;			// アクセス属性
	LPSTR			segmentclass;	// セグメントクラス

	RegisterList*	assume;			// segment CODE == ES;のESを保存するために使う

	SegmentList(void)
		{ name=NULL; align=TK_BYTE; combine=TK_PRIVATE; use=TK_USE32;
			access=TK_ER; segmentclass=NULL; assume=NULL; }
	SegmentList(LPSTR n, Token al, Token co, Token us, Token ac, LPSTR cl)
		{ name=n; align=al; combine=co; use=us; access=ac; segmentclass=cl; assume=NULL; }

	void	AddStaticData(LPSTR varname, LabelList* label, LPSTR initialize);
};

typedef map<string, SegmentList*> MaplpSegmentList;

class Segment{
  public:
	MaplpSegmentList	mapsegment;
	
	Segment(void);
	‾Segment();
	
	void			Add(LPSTR key, SegmentList* s){ string k=key; mapsegment[k]=s; }
	SegmentList*	Find(LPSTR key){
		string k = key;
		MaplpSegmentList::iterator it = mapsegment.find(k);
		if(it == mapsegment.end()) return NULL; else return it->second;
	}
};

class TagList;

class MemberList{
  public:
	bool			bSigned;	// 符号あり型かどうか
	TagList*		type;		// 変数の型
	Token			ptype;		// near:TK_NEAR, far:TK_FAR
	int				pdepth;		// ポインタの深さ。これが0ならポインタでない
	bool			bArray;		// 配列であるか
	int				size;		// 変数のサイズ（配列、ポインタ、構造体も加味）
	int				offset;		// 構造体でのメンバのオフセット

	MemberList(void)
		{ type=NULL; bSigned=true; ptype=TK_FAR; pdepth=0; bArray=false; size=0; offset=0; }
	MemberList(bool bs, TagList* t, Token pt, int pd, bool ba, int s, int o)
		{bSigned=bs; type=t; ptype=pt; pdepth=pd; bArray=ba; size=s; offset=o; }
	MemberList(MemberList* m)
		{bSigned=m->bSigned; type=m->type; ptype=m->ptype; pdepth=m->pdepth;
			bArray=m->bArray; size=m->size; offset=m->offset; }
	MemberList&	operator=(MemberList& label);
};

typedef map<string, MemberList*> MaplpMemberList;

// typedefやはじめからある型は"type"というメンバを見る。typedefならmember->type!=NULL
class TagList{
  public:
	int				size;		// この型のサイズ
	bool			bStruct;	// 構造体であるか
	MaplpMemberList	mapmember;	// 構造体であればメンバのリスト
	
	TagList(bool bs){ bStruct=bs; size=0;}
	‾TagList();

	void			AddMemberList(LPSTR n, bool bs, TagList* t, Token pt, int pd, bool ba, int s);
	MemberList*		FindMemberList(LPSTR key){
		string k = key;
		MaplpMemberList::iterator it = mapmember.find(k);
		if(it == 