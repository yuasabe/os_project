/* Generated automatically by the program `gencodes'
   from the machine description file `md'.  */

#ifndef GCC_INSN_CODES_H
#define GCC_INSN_CODES_H

enum insn_code {
  CODE_FOR_cmpdi_ccno_1_rex64 = 0,
  CODE_FOR_cmpdi_1_insn_rex64 = 2,
  CODE_FOR_cmpqi_ext_3_insn = 15,
  CODE_FOR_cmpqi_ext_3_insn_rex64 = 16,
  CODE_FOR_x86_fnstsw_1 = 30,
  CODE_FOR_x86_sahf_1 = 31,
  CODE_FOR_popsi1 = 42,
  CODE_FOR_movsi_insv_1 = 73,
  CODE_FOR_pushdi2_rex64 = 77,
  CODE_FOR_popdi1 = 80,
  CODE_FOR_swapxf = 105,
  CODE_FOR_swaptf = 106,
  CODE_FOR_zero_extendhisi2_and = 107,
  CODE_FOR_zero_extendsidi2_32 = 115,
  CODE_FOR_zero_extendsidi2_rex64 = 116,
  CODE_FOR_zero_extendhidi2 = 117,
  CODE_FOR_zero_extendqidi2 = 118,
  CODE_FOR_extendsidi2_rex64 = 120,
  CODE_FOR_extendhidi2 = 121,
  CODE_FOR_extendqidi2 = 122,
  CODE_FOR_extendhisi2 = 123,
  CODE_FOR_extendqihi2 = 125,
  CODE_FOR_extendqisi2 = 126,
  CODE_FOR_truncdfsf2_3 = 142,
  CODE_FOR_truncdfsf2_sse_only = 143,
  CODE_FOR_fix_truncdi_nomemory = 153,
  CODE_FOR_fix_truncdi_memory = 154,
  CODE_FOR_fix_truncsfdi_sse = 155,
  CODE_FOR_fix_truncdfdi_sse = 156,
  CODE_FOR_fix_truncsi_nomemory = 158,
  CODE_FOR_fix_truncsi_memory = 159,
  CODE_FOR_fix_truncsfsi_sse = 160,
  CODE_FOR_fix_truncdfsi_sse = 161,
  CODE_FOR_fix_trunchi_nomemory = 163,
  CODE_FOR_fix_trunchi_memory = 164,
  CODE_FOR_x86_fnstcw_1 = 165,
  CODE_FOR_x86_fldcw_1 = 166,
  CODE_FOR_floathisf2 = 167,
  CODE_FOR_floathidf2 = 173,
  CODE_FOR_floathixf2 = 179,
  CODE_FOR_floathitf2 = 180,
  CODE_FOR_floatsixf2 = 181,
  CODE_FOR_floatsitf2 = 182,
  CODE_FOR_floatdixf2 = 183,
  CODE_FOR_floatditf2 = 184,
  CODE_FOR_addqi3_cc = 191,
  CODE_FOR_addsi_1_zext = 208,
  CODE_FOR_addqi_ext_1 = 227,
  CODE_FOR_subdi3_carry_rex64 = 231,
  CODE_FOR_subsi3_carry = 235,
  CODE_FOR_subsi3_carry_zext = 236,
  CODE_FOR_divqi3 = 266,
  CODE_FOR_udivqi3 = 267,
  CODE_FOR_divmodhi4 = 274,
  CODE_FOR_udivmoddi4 = 275,
  CODE_FOR_udivmodsi4 = 277,
  CODE_FOR_testsi_1 = 281,
  CODE_FOR_andqi_ext_0 = 302,
  CODE_FOR_negsf2_memory = 355,
  CODE_FOR_negsf2_ifs = 356,
  CODE_FOR_negdf2_memory = 358,
  CODE_FOR_negdf2_ifs = 359,
  CODE_FOR_abssf2_memory = 374,
  CODE_FOR_abssf2_ifs = 375,
  CODE_FOR_absdf2_memory = 377,
  CODE_FOR_absdf2_ifs = 378,
  CODE_FOR_ashldi3_1 = 405,
  CODE_FOR_x86_shld_1 = 407,
  CODE_FOR_ashrdi3_63_rex64 = 418,
  CODE_FOR_ashrdi3_1 = 423,
  CODE_FOR_x86_shrd_1 = 425,
  CODE_FOR_ashrsi3_31 = 426,
  CODE_FOR_lshrdi3_1 = 448,
  CODE_FOR_setcc_2 = 487,
  CODE_FOR_jump = 502,
  CODE_FOR_doloop_end_internal = 507,
  CODE_FOR_blockage = 513,
  CODE_FOR_return_internal = 514,
  CODE_FOR_return_pop_internal = 515,
  CODE_FOR_return_indirect_internal = 516,
  CODE_FOR_nop = 517,
  CODE_FOR_prologue_set_got = 518,
  CODE_FOR_prologue_get_pc = 519,
  CODE_FOR_eh_return_si = 520,
  CODE_FOR_eh_return_di = 521,
  CODE_FOR_leave = 522,
  CODE_FOR_leave_rex64 = 523,
  CODE_FOR_ffssi_1 = 524,
  CODE_FOR_sqrtsf2_1 = 559,
  CODE_FOR_sqrtsf2_1_sse_only = 560,
  CODE_FOR_sqrtsf2_i387 = 561,
  CODE_FOR_sqrtdf2_1 = 562,
  CODE_FOR_sqrtdf2_1_sse_only = 563,
  CODE_FOR_sqrtdf2_i387 = 564,
  CODE_FOR_sqrtxf2 = 566,
  CODE_FOR_sqrttf2 = 567,
  CODE_FOR_sindf2 = 572,
  CODE_FOR_sinsf2 = 573,
  CODE_FOR_sinxf2 = 575,
  CODE_FOR_sintf2 = 576,
  CODE_FOR_cosdf2 = 577,
  CODE_FOR_cossf2 = 578,
  CODE_FOR_cosxf2 = 580,
  CODE_FOR_costf2 = 581,
  CODE_FOR_cld = 582,
  CODE_FOR_strmovdi_rex_1 = 583,
  CODE_FOR_strmovsi_1 = 584,
  CODE_FOR_strmovsi_rex_1 = 585,
  CODE_FOR_strmovhi_1 = 586,
  CODE_FOR_strmovhi_rex_1 = 587,
  CODE_FOR_strmovqi_1 = 588,
  CODE_FOR_strmovqi_rex_1 = 589,
  CODE_FOR_rep_movdi_rex64 = 590,
  CODE_FOR_rep_movsi = 591,
  CODE_FOR_rep_movsi_rex64 = 592,
  CODE_FOR_rep_movqi = 593,
  CODE_FOR_rep_movqi_rex64 = 594,
  CODE_FOR_strsetdi_rex_1 = 595,
  CODE_FOR_strsetsi_1 = 596,
  CODE_FOR_strsetsi_rex_1 = 597,
  CODE_FOR_strsethi_1 = 598,
  CODE_FOR_strsethi_rex_1 = 599,
  CODE_FOR_strsetqi_1 = 600,
  CODE_FOR_strsetqi_rex_1 = 601,
  CODE_FOR_rep_stosdi_rex64 = 602,
  CODE_FOR_rep_stossi = 603,
  CODE_FOR_rep_stossi_rex64 = 604,
  CODE_FOR_rep_stosqi = 605,
  CODE_FOR_rep_stosqi_rex64 = 606,
  CODE_FOR_cmpstrqi_nz_1 = 607,
  CODE_FOR_cmpstrqi_nz_rex_1 = 608,
  CODE_FOR_cmpstrqi_1 = 609,
  CODE_FOR_cmpstrqi_rex_1 = 610,
  CODE_FOR_strlenqi_1 = 611,
  CODE_FOR_strlenqi_rex_1 = 612,
  CODE_FOR_x86_movdicc_0_m1_rex64 = 613,
  CODE_FOR_x86_movsicc_0_m1 = 615,
  CODE_FOR_pro_epilogue_adjust_stack_rex64 = 636,
  CODE_FOR_sse_movsfcc = 637,
  CODE_FOR_sse_movsfcc_eq = 638,
  CODE_FOR_sse_movdfcc = 639,
  CODE_FOR_sse_movdfcc_eq = 640,
  CODE_FOR_allocate_stack_worker_1 = 649,
  CODE_FOR_allocate_stack_worker_rex64 = 650,
  CODE_FOR_trap = 657,
  CODE_FOR_movv4sf_internal = 659,
  CODE_FOR_movv4si_internal = 660,
  CODE_FOR_movv8qi_internal = 661,
  CODE_FOR_movv4hi_internal = 662,
  CODE_FOR_movv2si_internal = 663,
  CODE_FOR_movv2sf_internal = 664,
  CODE_FOR_movti_internal = 672,
  CODE_FOR_sse_movaps = 674,
  CODE_FOR_sse_movups = 675,
  CODE_FOR_sse_movmskps = 676,
  CODE_FOR_mmx_pmovmskb = 677,
  CODE_FOR_mmx_maskmovq = 678,
  CODE_FOR_mmx_maskmovq_rex = 679,
  CODE_FOR_sse_movntv4sf = 680,
  CODE_FOR_sse_movntdi = 681,
  CODE_FOR_sse_movhlps = 682,
  CODE_FOR_sse_movlhps = 683,
  CODE_FOR_sse_movhps = 684,
  CODE_FOR_sse_movlps = 685,
  CODE_FOR_sse_loadss = 686,
  CODE_FOR_sse_movss = 687,
  CODE_FOR_sse_storess = 688,
  CODE_FOR_sse_shufps = 689,
  CODE_FOR_addv4sf3 = 690,
  CODE_FOR_vmaddv4sf3 = 691,
  CODE_FOR_subv4sf3 = 692,
  CODE_FOR_vmsubv4sf3 = 693,
  CODE_FOR_mulv4sf3 = 694,
  CODE_FOR_vmmulv4sf3 = 695,
  CODE_FOR_divv4sf3 = 696,
  CODE_FOR_vmdivv4sf3 = 697,
  CODE_FOR_rcpv4sf2 = 698,
  CODE_FOR_vmrcpv4sf2 = 699,
  CODE_FOR_rsqrtv4sf2 = 700,
  CODE_FOR_vmrsqrtv4sf2 = 701,
  CODE_FOR_sqrtv4sf2 = 702,
  CODE_FOR_vmsqrtv4sf2 = 703,
  CODE_FOR_sse_andti3 = 708,
  CODE_FOR_sse_nandti3 = 712,
  CODE_FOR_sse_iorti3 = 718,
  CODE_FOR_sse_xorti3 = 724,
  CODE_FOR_sse_clrv4sf = 726,
  CODE_FOR_maskcmpv4sf3 = 727,
  CODE_FOR_maskncmpv4sf3 = 728,
  CODE_FOR_vmmaskcmpv4sf3 = 729,
  CODE_FOR_vmmaskncmpv4sf3 = 730,
  CODE_FOR_sse_comi = 731,
  CODE_FOR_sse_ucomi = 732,
  CODE_FOR_sse_unpckhps = 733,
  CODE_FOR_sse_unpcklps = 734,
  CODE_FOR_smaxv4sf3 = 735,
  CODE_FOR_vmsmaxv4sf3 = 736,
  CODE_FOR_sminv4sf3 = 737,
  CODE_FOR_vmsminv4sf3 = 738,
  CODE_FOR_cvtpi2ps = 739,
  CODE_FOR_cvtps2pi = 740,
  CODE_FOR_cvttps2pi = 741,
  CODE_FOR_cvtsi2ss = 742,
  CODE_FOR_cvtss2si = 743,
  CODE_FOR_cvttss2si = 744,
  CODE_FOR_addv8qi3 = 745,
  CODE_FOR_addv4hi3 = 746,
  CODE_FOR_addv2si3 = 747,
  CODE_FOR_ssaddv8qi3 = 748,
  CODE_FOR_ssaddv4hi3 = 749,
  CODE_FOR_usaddv8qi3 = 750,
  CODE_FOR_usaddv4hi3 = 751,
  CODE_FOR_subv8qi3 = 752,
  CODE_FOR_subv4hi3 = 753,
  CODE_FOR_subv2si3 = 754,
  CODE_FOR_sssubv8qi3 = 755,
  CODE_FOR_sssubv4hi3 = 756,
  CODE_FOR_ussubv8qi3 = 757,
  CODE_FOR_ussubv4hi3 = 758,
  CODE_FOR_mulv4hi3 = 759,
  CODE_FOR_smulv4hi3_highpart = 760,
  CODE_FOR_umulv4hi3_highpart = 761,
  CODE_FOR_mmx_pmaddwd = 762,
  CODE_FOR_mmx_iordi3 = 763,
  CODE_FOR_mmx_xordi3 = 764,
  CODE_FOR_mmx_clrdi = 765,
  CODE_FOR_mmx_anddi3 = 766,
  CODE_FOR_mmx_nanddi3 = 767,
  CODE_FOR_mmx_uavgv8qi3 = 768,
  CODE_FOR_mmx_uavgv4hi3 = 769,
  CODE_FOR_mmx_psadbw = 770,
  CODE_FOR_mmx_pinsrw = 771,
  CODE_FOR_mmx_pextrw = 772,
  CODE_FOR_mmx_pshufw = 773,
  CODE_FOR_eqv8qi3 = 774,
  CODE_FOR_eqv4hi3 = 775,
  CODE_FOR_eqv2si3 = 776,
  CODE_FOR_gtv8qi3 = 777,
  CODE_FOR_gtv4hi3 = 778,
  CODE_FOR_gtv2si3 = 779,
  CODE_FOR_umaxv8qi3 = 780,
  CODE_FOR_smaxv4hi3 = 781,
  CODE_FOR_uminv8qi3 = 782,
  CODE_FOR_sminv4hi3 = 783,
  CODE_FOR_ashrv4hi3 = 784,
  CODE_FOR_ashrv2si3 = 785,
  CODE_FOR_lshrv4hi3 = 786,
  CODE_FOR_lshrv2si3 = 787,
  CODE_FOR_mmx_lshrdi3 = 788,
  CODE_FOR_ashlv4hi3 = 789,
  CODE_FOR_ashlv2si3 = 790,
  CODE_FOR_mmx_ashldi3 = 791,
  CODE_FOR_mmx_packsswb = 792,
  CODE_FOR_mmx_packssdw = 793,
  CODE_FOR_mmx_packuswb = 794,
  CODE_FOR_mmx_punpckhbw = 795,
  CODE_FOR_mmx_punpckhwd = 796,
  CODE_FOR_mmx_punpckhdq = 797,
  CODE_FOR_mmx_punpcklbw = 798,
  CODE_FOR_mmx_punpcklwd = 799,
  CODE_FOR_mmx_punpckldq = 800,
  CODE_FOR_emms = 801,
  CODE_FOR_ldmxcsr = 802,
  CODE_FOR_stmxcsr = 803,
  CODE_FOR_addv2sf3 = 806,
  CODE_FOR_subv2sf3 = 807,
  CODE_FOR_subrv2sf3 = 808,
  CODE_FOR_gtv2sf3 = 809,
  CODE_FOR_gev2sf3 = 810,
  CODE_FOR_eqv2sf3 = 811,
  CODE_FOR_pfmaxv2sf3 = 812,
  CODE_FOR_pfminv2sf3 = 813,
  CODE_FOR_mulv2sf3 = 814,
  CODE_FOR_femms = 815,
  CODE_FOR_pf2id = 816,
  CODE_FOR_pf2iw = 817,
  CODE_FOR_pfacc = 818,
  CODE_FOR_pfnacc = 819,
  CODE_FOR_pfpnacc = 820,
  CODE_FOR_pi2fw = 821,
  CODE_FOR_floatv2si2 = 822,
  CODE_FOR_pavgusb = 823,
  CODE_FOR_pfrcpv2sf2 = 824,
  CODE_FOR_pfrcpit1v2sf3 = 825,
  CODE_FOR_pfrcpit2v2sf3 = 826,
  CODE_FOR_pfrsqrtv2sf2 = 827,
  CODE_FOR_pfrsqit1v2sf3 = 828,
  CODE_FOR_pmulhrwv4hi3 = 829,
  CODE_FOR_pswapdv2si2 = 830,
  CODE_FOR_pswapdv2sf2 = 831,
  CODE_FOR_cmpdi = 834,
  CODE_FOR_cmpsi = 835,
  CODE_FOR_cmphi = 836,
  CODE_FOR_cmpqi = 837,
  CODE_FOR_cmpdi_1_rex64 = 838,
  CODE_FOR_cmpsi_1 = 839,
  CODE_FOR_cmpqi_ext_3 = 840,
  CODE_FOR_cmpxf = 841,
  CODE_FOR_cmptf = 842,
  CODE_FOR_cmpdf = 843,
  CODE_FOR_cmpsf = 844,
  CODE_FOR_movsi = 846,
  CODE_FOR_movhi = 847,
  CODE_FOR_movstricthi = 848,
  CODE_FOR_movqi = 849,
  CODE_FOR_reload_outqi = 850,
  CODE_FOR_movstrictqi = 851,
  CODE_FOR_movdi = 852,
  CODE_FOR_movsf = 861,
  CODE_FOR_movdf = 865,
  CODE_FOR_movxf = 870,
  CODE_FOR_movtf = 871,
  CODE_FOR_zero_extendhisi2 = 878,
  CODE_FOR_zero_extendqihi2 = 880,
  CODE_FOR_zero_extendqisi2 = 884,
  CODE_FOR_zero_extendsidi2 = 888,
  CODE_FOR_extendsidi2 = 892,
  CODE_FOR_extendsfdf2 = 904,
  CODE_FOR_extendsfxf2 = 905,
  CODE_FOR_extendsftf2 = 906,
  CODE_FOR_extenddfxf2 = 907,
  CODE_FOR_extenddftf2 = 908,
  CODE_FOR_truncdfsf2 = 909,
  CODE_FOR_truncxfsf2 = 913,
  CODE_FOR_trunctfsf2 = 916,
  CODE_FOR_truncxfdf2 = 919,
  CODE_FOR_trunctfdf2 = 922,
  CODE_FOR_fix_truncxfdi2 = 925,
  CODE_FOR_fix_trunctfdi2 = 926,
  CODE_FOR_fix_truncdfdi2 = 927,
  CODE_FOR_fix_truncsfdi2 = 928,
  CODE_FOR_fix_truncxfsi2 = 932,
  CODE_FOR_fix_trunctfsi2 = 933,
  CODE_FOR_fix_truncdfsi2 = 934,
  CODE_FOR_fix_truncsfsi2 = 935,
  CODE_FOR_fix_truncxfhi2 = 939,
  CODE_FOR_fix_trunctfhi2 = 940,
  CODE_FOR_fix_truncdfhi2 = 941,
  CODE_FOR_fix_truncsfhi2 = 942,
  CODE_FOR_floatsisf2 = 946,
  CODE_FOR_floatdisf2 = 947,
  CODE_FOR_floatsidf2 = 948,
  CODE_FOR_floatdidf2 = 949,
  CODE_FOR_adddi3 = 951,
  CODE_FOR_addsi3 = 953,
  CODE_FOR_addhi3 = 963,
  CODE_FOR_addqi3 = 964,
  CODE_FOR_addxf3 = 965,
  CODE_FOR_addtf3 = 966,
  CODE_FOR_adddf3 = 967,
  CODE_FOR_addsf3 = 968,
  CODE_FOR_subdi3 = 969,
  CODE_FOR_subsi3 = 971,
  CODE_FOR_subhi3 = 972,
  CODE_FOR_subqi3 = 973,
  CODE_FOR_subxf3 = 974,
  CODE_FOR_subtf3 = 975,
  CODE_FOR_subdf3 = 976,
  CODE_FOR_subsf3 = 977,
  CODE_FOR_muldi3 = 978,
  CODE_FOR_mulsi3 = 979,
  CODE_FOR_mulhi3 = 980,
  CODE_FOR_mulqi3 = 981,
  CODE_FOR_umulqihi3 = 982,
  CODE_FOR_mulqihi3 = 983,
  CODE_FOR_umulditi3 = 984,
  CODE_FOR_umulsidi3 = 985,
  CODE_FOR_mulditi3 = 986,
  CODE_FOR_mulsidi3 = 987,
  CODE_FOR_umuldi3_highpart = 988,
  CODE_FOR_umulsi3_highpart = 989,
  CODE_FOR_smuldi3_highpart = 990,
  CODE_FOR_smulsi3_highpart = 991,
  CODE_FOR_mulxf3 = 992,
  CODE_FOR_multf3 = 993,
  CODE_FOR_muldf3 = 994,
  CODE_FOR_mulsf3 = 995,
  CODE_FOR_divxf3 = 996,
  CODE_FOR_divtf3 = 997,
  CODE_FOR_divdf3 = 998,
  CODE_FOR_divsf3 = 999,
  CODE_FOR_divmoddi4 = 1000,
  CODE_FOR_divmodsi4 = 1002,
  CODE_FOR_udivmodhi4 = 1006,
  CODE_FOR_testsi_ccno_1 = 1007,
  CODE_FOR_testqi_ccz_1 = 1008,
  CODE_FOR_testqi_ext_ccno_0 = 1009,
  CODE_FOR_anddi3 = 1011,
  CODE_FOR_andsi3 = 1012,
  CODE_FOR_andhi3 = 1016,
  CODE_FOR_andqi3 = 1017,
  CODE_FOR_iordi3 = 1018,
  CODE_FOR_iorsi3 = 1019,
  CODE_FOR_iorhi3 = 1020,
  CODE_FOR_iorqi3 = 1021,
  COD