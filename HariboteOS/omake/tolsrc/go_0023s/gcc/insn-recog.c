/* Generated automatically by the program `genrecog' from the target
   machine description file.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tm_p.h"
#include "function.h"
#include "insn-config.h"
#include "recog.h"
#include "real.h"
#include "output.h"
#include "flags.h"
#include "hard-reg-set.h"
#include "resource.h"
#include "toplev.h"
#include "reload.h"



/* `recog' contains a decision tree that recognizes whether the rtx
   X0 is a valid instruction.

   recog returns -1 if the rtx is not valid.  If the rtx is valid, recog
   returns a nonnegative number which is the insn code number for the
   pattern that matched.  This is the same as the order in the machine
   description of the entry that matched.  This number can be used as an
   index into `insn_data' and other tables.

   The third argument to recog is an optional pointer to an int.  If
   present, recog will accept a pattern if it matches except for missing
   CLOBBER expressions at the end.  In that case, the value pointed to by
   the optional pointer will be set to the number of CLOBBERs that need
   to be added (it should be initialized to zero by the caller).  If it
   is set nonzero, the caller should allocate a PARALLEL of the
   appropriate size, copy the initial entries, and call add_clobbers
   (found in insn-emit.c) to fill in the CLOBBERs.


   The function split_insns returns 0 if the rtl could not
   be split or the split rtl in a SEQUENCE if it can be.

   The function peephole2_insns returns 0 if the rtl could not
   be matched. If there was a match, the new rtl is returned in a SEQUENCE,
   and LAST_INSN will point to the last recognized insn in the old sequence.
*/


extern rtx gen_split_845 PARAMS ((rtx *));
extern rtx gen_peephole2_853 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_854 PARAMS ((rtx, rtx *));
extern rtx gen_split_855 PARAMS ((rtx *));
extern rtx gen_split_856 PARAMS ((rtx *));
extern rtx gen_split_857 PARAMS ((rtx *));
extern rtx gen_peephole2_858 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_859 PARAMS ((rtx, rtx *));
extern rtx gen_split_860 PARAMS ((rtx *));
extern rtx gen_split_862 PARAMS ((rtx *));
extern rtx gen_split_863 PARAMS ((rtx *));
extern rtx gen_split_864 PARAMS ((rtx *));
extern rtx gen_split_866 PARAMS ((rtx *));
extern rtx gen_split_867 PARAMS ((rtx *));
extern rtx gen_split_868 PARAMS ((rtx *));
extern rtx gen_split_869 PARAMS ((rtx *));
extern rtx gen_split_872 PARAMS ((rtx *));
extern rtx gen_split_873 PARAMS ((rtx *));
extern rtx gen_split_874 PARAMS ((rtx *));
extern rtx gen_split_875 PARAMS ((rtx *));
extern rtx gen_split_876 PARAMS ((rtx *));
extern rtx gen_split_877 PARAMS ((rtx *));
extern rtx gen_split_879 PARAMS ((rtx *));
extern rtx gen_split_881 PARAMS ((rtx *));
extern rtx gen_split_882 PARAMS ((rtx *));
extern rtx gen_split_883 PARAMS ((rtx *));
extern rtx gen_split_885 PARAMS ((rtx *));
extern rtx gen_split_886 PARAMS ((rtx *));
extern rtx gen_split_887 PARAMS ((rtx *));
extern rtx gen_split_889 PARAMS ((rtx *));
extern rtx gen_split_890 PARAMS ((rtx *));
extern rtx gen_split_891 PARAMS ((rtx *));
extern rtx gen_split_893 PARAMS ((rtx *));
extern rtx gen_split_894 PARAMS ((rtx *));
extern rtx gen_split_895 PARAMS ((rtx *));
extern rtx gen_split_896 PARAMS ((rtx *));
extern rtx gen_split_897 PARAMS ((rtx *));
extern rtx gen_split_898 PARAMS ((rtx *));
extern rtx gen_split_899 PARAMS ((rtx *));
extern rtx gen_split_900 PARAMS ((rtx *));
extern rtx gen_split_901 PARAMS ((rtx *));
extern rtx gen_split_902 PARAMS ((rtx *));
extern rtx gen_split_903 PARAMS ((rtx *));
extern rtx gen_split_910 PARAMS ((rtx *));
extern rtx gen_split_911 PARAMS ((rtx *));
extern rtx gen_split_912 PARAMS ((rtx *));
extern rtx gen_split_914 PARAMS ((rtx *));
extern rtx gen_split_915 PARAMS ((rtx *));
extern rtx gen_split_917 PARAMS ((rtx *));
extern rtx gen_split_918 PARAMS ((rtx *));
extern rtx gen_split_920 PARAMS ((rtx *));
extern rtx gen_split_921 PARAMS ((rtx *));
extern rtx gen_split_923 PARAMS ((rtx *));
extern rtx gen_split_924 PARAMS ((rtx *));
extern rtx gen_split_929 PARAMS ((rtx *));
extern rtx gen_split_930 PARAMS ((rtx *));
extern rtx gen_split_931 PARAMS ((rtx *));
extern rtx gen_split_936 PARAMS ((rtx *));
extern rtx gen_split_937 PARAMS ((rtx *));
extern rtx gen_split_938 PARAMS ((rtx *));
extern rtx gen_split_943 PARAMS ((rtx *));
extern rtx gen_split_944 PARAMS ((rtx *));
extern rtx gen_split_945 PARAMS ((rtx *));
extern rtx gen_split_950 PARAMS ((rtx *));
extern rtx gen_split_952 PARAMS ((rtx *));
extern rtx gen_split_954 PARAMS ((rtx *));
extern rtx gen_split_955 PARAMS ((rtx *));
extern rtx gen_split_956 PARAMS ((rtx *));
extern rtx gen_split_957 PARAMS ((rtx *));
extern rtx gen_split_958 PARAMS ((rtx *));
extern rtx gen_split_959 PARAMS ((rtx *));
extern rtx gen_split_960 PARAMS ((rtx *));
extern rtx gen_split_961 PARAMS ((rtx *));
extern rtx gen_split_962 PARAMS ((rtx *));
extern rtx gen_split_970 PARAMS ((rtx *));
extern rtx gen_split_1001 PARAMS ((rtx *));
extern rtx gen_split_1003 PARAMS ((rtx *));
extern rtx gen_split_1004 PARAMS ((rtx *));
extern rtx gen_split_1005 PARAMS ((rtx *));
extern rtx gen_split_1010 PARAMS ((rtx *));
extern rtx gen_split_1013 PARAMS ((rtx *));
extern rtx gen_split_1014 PARAMS ((rtx *));
extern rtx gen_split_1015 PARAMS ((rtx *));
extern rtx gen_split_1028 PARAMS ((rtx *));
extern rtx gen_split_1033 PARAMS ((rtx *));
extern rtx gen_split_1034 PARAMS ((rtx *));
extern rtx gen_split_1035 PARAMS ((rtx *));
extern rtx gen_split_1036 PARAMS ((rtx *));
extern rtx gen_split_1037 PARAMS ((rtx *));
extern rtx gen_split_1038 PARAMS ((rtx *));
extern rtx gen_split_1040 PARAMS ((rtx *));
extern rtx gen_split_1041 PARAMS ((rtx *));
extern rtx gen_split_1042 PARAMS ((rtx *));
extern rtx gen_split_1043 PARAMS ((rtx *));
extern rtx gen_split_1044 PARAMS ((rtx *));
extern rtx gen_split_1045 PARAMS ((rtx *));
extern rtx gen_split_1048 PARAMS ((rtx *));
extern rtx gen_split_1049 PARAMS ((rtx *));
extern rtx gen_split_1050 PARAMS ((rtx *));
extern rtx gen_split_1051 PARAMS ((rtx *));
extern rtx gen_split_1053 PARAMS ((rtx *));
extern rtx gen_split_1054 PARAMS ((rtx *));
extern rtx gen_split_1055 PARAMS ((rtx *));
extern rtx gen_split_1056 PARAMS ((rtx *));
extern rtx gen_split_1057 PARAMS ((rtx *));
extern rtx gen_split_1058 PARAMS ((rtx *));
extern rtx gen_split_1060 PARAMS ((rtx *));
extern rtx gen_split_1061 PARAMS ((rtx *));
extern rtx gen_split_1062 PARAMS ((rtx *));
extern rtx gen_split_1063 PARAMS ((rtx *));
extern rtx gen_split_1064 PARAMS ((rtx *));
extern rtx gen_split_1067 PARAMS ((rtx *));
extern rtx gen_split_1068 PARAMS ((rtx *));
extern rtx gen_split_1069 PARAMS ((rtx *));
extern rtx gen_split_1070 PARAMS ((rtx *));
extern rtx gen_split_1072 PARAMS ((rtx *));
extern rtx gen_split_1074 PARAMS ((rtx *));
extern rtx gen_split_1075 PARAMS ((rtx *));
extern rtx gen_split_1077 PARAMS ((rtx *));
extern rtx gen_split_1079 PARAMS ((rtx *));
extern rtx gen_split_1081 PARAMS ((rtx *));
extern rtx gen_split_1082 PARAMS ((rtx *));
extern rtx gen_split_1083 PARAMS ((rtx *));
extern rtx gen_split_1087 PARAMS ((rtx *));
extern rtx gen_split_1088 PARAMS ((rtx *));
extern rtx gen_split_1092 PARAMS ((rtx *));
extern rtx gen_split_1093 PARAMS ((rtx *));
extern rtx gen_split_1099 PARAMS ((rtx *));
extern rtx gen_split_1100 PARAMS ((rtx *));
extern rtx gen_split_1133 PARAMS ((rtx *));
extern rtx gen_split_1134 PARAMS ((rtx *));
extern rtx gen_split_1135 PARAMS ((rtx *));
extern rtx gen_split_1136 PARAMS ((rtx *));
extern rtx gen_split_1155 PARAMS ((rtx *));
extern rtx gen_split_1156 PARAMS ((rtx *));
extern rtx gen_split_1157 PARAMS ((rtx *));
extern rtx gen_split_1158 PARAMS ((rtx *));
extern rtx gen_split_1162 PARAMS ((rtx *));
extern rtx gen_split_1163 PARAMS ((rtx *));
extern rtx gen_peephole2_1164 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1165 PARAMS ((rtx, rtx *));
extern rtx gen_split_1178 PARAMS ((rtx *));
extern rtx gen_split_1179 PARAMS ((rtx *));
extern rtx gen_split_1181 PARAMS ((rtx *));
extern rtx gen_split_1182 PARAMS ((rtx *));
extern rtx gen_peephole2_1207 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1208 PARAMS ((rtx, rtx *));
extern rtx gen_split_1214 PARAMS ((rtx *));
extern rtx gen_split_1218 PARAMS ((rtx *));
extern rtx gen_split_1219 PARAMS ((rtx *));
extern rtx gen_split_1221 PARAMS ((rtx *));
extern rtx gen_split_1222 PARAMS ((rtx *));
extern rtx gen_split_1224 PARAMS ((rtx *));
extern rtx gen_split_1225 PARAMS ((rtx *));
extern rtx gen_split_1227 PARAMS ((rtx *));
extern rtx gen_split_1228 PARAMS ((rtx *));
extern rtx gen_split_1230 PARAMS ((rtx *));
extern rtx gen_split_1231 PARAMS ((rtx *));
extern rtx gen_split_1232 PARAMS ((rtx *));
extern rtx gen_split_1236 PARAMS ((rtx *));
extern rtx gen_split_1237 PARAMS ((rtx *));
extern rtx gen_split_1238 PARAMS ((rtx *));
extern rtx gen_split_1239 PARAMS ((rtx *));
extern rtx gen_split_1240 PARAMS ((rtx *));
extern rtx gen_split_1241 PARAMS ((rtx *));
extern rtx gen_peephole2_1242 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1243 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1244 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1245 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1246 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1247 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1248 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1249 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1250 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1251 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1252 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1253 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1254 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1255 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1256 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1257 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1258 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1259 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1260 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1261 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1262 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1263 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1264 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1265 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1266 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1267 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1268 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1269 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1270 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1271 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1272 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1273 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1274 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1275 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1276 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1277 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1278 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1279 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1280 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1281 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1282 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1283 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1284 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1285 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1286 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1287 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1288 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1289 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1290 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1291 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1292 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1293 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1294 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1295 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1296 PARAMS ((rtx, rtx *));
extern rtx gen_peephole2_1297 PARAMS ((rtx, rtx *));
extern rtx gen_split_1306 PARAMS ((rtx *));
extern rtx gen_split_1307 PARAMS ((rtx *));
extern rtx gen_split_1308 PARAMS ((rtx *));
extern rtx gen_split_1309 PARAMS ((rtx *));
extern rtx gen_split_1310 PARAMS ((rtx *));
extern rtx gen_split_1311 PARAMS ((rtx *));
extern rtx gen_split_1312 PARAMS ((rtx *));
extern rtx gen_split_1313 PARAMS ((rtx *));



static int recog_1 PARAMS ((rtx, rtx, int *));
static int
recog_1 (x0, insn, pnum_clobbers)
     rtx x0 ATTRIBUTE_UNUSED;
     rtx insn ATTRIBUTE_UNUSED;
     int *pnum_clobbers ATTRIBUTE_UNUSED;
{
  rtx * const operands ATTRIBUTE_UNUSED = &recog_data.operand[0];
  rtx x1 ATTRIBUTE_UNUSED;
  rtx x2 ATTRIBUTE_UNUSED;
  rtx x3 ATTRIBUTE_UNUSED;
  rtx x4 ATTRIBUTE_UNUSED;
  rtx x5 ATTRIBUTE_UNUSED;
  rtx x6 ATTRIBUTE_UNUSED;
  rtx x7 ATTRIBUTE_UNUSED;
  int tem ATTRIBUTE_UNUSED;

  x1 = XEXP (x0, 0);
  switch (GET_CODE (x1))
    {
    case MEM:
      goto L10929;
    case REG:
      goto L10930;
    default:
     break;
   }
 L10842: ATTRIBUTE_UNUSED_LABEL
  if (register_operand (x1, HImode))
    {
      operands[0] = x1;
      goto L139;
    }
 L10852: ATTRIBUTE_UNUSED_LABEL
  if (nonimmediate_operand (x1, HImode))
    {
      operands[0] = x1;
      goto L1074;
    }
 L10853: ATTRIBUTE_UNUSED_LABEL
  if (GET_CODE (x1) == MEM)
    goto L354;
  if (register_operand (x1, HImode))
    {
      operands[0] = x1;
      goto L359;
    }
 L10879: ATTRIBUTE_UNUSED_LABEL
  if (memory_operand (x1, HImode))
    {
      operands[0] = x1;
      goto L1101;
    }
  goto ret0;

 L10929: ATTRIBUTE_UNUSED_LABEL
  if (push_operand (x1, HImode))
    {
      operands[0] = x1;
      goto L342;
    }
  goto L10852;

 L342: ATTRIBUTE_UNUSED_LABEL
  x1 = XEXP (x0, 1);
  if (general_no_elim_operand (x1, HImode))
    {
      operands[1] = x1;
      goto L343;
    }
 L346: ATTRIBUTE_UNUSED_LABEL
  if (nonmemory_no_elim_operand (x1, HImode))
    {
      operands[1] = x1;
      goto L347;
    }
  x1 = XEXP (x0, 0);
  goto L10852;

 L343: ATTRIBUTE_UNUSED_LABEL
  if ((!TARGET_64BIT))
    {
      return 49;
    }
  x1 = XEXP (x0, 1);
  goto L346;

 L347: ATTRIBUTE_UNUSED_LABEL
  if ((TARGET_64BIT))
    {
      return 50;
    }
  x1 = XEXP (x0, 0);
  goto L10852;

 L10930: ATTRIBUTE_UNUSED_LABEL
  if (XINT (x1, 0) == 18)
    goto L1105;
  goto L10842;

 L1105: ATTRIBUTE_UNUSED_LABEL
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) == HImode
      && GET_CODE (x1) == UNSPEC
      && XVECLEN (x1, 0) == 1
      && XINT (x1, 1) == 12)
    goto L1106;
  x1 = XEXP (x0, 0);
  goto L10842;

 L1106: ATTRIBUTE_UNUSED_LABEL
  x2 = XVECEXP (x1, 0, 0);
  if (memory_operand (x2, HImode))
    {
      operands[0] = x2;
      goto L1107;
    }
  x1 = XEXP (x0, 0);
  goto L10842;

 L1107: ATTRIBUTE_UNUSED_LABEL
  if ((TARGET_80387))
    {
      return 166;
    }
  x1 = XEXP (x0, 0);
  goto L10842;

 L139: ATTRIBUTE_UNUSED_LABEL
  x1 = XEXP (x0, 1);
  if (GET_MODE (x1) == HImode
      && GET_CODE (x1) == UNSPEC
      && XVECLEN (x1, 0) == 1
      && XINT (x1, 1) == 9)
    goto L140;
  x1 = XEXP (x0, 0);
  goto L10852;

 L140: ATTRIBUTE_UNUSED_LABEL
  x2 = XVECEXP (x1, 0, 0);
  switch (GET_MODE (x2))
    {
    case CCFPmode:
      goto L10931;
    case CCFPUmode:
      goto L10932;
    default:
      break;
    }
 L219: ATTRIBUTE_UNUSED_LABEL
  if (GET_CODE (x2) == REG
      && XINT (x2, 0) == 18
      && (TARGET_80387))
    {
      return 30;
    }
  x1 = XEXP (x0, 0);
  goto L10852;

 L10931: ATTRIBUTE_UNUSED_LABEL
  if (GET_CODE (x2) == COMPARE)
    goto L154;
  goto L219;

 L154: ATTRIBUTE_UNUSED_LABEL
  x3 = XEXP (x2, 0);
  switch (GET_MODE (x3))
    {
    case SFmode:
      goto L10933;
    case DFmode:
      goto L10934;
    case XFmode:
      goto L10935;
    case TFmode:
      goto L10936;
    default:
      break;
    }
 L141: ATTRIBUTE_UNUSED_LABEL
  if (register_operand (x3, VOIDmode))
    {
      operands[1] = x3;
      goto L142;
    }
  goto L219;

 L10933: ATTRIBUTE_UNUSED_LABEL
  if (register_operand (x3, SF