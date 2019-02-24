/* A Bison parser, made from parse.y
   by GNU bison 1.34.  */

#define YYBISON 1  /* Identify Bison output.  */

# define	IDENTIFIER	257
# define	TYPENAME	258
# define	SELFNAME	259
# define	PFUNCNAME	260
# define	SCSPEC	261
# define	TYPESPEC	262
# define	CV_QUALIFIER	263
# define	CONSTANT	264
# define	VAR_FUNC_NAME	265
# define	STRING	266
# define	ELLIPSIS	267
# define	SIZEOF	268
# define	ENUM	269
# define	IF	270
# define	ELSE	271
# define	WHILE	272
# define	DO	273
# define	FOR	274
# define	SWITCH	275
# define	CASE	276
# define	DEFAULT	277
# define	BREAK	278
# define	CONTINUE	279
# define	RETURN_KEYWORD	280
# define	GOTO	281
# define	ASM_KEYWORD	282
# define	TYPEOF	283
# define	ALIGNOF	284
# define	SIGOF	285
# define	ATTRIBUTE	286
# define	EXTENSION	287
# define	LABEL	288
# define	REALPART	289
# define	IMAGPART	290
# define	VA_ARG	291
# define	AGGR	292
# define	VISSPEC	293
# define	DELETE	294
# define	NEW	295
# define	THIS	296
# define	OPERATOR	297
# define	CXX_TRUE	298
# define	CXX_FALSE	299
# define	NAMESPACE	300
# define	TYPENAME_KEYWORD	301
# define	USING	302
# define	LEFT_RIGHT	303
# define	TEMPLATE	304
# define	TYPEID	305
# define	DYNAMIC_CAST	306
# define	STATIC_CAST	307
# define	REINTERPRET_CAST	308
# define	CONST_CAST	309
# define	SCOPE	310
# define	EXPORT	311
# define	EMPTY	312
# define	PTYPENAME	313
# define	NSNAME	314
# define	THROW	315
# define	ASSIGN	316
# define	OROR	317
# define	ANDAND	318
# define	MIN_MAX	319
# define	EQCOMPARE	320
# define	ARITHCOMPARE	321
# define	LSHIFT	322
# define	RSHIFT	323
# define	POINTSAT_STAR	324
# define	DOT_STAR	325
# define	UNARY	326
# define	PLUSPLUS	327
# define	MINUSMINUS	328
# define	HYPERUNARY	329
# define	POINTSAT	330
# define	TRY	331
# define	CATCH	332
# define	EXTERN_LANG_STRING	333
# define	ALL	334
# define	PRE_PARSED_CLASS_DECL	335
# define	DEFARG	336
# define	DEFARG_MARKER	337
# define	PRE_PARSED_FUNCTION_DECL	338
# define	TYPENAME_DEFN	339
# define	IDENTIFIER_DEFN	340
# define	PTYPENAME_DEFN	341
# define	END_OF_LINE	342
# define	END_OF_SAVED_INPUT	343

#line 30 "parse.y"

/* Cause the `yydebug' variable to be defined.  */
#define YYDEBUG 1

/* !kawai! */
#include "../gcc/config.h"

#include "../gcc/system.h"

#include "../gcc/tree.h"
#include "../gcc/input.h"
#include "../gcc/flags.h"
#include "cp-tree.h"
#include "lex.h"
#include "../gcc/output.h"
#include "../gcc/except.h"
#include "../gcc/toplev.h"
#include "../gcc/ggc.h"
/* end of !kawai! */

extern struct obstack permanent_obstack;

/* Like YYERROR but do call yyerror.  */
#define YYERROR1 { yyerror ("syntax error"); YYERROR; }

#define OP0(NODE) (TREE_OPERAND (NODE, 0))
#define OP1(NODE) (TREE_OPERAND (NODE, 1))

/* Contains the statement keyword (if/while/do) to include in an
   error message if the user supplies an empty conditional expression.  */
static const char *cond_stmt_keyword;

/* Nonzero if we have an `extern "C"' acting as an extern specifier.  */
int have_extern_spec;
int used_extern_spec;

/* List of types and structure classes of the current declaration.  */
static tree current_declspecs;

/* List of prefix attributes in effect.
   Prefix attributes are parsed by the reserved_declspecs and declmods
   rules.  They create a list that contains *both* declspecs and attrs.  */
/* ??? It is not clear yet that all cases where an attribute can now appear in
   a declspec list have been updated.  */
static tree prefix_attributes;

/* When defining an enumeration, this is the type of the enumeration.  */
static tree current_enum_type;

/* When parsing a conversion operator name, this is the scope of the
   operator itself.  */
static tree saved_scopes;

static tree empty_parms PARAMS ((void));
static tree parse_decl0 PARAMS ((tree, tree, tree, tree, int));
static tree parse_decl PARAMS ((tree, tree, int));
static void parse_end_decl PARAMS ((tree, tree, tree));
static tree parse_field0 PARAMS ((tree, tree, tree, tree, tree, tree));
static tree parse_field PARAMS ((tree, tree, tree, tree));
static tree parse_bitfield0 PARAMS ((tree, tree, tree, tree, tree));
static tree parse_bitfield PARAMS ((tree, tree, tree));
static tree parse_method PARAMS ((tree, tree, tree));
static void frob_specs PARAMS ((tree, tree));
static void check_class_key PARAMS ((tree, tree));

/* Cons up an empty parameter list.  */
static inline tree
empty_parms ()
{
  tree parms;

#ifndef NO_IMPLICIT_EXTERN_C
  if (in_system_header && current_class_type == NULL 
      && current_lang_name == lang_name_c)
    parms = NULL_TREE;
  else
#endif
  parms = void_list_node;
  return parms;
}

/* Record the decl-specifiers, attributes and type lookups from the
   decl-specifier-seq in a declaration.  */

static void
frob_specs (specs_attrs, lookups)
     tree specs_attrs, lookups;
{
  save_type_access_control (lookups);
  split_specs_attrs (specs_attrs, &current_declspecs, &prefix_attributes);
  if (current_declspecs
      && TREE_CODE (current_declspecs) != TREE_LIST)
    current_declspecs = build_tree_list (NULL_TREE, current_declspecs);
  if (have_extern_spec && !used_extern_spec)
    {
      /* We have to indicate that there is an "extern", but that it
         was part of a language specifier.  For instance,
	 
    	    extern "C" typedef int (*Ptr) ();

         is well formed.  */
      current_declspecs = tree_cons (error_mark_node,
				     get_identifier ("extern"), 
				     current_declspecs);
      used_extern_spec = 1;
    }
}

static tree
parse_decl (declarator, attributes, initialized)
     tree declarator, attributes;
     int initialized;
{
  return start_decl (declarator, current_declspecs, initialized,
		     attributes, prefix_attributes);
}

static tree
parse_decl0 (declarator, specs_attrs, lookups, attributes, initialized)
     tree declarator, specs_attrs, lookups, attributes;
     int initialized;
{
  frob_specs (specs_attrs, lookups);
  return parse_decl (declarator, attributes, initialized);
}

static void
parse_end_decl (decl, init, asmspec)
     tree decl, init, asmspec;
{
  /* If decl is NULL_TREE, then this was a variable declaration using
     () syntax for the initializer, so we handled it in grokdeclarator.  */
  if (decl)
    decl_type_access_control (decl);
  cp_finish_decl (decl, init, asmspec, init ? LOOKUP_ONLYCONVERTING : 0);
}

static tree
parse_field (declarator, attributes, asmspec, init)
     tree declarator, attributes, asmspec, init;
{
  tree d = grokfield (declarator, current_declspecs, init, asmspec,
		      chainon (attributes, prefix_attributes));
  decl_type_access_control (d);
  return d;
}

static tree
parse_field0 (declarator, specs_attrs, lookups, attributes, asmspec, init)
     tree declarator, specs_attrs, lookups, attributes, asmspec, init;
{
  frob_specs (specs_attrs, lookups);
  return parse_field (declarator, attributes, asmspec, init);
}

static tree
parse_bitfield (declarator, attributes, width)
     tree declarator, attributes, width;
{
  tree d = grokbitfield (declarator, current_declspecs, width);
  cplus_decl_attributes (&d, chainon (attributes, prefix_attributes), 0);
  decl_type_access_control (d);
  return d;
}

static tree
parse_bitfield0 (declarator, specs_attrs, lookups, attributes, width)
     tree declarator, specs_attrs, lookups, attributes, width;
{
  frob_specs (specs_attrs, lookups);
  return parse_bitfield (declarator, attributes, width);
}

static tree
parse_method (declarator, specs_attrs, lookups)
     tree declarator, specs_attrs, lookups;
{
  tree d;
  frob_specs (specs_attrs, lookups);
  d = start_method (current_declspecs, declarator, prefix_attributes);
  decl_type_access_control (d);
  return d;
}

static void
check_class_key (key, aggr)
     tree key;
     tree aggr;
{
  if (TREE_CODE (key) == TREE_LIST)
    key = TREE_VALUE (key);
  if ((key == union_type_node) != (TREE_CODE (aggr) == UNION_TYPE))
    pedwarn ("`%s' tag used in naming `%#T'",
	     key == union_type_node ? "union"
	     : key == record_type_node ? "struct" : "class", aggr);
}

void
cp_parse_init ()
{
  ggc_add_tree_root (&current_declspecs, 1);
  ggc_add_tree_root (&prefix_attributes, 1);
  ggc_add_tree_root (&current_enum_type, 1);
  ggc_add_tree_root (&saved_scopes, 1);
}

/* Rename the "yyparse" function so that we can override it elsewhere.  */
#define yyparse yyparse_1

#line 240 "parse.y"
#ifndef YYSTYPE
typedef union {
  long itype; 
  tree ttype; 
  char *strtype; 
  enum tree_code code; 
  flagged_type_tree ftype;
  struct unparsed_text *pi;
} yystype;
# define YYSTYPE yystype
#endif
#line 447 "parse.y"

/* Tell yyparse how to print a token's value, if yydebug is set.  */
#define YYPRINT(FILE,YYCHAR,YYLVAL) yyprint(FILE,YYCHAR,YYLVAL)
extern void yyprint			PARAMS ((FILE *, int, YYSTYPE));
#ifndef YYDEBUG
# define YYDEBUG 0
#endif



#define	YYFINAL		1830
#define	YYFLAG		-32768
#define	YYNTBASE	114

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 343 ? yytranslate[x] : 404)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   112,     2,     2,     2,    85,    73,     2,
      95,   110,    83,    81,    62,    82,    94,    84,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    65,    63,
      77,    67,    78,    68,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    96,     2,   113,    72,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    61,    71,   111,    91,     2,     2,     2,
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
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    64,    66,    69,    70,    74,
      75,    76,    79,    80,    86,    87,    88,    89,    90,    92,
      93,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109
};

#if YYDEBUG
static const short yyprhs[] =
{
       0,     0,     1,     3,     4,     7,    10,    12,    13,    14,
      15,    17,    19,    20,    23,    26,    28,    29,    33,    35,
      41,    46,    52,    57,    58,    65,    66,    72,    74,    77,
      79,    82,    83,    90,    93,    97,   101,   105,   109,   114,
     115,   121,   124,   128,   130,   132,   135,   138,   140,   143,
     144,   150,   154,   156,   158,   160,   164,   166,   167,   170,
     173,   177,   179,   183,   185,   189,   191,   195,   198,   201,
     204,   206,   208,   214,   219,   222,   225,   229,   233,   236,
     239,   243,   247,   250,   253,   256,   259,   262,   265,   267,
     269,   271,   273,   274,   276,   279,   280,   282,   283,   290,
     294,   298,   302,   303,   312,   318,   319,   329,   336,   337,
     346,   352,   353,   363,   370,   373,   376,   378,   381,   383,
     390,   399,   404,   411,   418,   423,   426,   428,   431,   434,
     436,   439,   441,   444,   447,   452,   455,   458,   459,   460,
     462,   466,   469,   473,   475,   480,   483,   488,   491,   496,
     499,   501,   503,   505,   507,   509,   511,   513,   515,   517,
     519,   521,   523,   524,   531,   532,   539,   540,   546,   547,
     553,   554,   562,   563,   571,   572,   579,   580,   587,   588,
     589,   595,   601,   603,   605,   611,   617,   618,   620,   622,
     623,   625,   627,   631,   633,   635,   638,   640,   644,   646,
     648,   650,   652,   654,   656,   658,   660,   664,   666,   670,
     671,   673,   675,   676,   684,   686,   688,   692,   697,   701,
     705,   709,   713,   717,   719,   721,   723,   726,   729,   732,
     735,   738,   741,   744,   749,   752,   757,   760,   764,   768,
     773,   778,   784,   790,   797,   800,   805,   811,   814,   817,
     821,   825,   829,   831,   835,   838,   842,   847,   849,   852,
     858,   860,   864,   868,   872,   876,   880,   884,   888,   892,
     896,   900,   904,   908,   912,   916,   920,   924,   928,   932,
     936,   942,   946,   950,   952,   955,   957,   961,   965,   969,
     973,   977,   981,   985,   989,   993,   997,  1001,  1005,  1009,
    1013,  1017,  1021,  1025,  1029,  1035,  1039,  1043,  1045,  1048,
    1052,  1056,  1058,  1060,  1062,  1064,  1066,  1067,  1073,  1079,
    1085,  1091,  1097,  1099,  1101,  1103,  1105,  1108,  1110,  1113,
    1116,  1120,  1125,  1130,  1132,  1134,  1136,  1140,  1142,  1144,
    1146,  1148,  1150,  1154,  1158,  1162,  1163,  1168,  1173,  1176,
    1181,  1184,  1191,  1196,  1199,  1202,  1204,  1209,  1211,  1219,
    1227,  1235,  1243,  1248,  1253,  1256,  1259,  1262,  1264,  1269,
    1272,  1275,  1281,  1285,  1288,  1291,  1297,  1301,  1307,  1311,
    1316,  1323,  1326,  1328,  1331,  1333,  1336,  1338,  1340,  1342,
    1345,  1346,  1349,  1352,  1356,  1360,  1364,  1367,  1370,  1373,
    1375,  1377,  1379,  1382,  1385,  1388,  1391,  1393,  1395,  1397,
    1399,  1402,  1405,  1409,  1413,  1417,  1422,  1424,  1427,  1430,
    1432,  1434,  1437,  1440,  1443,  1445,  1448,  1451,  1455,  1457,
    1460,  1463,  1465,  1467,  1469,  1471,  1476,  1481,  1486,  1491,
    1493,  1495,  1497,  1499,  1503,  1505,  1509,  1511,  1515,  1516,
    1521,  1522,  1529,  1533,  1534,  1539,  1541,  1545,  1549,  1550,
    1555,  1559,  1560,  1562,  1564,  1567,  1574,  1576,  1580,  1581,
    1583,  1588,  1595,  1600,  1602,  1604,  1606,  1608,  1610,  1614,
    1615,  1618,  1620,  1623,  1627,  1632,  1634,  1636,  1640,  1645,
    1649,  1655,  1659,  1663,  1667,  1668,  1672,  1676,  1680,  1681,
    1684,  1687,  1688,  1695,  1696,  1702,  1705,  1708,  1711,  1712,
    1713,  1714,  1726,  1728,  1729,  1731,  1732,  1734,  1736,  1739,
    1742,  1745,  1748,  1751,  1754,  1758,  1763,  1767,  1770,  1774,
    1779,  1781,  1784,  1786,  1789,  1792,  1795,  1798,  1802,  1806,
    1809,  1810,  1813,  1817,  1819,  1824,  1826,  1830,  1832,  1834,
    1837,  1840,  1844,  1848,  1849,  1851,  1855,  1858,  1861,  1863,
    1866,  1869,  1872,  1875,  1878,  1881,  1884,  1886,  1889,  1892,
    1896,  1898,  1901,  1904,  1909,  1914,  1917,  1919,  1925,  1930,
    1932,  1933,  1935,  1939,  1940,  1942,  1946,  1948,  1950,  1952,
    1954,  1959,  1964,  1969,  1974,  1979,  1983,  1988,  1993,  1998,
    2003,  2007,  2010,  2012,  2014,  2018,  2020,  2024,  2027,  2029,
    2036,  2037,  2040,  2042,  2045,  2047,  2050,  2054,  2058,  2060,
    2064,  2066,  2069,  2073,  2077,  2080,  2083,  2087,  2089,  2094,
    2099,  2103,  2107,  2110,  2112,  2114,  2117,  2119,  2121,  2124,
    2127,  2129,  2132,  2136,  2140,  2143,  2146,  2150,  2152,  2156,
    2160,  2163,  2166,  2170,  2172,  2177,  2181,  2186,  2190,  2192,
    2195,  2198,  2201,  2204,  2207,  2210,  2213,  2215,  2218,  2223,
    2228,  2231,  2233,  2235,  2237,  2239,  2242,  2247,  2251,  2255,
    2258,  2261,  2264,  2267,  2269,  2272,  2275,  2278,  2281,  2285,
    2287,  2290,  2294,  2299,  2302,  2305,  2308,  2311,  2314,  2317,
    2322,  2325,  2327,  2330,  2333,  2337,  2339,  2343,  2346,  2350,
    2353,  2356,  2360,  2362,  2366,  2371,  2373,  2376,  2380,  2383,
    2386,  2388,  2392,  2395,  2398,  2400,  2403,  2407,  2409,  2413,
    2420,  2425,  2430,  2434,  2440,  2444,  2448,  2452,  2455,  2457,
    2459,  2462,  2465,  2468,  2469,  2471,  2473,  2476,  2480,  2481,
    2486,  2488,  2489,  2490,  2496,  2498,  2499,  2503,  2505,  2508,
    2510,  2513,  2514,  2519,  2521,  2522,  2523,  2529,  2530,  2531,
    2539,  2540,  2541,  2542,  2543,  2556,  2557,  2558,  2566,  2567,
    2573,  2574,  2582,  2583,  2588,  2591,  2594,  2597,  2601,  2608,
    2617,  2628,  2637,  2650,  2661,  2672,  2677,  2681,  2684,  2687,
    2689,  2691,  2693,  2695,  2697,  2698,  2699,  2705,  2706,  2707,
    2713,  2715,  2718,  2719,  2720,  2721,  2727,  2729,  2731,  2735,
    2739,  2742,  2745,  2748,  2751,  2754,  2756,  2759,  2760,  2762,
    2763,  2765,  2767,  2768,  2770,  2772,  2776,  2781,  2789,  2791,
    2795,  2796,  2798,  2800,  2802,  2805,  2808,  2811,  2813,  2816,
    2819,  2820,  2824,  2826,  2828,  2830,  2833,  2836,  2839,  2844,
    2847,  2850,  2853,  2856,  2859,  2862,  2864,  2867,  2869,  2872,
    2874,  2876,  2877,  2878,  2880,  2886,  2890,  2891,  2895,  2896,
    2897,  2902,  2905,  2907,  2909,  2911,  2915,  2916,  2920,  2924,
    2928,  2930,  2931,  2935,  2939,  2943,  2947,  2951,  2955,  2959,
    2963,  2967,  2971,  2975,  2979,  2983,  2987,  2991,  2995,  2999,
    3003,  3007,  3011,  3015,  3019,  3023,  3028,  3032,  3036,  3040,
    3044,  3049,  3053,  3057,  3063,  3069,  3074,  3078
};
static const short yyrhs[] =
{
      -1,   115,     0,     0,   116,   122,     0,   115,   122,     0,
     115,     0,     0,     0,     0,    33,     0,    28,     0,     0,
     123,   124,     0,   155,   152,     0,   149,     0,     0,    57,
     125,   146,     0,   146,     0,   121,    95,   223,   110,    63,
       0,   136,    61,   117,   111,     0,   136,   118,   155,   119,
     152,     0,   136,   118,   149,   119,     0,     0,    46,   170,
      61,   126,   117,   111,     0,     0,    46,    61,   127,   117,
     111,     0,   128,     0,   130,    63,     0,   132,     0,   120,
     124,     0,     0,    46,   170,    67,   129,   135,    63,     0,
      48,   312,     0,    48,   326,   312,     0,    48,   326,   213,
       0,    48,   134,   170,     0,    48,   326,   170,     0,    48,
     326,   134,   170,     0,     0,    48,    46,   133,   135,    63,
       0,    60,    56,     0,   134,    60,    56,     0,   213,     0,
     312,     0,   326,   312,     0,   326,   213,     0,    99,     0,
     136,    99,     0,     0,    50,    77,   138,   141,    78,     0,
      50,    77,    78,     0,   137,     0,   139,     0,   145,     0,
     141,    62,   145,     0,   170,     0,     0,   272,   142,     0,
      47,   142,     0,   137,   272,   142,     0,   143,     0,   143,
      67,   229,     0,   390,     0,   390,    67,   208,     0,   144,
       0,   144,    67,   191,     0,   140,   147,     0,   140,     1,
       0,   155,   152,     0,   148,     0,   146,     0,   136,   118,
     155,   119,   152, 