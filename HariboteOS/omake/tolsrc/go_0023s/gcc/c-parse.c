/* A Bison parser, made from c-parse.y
   by GNU bison 1.34.  */

#define YYBISON 1  /* Identify Bison output.  */

# define	IDENTIFIER	257
# define	TYPENAME	258
# define	SCSPEC	259
# define	TYPESPEC	260
# define	TYPE_QUAL	261
# define	CONSTANT	262
# define	STRING	263
# define	ELLIPSIS	264
# define	SIZEOF	265
# define	ENUM	266
# define	STRUCT	267
# define	UNION	268
# define	IF	269
# define	ELSE	270
# define	WHILE	271
# define	DO	272
# define	FOR	273
# define	SWITCH	274
# define	CASE	275
# define	DEFAULT	276
# define	BREAK	277
# define	CONTINUE	278
# define	RETURN	279
# define	GOTO	280
# define	ASM_KEYWORD	281
# define	TYPEOF	282
# define	ALIGNOF	283
# define	ATTRIBUTE	284
# define	EXTENSION	285
# define	LABEL	286
# define	REALPART	287
# define	IMAGPART	288
# define	VA_ARG	289
# define	CHOOSE_EXPR	290
# define	TYPES_COMPATIBLE_P	291
# define	PTR_VALUE	292
# define	PTR_BASE	293
# define	PTR_EXTENT	294
# define	STRING_FUNC_NAME	295
# define	VAR_FUNC_NAME	296
# define	ASSIGN	297
# define	OROR	298
# define	ANDAND	299
# define	EQCOMPARE	300
# define	ARITHCOMPARE	301
# define	LSHIFT	302
# define	RSHIFT	303
# define	UNARY	304
# define	PLUSPLUS	305
# define	MINUSMINUS	306
# define	HYPERUNARY	307
# define	POINTSAT	308
# define	INTERFACE	309
# define	IMPLEMENTATION	310
# define	END	311
# define	SELECTOR	312
# define	DEFS	313
# define	ENCODE	314
# define	CLASSNAME	315
# define	PUBLIC	316
# define	PRIVATE	317
# define	PROTECTED	318
# define	PROTOCOL	319
# define	OBJECTNAME	320
# define	CLASS	321
# define	ALIAS	322

#line 34 "c-parse.y"

#include "config.h"
#include "system.h"
#include "tree.h"
#include "input.h"
#include "cpplib.h"
#include "intl.h"
#include "timevar.h"
#include "c-lex.h"
#include "c-tree.h"
#include "c-pragma.h"
#include "flags.h"
#include "output.h"
#include "toplev.h"
#include "ggc.h"
  
#ifdef MULTIBYTE_CHARS
#include <locale.h>
#endif


/* Like YYERROR but do call yyerror.  */
#define YYERROR1 { yyerror ("syntax error"); YYERROR; }

/* Cause the "yydebug" variable to be defined.  */
#define YYDEBUG 1

/* Rename the "yyparse" function so that we can override it elsewhere.  */
#define yyparse yyparse_1

#line 67 "c-parse.y"
#ifndef YYSTYPE
typedef union {long itype; tree ttype; enum tree_code code;
	const char *filename; int lineno; } yystype;
# define YYSTYPE yystype
#endif
#line 200 "c-parse.y"

/* Number of statements (loosely speaking) and compound statements 
   seen so far.  */
static int stmt_count;
static int compstmt_count;
  
/* Input file and line number of the end of the body of last simple_if;
   used by the stmt-rule immediately after simple_if returns.  */
static const char *if_stmt_file;
static int if_stmt_line;

/* List of types and structure classes of the current declaration.  */
static tree current_declspecs = NULL_TREE;
static tree prefix_attributes = NULL_TREE;

/* List of all the attributes applying to the identifier currently being
   declared; includes prefix_attributes and possibly some more attributes
   just after a comma.  */
static tree all_prefix_attributes = NULL_TREE;

/* Stack of saved values of current_declspecs, prefix_attributes and
   all_prefix_attributes.  */
static tree declspec_stack;

/* PUSH_DECLSPEC_STACK is called from setspecs; POP_DECLSPEC_STACK
   should be called from the productions making use of setspecs.  */
#define PUSH_DECLSPEC_STACK						 ¥
  do {									 ¥
    declspec_stack = tree_cons (build_tree_list (prefix_attributes,	 ¥
						 all_prefix_attributes), ¥
				current_declspecs,			 ¥
				declspec_stack);			 ¥
  } while (0)

#define POP_DECLSPEC_STACK						¥
  do {									¥
    current_declspecs = TREE_VALUE (declspec_stack);			¥
    prefix_attributes = TREE_PURPOSE (TREE_PURPOSE (declspec_stack));	¥
    all_prefix_attributes = TREE_VALUE (TREE_PURPOSE (declspec_stack));	¥
    declspec_stack = TREE_CHAIN (declspec_stack);			¥
  } while (0)

/* For __extension__, save/restore the warning flags which are
   controlled by __extension__.  */
#define SAVE_WARN_FLAGS()			¥
	size_int (pedantic			¥
		  | (warn_pointer_arith << 1)	¥
		  | (warn_traditional << 2))

#define RESTORE_WARN_FLAGS(tval)		¥
  do {						¥
    int val = tree_low_cst (tval, 0);		¥
    pedantic = val & 1;				¥
    warn_pointer_arith = (val >> 1) & 1;	¥
    warn_traditional = (val >> 2) & 1;		¥
  } while (0)


#define OBJC_NEED_RAW_IDENTIFIER(VAL)	/* nothing */

/* Tell yyparse how to print a token's value, if yydebug is set.  */

#define YYPRINT(FILE,YYCHAR,YYLVAL) yyprint(FILE,YYCHAR,YYLVAL)

static void yyprint	  PARAMS ((FILE *, int, YYSTYPE));
static void yyerror	  PARAMS ((const char *));
static int yylexname	  PARAMS ((void));
static inline int _yylex  PARAMS ((void));
static int  yylex	  PARAMS ((void));
static void init_reswords PARAMS ((void));

/* Add GC roots for variables local to this file.  */
void
c_parse_init ()
{
  init_reswords ();

  ggc_add_tree_root (&declspec_stack, 1);
  ggc_add_tree_root (&current_declspecs, 1);
  ggc_add_tree_root (&prefix_attributes, 1);
  ggc_add_tree_root (&all_prefix_attributes, 1);
}

#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		900
#define	YYFLAG		-32768
#define	YYNTBASE	91

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 322 ? yytranslate[x] : 289)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    87,     2,     2,     2,    60,    51,     2,
      67,    83,    58,    56,    88,    57,    66,    59,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    46,    84,
       2,    44,     2,    45,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    68,     2,    90,    50,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    89,    49,    85,    86,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    47,    48,
      52,    53,    54,    55,    61,    62,    63,    64,    65,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     1,     3,     4,     7,     8,    12,    14,    16,
      22,    25,    29,    34,    39,    42,    45,    48,    50,    51,
      52,    62,    67,    68,    69,    79,    84,    85,    86,    95,
      99,   101,   103,   105,   107,   109,   111,   113,   115,   117,
     119,   120,   122,   124,   128,   130,   133,   136,   139,   142,
     145,   150,   153,   158,   161,   164,   166,   168,   170,   175,
     177,   181,   185,   189,   193,   197,   201,   205,   209,   213,
     217,   221,   225,   226,   231,   232,   237,   238,   239,   247,
     248,   254,   258,   262,   264,   266,   268,   270,   271,   279,
     283,   287,   291,   295,   300,   307,   316,   323,   328,   332,
     336,   339,   342,   344,   347,   348,   350,   353,   357,   359,
     361,   364,   367,   372,   377,   380,   383,   387,   388,   390,
     395,   400,   404,   408,   411,   414,   416,   419,   422,   425,
     428,   431,   433,   436,   438,   441,   444,   447,   450,   453,
     456,   458,   461,   464,   467,   470,   473,   476,   479,   482,
     485,   488,   491,   494,   497,   500,   503,   506,   508,   511,
     514,   517,   520,   523,   526,   529,   532,   535,   538,   541,
     544,   547,   550,   553,   556,   559,   562,   565,   568,   571,
     574,   577,   580,   583,   586,   589,   592,   595,   598,   601,
     604,   607,   610,   613,   616,   619,   622,   625,   628,   631,
     634,   637,   640,   642,   644,   646,   648,   650,   652,   654,
     656,   658,   660,   662,   664,   666,   668,   670,   672,   674,
     676,   678,   680,   682,   684,   686,   688,   690,   692,   694,
     696,   698,   700,   702,   704,   706,   708,   710,   712,   714,
     716,   718,   720,   722,   724,   726,   728,   730,   732,   734,
     736,   738,   740,   742,   744,   746,   748,   750,   752,   753,
     755,   757,   759,   761,   763,   765,   767,   769,   774,   779,
     781,   786,   788,   793,   794,   799,   800,   807,   811,   812,
     819,   823,   824,   826,   828,   831,   838,   840,   844,   845,
     847,   852,   859,   864,   866,   868,   870,   872,   874,   875,
     880,   882,   883,   886,   888,   892,   896,   899,   900,   905,
     907,   908,   913,   915,   917,   919,   922,   925,   931,   935,
     936,   937,   945,   946,   947,   955,   957,   959,   964,   968,
     971,   975,   977,   979,   981,   985,   988,   990,   994,   997,
    1001,  1005,  1010,  1014,  1019,  1023,  1026,  1028,  1030,  1033,
    1035,  1038,  1040,  1043,  1044,  1052,  1058,  1059,  1067,  1073,
    1074,  1083,  1084,  1092,  1095,  1098,  1101,  1102,  1104,  1105,
    1107,  1109,  1112,  1113,  1117,  1120,  1124,  1129,  1133,  1135,
    1137,  1140,  1142,  1147,  1149,  1154,  1159,  1166,  1172,  1177,
    1184,  1190,  1192,  1196,  1198,  1200,  1204,  1205,  1209,  1210,
    1212,  1213,  1215,  1218,  1220,  1222,  1224,  1228,  1231,  1235,
    1240,  1244,  1247,  1250,  1252,  1256,  1261,  1264,  1268,  1272,
    1277,  1282,  1288,  1294,  1296,  1298,  1300,  1302,  1304,  1307,
    1310,  1313,  1316,  1318,  1321,  1324,  1327,  1329,  1332,  1335,
    1338,  1341,  1343,  1346,  1348,  1350,  1352,  1354,  1357,  1358,
    1359,  1360,  1361,  1362,  1364,  1366,  1369,  1373,  1375,  1378,
    1380,  1382,  1388,  1390,  1392,  1395,  1398,  1401,  1404,  1405,
    1411,  1412,  1417,  1418,  1419,  1421,  1424,  1428,  1432,  1436,
    1437,  1442,  1444,  1448,  1449,  1450,  1458,  1464,  1467,  1468,
    1469,  1470,  1471,  1484,  1485,  1492,  1495,  1497,  1499,  1502,
    1506,  1509,  1512,  1515,  1519,  1526,  1535,  1546,  1559,  1563,
    1568,  1570,  1574,  1580,  1583,  1589,  1590,  1592,  1593,  1595,
    1596,  1598,  1600,  1604,  1609,  1617,  1619,  1623,  1624,  1628,
    1631,  1632,  1633,  1640,  1643,  1644,  1646,  1648,  1652,  1654,
    1658,  1663,  1668,  1672,  1677,  1681,  1686,  1691,  1695,  1700,
    1704,  1706,  1707,  1711,  1713,  1716,  1718,  1722,  1724,  1728
};
static const short yyrhs[] =
{
      -1,    92,     0,     0,    93,    9