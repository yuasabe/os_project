#include "table.h"

Register::Register(){
	RegisterList*	rl;
	rl = new RegisterList("AL" , R_GENERAL, 1, false, false); Add("AL" , rl);
	rl = new RegisterList("AH" , R_GENERAL, 1, false, false); Add("AH" , rl);
	rl = new RegisterList("AX" , R_GENERAL, 2, false, false); Add("AX" , rl);
	rl = new RegisterList("EAX", R_GENERAL, 4, true , true ); Add("EAX", rl);
	rl = new RegisterList("BL" , R_GENERAL, 1, false, false); Add("BL" , rl);
	rl = new RegisterList("BH" , R_GENERAL, 1, false, false); Add("BH" , rl);
	rl = new RegisterList("BX" , R_GENERAL, 2, true , false); Add("BX" , rl);
	rl = new RegisterList("EBX", R_GENERAL, 4, true , true ); Add("EBX", rl);
	rl = new RegisterList("CL" , R_GENERAL, 1, false, false); Add("CL" , rl);
	rl = new RegisterList("CH" , R_GENERAL, 1, false, false); Add("CH" , rl);
	rl = new RegisterList("CX" , R_GENERAL, 2, false, false); Add("CX" , rl);
	rl = new RegisterList("ECX", R_GENERAL, 4, true , true ); Add("ECX", rl);
	rl = new RegisterList("DL" , R_GENERAL, 1, false, false); Add("DL" , rl);
	rl = new RegisterList("DH" , R_GENERAL, 1, false, false); Add("DH" , rl);
	rl = new RegisterList("DX" , R_GENERAL, 2, false, false); Add("DX" , rl);
	rl = new RegisterList("EDX", R_GENERAL, 4, true , true ); Add("EDX", rl);
	rl = new RegisterList("DI" , R_GENERAL, 2, true , true ); Add("DI" , rl);
	rl = new RegisterList("EDI", R_GENERAL, 4, true , true ); Add("EDI", rl);
	rl = new RegisterList("SI" , R_GENERAL, 2, true , true ); Add("SI" , rl);
	rl = new RegisterList("ESI", R_GENERAL, 4, true , true ); Add("ESI", rl);
	rl = new RegisterList("BP" , R_GENERAL, 2, true , false); Add("BP" , rl);
	rl = new RegisterList("EBP", R_GENERAL, 4, true , true ); Add("EBP", rl);
	rl = new RegisterList("SP" , R_GENERAL, 2, false, false); Add("SP" , rl);
	rl = new RegisterList("ESP", R_GENERAL, 4, true , false); Add("ESP", rl);
	rl = new RegisterList("CS" , R_SEGREG , 2, false, false); Add("CS" , rl);
	rl = new RegisterList("DS" , R_SEGREG , 2, false, false); Add("DS" , rl);
	rl = new RegisterList("ES" , R_SEGREG , 2, false, false); Add("ES" , rl);
	rl = new RegisterList("FS" , R_SEGREG , 2, false, false); Add("FS" , rl);
	rl = new RegisterList("GS" , R_SEGREG , 2, false, false); Add("GS" , rl);
	rl = new RegisterList("SS" , R_SEGREG , 2, false, false); Add("SS" , rl);
	rl = new RegisterList("CR0", R_CTRL   , 4, false, false); Add("CR0", rl);
	rl = new RegisterList("CR2", R_CTRL   , 4, false, false); Add("CR2", rl);
	rl = new RegisterList("CR3", R_CTRL   , 4, false, false); Add("CR3", rl);
	rl = new RegisterList("DR0", R_DEBUG  , 4, false, false); Add("DR0", rl);
	rl = new RegisterList("DR1", R_DEBUG  , 4, false, false); Add("DR1", rl);
	rl = new RegisterList("DR2", R_DEBUG  , 4, false, false); Add("DR2", rl);
	rl = new RegisterList("DR3", R_DEBUG  , 4, false, false); Add("DR3", rl);
	rl = new RegisterList("DR6", R_DEBUG  , 4, false, false); Add("DR6", rl);
	rl = new RegisterList("DR7", R_DEBUG  , 4, false, false); Add("DR7", rl);
	rl = new RegisterList("TR3", R_TEST   , 4, false, false); Add("TR3", rl);
	rl = new RegisterList("TR4", R_TEST   , 4, false, false); Add("TR4", rl);
	rl = new RegisterList("TR5", R_TEST   , 4, false, false); Add("TR5", rl);
	rl = new RegisterList("TR6", R_TEST   , 4, false, false); Add("TR6", rl);
	rl = new RegisterList("TR7", R_TEST   , 4, false, false); Add("TR7", rl);
	rl = new RegisterList("MM0", R_MMX    , 8, false, false); Add("MM0", rl);
	rl = new RegisterList("MM1", R_MMX    , 8, false, false); Add("MM1", rl);
	rl = new RegisterList("MM2", R_MMX    , 8, false, false); Add("MM2", rl);
	rl = new RegisterList("MM3", R_MMX    , 8, false, false); Add("MM3", rl);
	rl = new RegisterList("MM4", R_MMX    , 8, false, false); Add("MM4", rl);
	rl = new RegisterList("MM5", R_MMX    , 8, false, false); Add("MM5", rl);
	rl = new RegisterList("MM6", R_MMX    , 8, false, false); Add("MM6", rl);
	rl = new RegisterList("MM7", R_MMX    , 8, false, false); Add("MM7", rl);
}

Register::‾Register(){
	MaplpRegisterList::iterator it;
	for(it = mapreg