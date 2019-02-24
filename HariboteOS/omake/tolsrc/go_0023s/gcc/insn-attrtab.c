/* Generated automatically by the program `genattrtab'
from the machine description file `md'.  */

#include "config.h"
#include "system.h"
#include "rtl.h"
#include "tm_p.h"
#include "insn-config.h"
#include "recog.h"
#include "regs.h"
#include "real.h"
#include "output.h"
#include "insn-attr.h"
#include "toplev.h"
#include "flags.h"

#define operands recog_data.operand

extern int insn_current_length PARAMS ((rtx));
int
insn_current_length (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return 0;

    }
}

extern int insn_variable_length_p PARAMS ((rtx));
int
insn_variable_length_p (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
    default:
      return 0;

    }
}

extern int insn_default_length PARAMS ((rtx));
int
insn_default_length (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 619:
      extract_constrain_insn_cached (insn);
      if ((which_alternative == 2) || (which_alternative == 3))
        {
	  return 16 /* 0x10 */;
        }
      else
        {
	  return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);
        }

    case 507:
      if (get_attr_type (insn) == TYPE_MULTI)
        {
	  return 16 /* 0x10 */;
        }
      else
        {
	  return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);
        }

    case 484:
    case 482:
    case 474:
    case 472:
    case 462:
    case 458:
    case 442:
    case 440:
    case 438:
    case 436:
      extract_insn_cached (insn);
      if (register_operand (operands[0], VOIDmode))
        {
	  return 2;
        }
      else
        {
	  return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);
        }

    case 479:
    case 478:
    case 468:
    case 464:
    case 460:
    case 454:
    case 450:
    case 432:
    case 428:
      extract_insn_cached (insn);
      if (register_operand (operands[0], SImode))
        {
	  return 2;
        }
      else
        {
	  return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);
        }

    case 476:
    case 466:
    case 446:
    case 444:
    case 421:
    case 419:
      extract_insn_cached (insn);
      if (register_operand (operands[0], DImode))
        {
	  return 2;
        }
      else
        {
	  return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);
        }

    case 177:
    case 174:
    case 171:
    case 168:
      extract_constrain_insn_cached (insn);
      if (which_alternative == 1)
        {
	  return 16 /* 0x10 */;
        }
      else
        {
	  return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);
        }

    case 140:
      extract_constrain_insn_cached (insn);
      if ((which_alternative == 1) || ((which_alternative == 2) || (which_alternative == 3)))
        {
	  return 16 /* 0x10 */;
        }
      else
        {
	  return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);
        }

    case 104:
    case 103:
    case 102:
    case 101:
      extract_constrain_insn_cached (insn);
      if ((which_alternative == 3) || (which_alternative == 4))
        {
	  return 16 /* 0x10 */;
        }
      else
        {
	  return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);
        }

    case 95:
    case 94:
      extract_constrain_insn_cached (insn);
      if (((which_alternative != 0) && ((which_alternative != 1) && (which_alternative != 2))) && ((which_alternative == 3) || (which_alternative == 4)))
        {
	  return 16 /* 0x10 */;
        }
      else
        {
	  return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);
        }

    case 89:
    case 88:
      extract_constrain_insn_cached (insn);
      if (which_alternative != 1)
        {
	  return 16 /* 0x10 */;
        }
      else
        {
	  return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);
        }

    case 84:
      extract_constrain_insn_cached (insn);
      if (which_alternative == 4)
        {
	  return 16 /* 0x10 */;
        }
      else
        {
	  return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);
        }

    case 673:
    case 83:
      extract_constrain_insn_cached (insn);
      if ((which_alternative == 0) || (which_alternative == 1))
        {
	  return 16 /* 0x10 */;
        }
      else
        {
	  return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);
        }

    case 184:
    case 183:
    case 182:
    case 181:
    case 180:
    case 179:
    case 176:
    case 173:
    case 170:
    case 167:
    case 150:
    case 148:
    case 146:
    case 144:
    case 139:
    case 77:
      extract_constrain_insn_cached (insn);
      if (which_alternative != 0)
        {
	  return 16 /* 0x10 */;
        }
      else
        {
	  return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);
        }

    case 658:
    case 657:
    case 648:
    case 647:
    case 646:
    case 645:
    case 644:
    case 643:
    case 642:
    case 641:
    case 640:
    case 639:
    case 638:
    case 637:
    case 633:
    case 632:
    case 630:
    case 629:
    case 627:
    case 626:
    case 624:
    case 623:
    case 524:
    case 521:
    case 520:
    case 519:
    case 501:
    case 500:
    case 499:
    case 498:
    case 497:
    case 496:
    case 495:
    case 494:
    case 493:
    case 492:
    case 449:
    case 448:
    case 424:
    case 423:
    case 406:
    case 405:
    case 383:
    case 382:
    case 381:
    case 380:
    case 379:
    case 378:
    case 377:
    case 376:
    case 375:
    case 374:
    case 364:
    case 363:
    case 362:
    case 361:
    case 360:
    case 359:
    case 358:
    case 357:
    case 356:
    case 355:
    case 344:
    case 289:
    case 288:
    case 277:
    case 275:
    case 274:
    case 272:
    case 271:
    case 269:
    case 268:
    case 230:
    case 185:
    case 164:
    case 163:
    case 162:
    case 159:
    case 158:
    case 157:
    case 154:
    case 153:
    case 152:
    case 132:
    case 131:
    case 130:
    case 129:
    case 128:
    case 119:
    case 115:
    case 100:
    case 99:
    case 98:
    case 97:
    case 93:
    case 92:
    case 76:
    case 29:
    case 28:
    case 26:
    case 25:
    case 22:
    case 18:
      return 16 /* 0x10 */;

    case -1:
      if (GET_CODE (PATTERN (insn)) != ASM_INPUT
          && asm_noperands (PATTERN (insn)) < 0)
        fatal_insn_not_found (insn);
      return 128 /* 0x80 */;

    case 805:
      return 135 /* 0x87 */;

    case 650:
    case 649:
      return 5;

    case 523:
    case 522:
    case 517:
    case 514:
    case 31:
      return 1;

    case 515:
      return 3;

    case 513:
      return 0;

    case 469:
    case 455:
    case 451:
    case 433:
    case 429:
    case 166:
    case 165:
    case 30:
      return 2;

    default:
      return get_attr_modrm (insn) + get_attr_prefix_0f (insn) + get_attr_i387 (insn) + 1 + get_attr_prefix_rep (insn) + get_attr_prefix_data16 (insn) + get_attr_length_immediate (insn) + get_attr_length_address (insn);

    }
}

extern int result_ready_cost PARAMS ((rtx));
int
result_ready_cost (insn)
     rtx insn;
{
  switch (recog_memoized (insn))
    {
    case 656:
    case 655:
    case 654:
    case 653:
    case 652:
    case 651:
      extract_insn_cached (insn);
      if ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (! (constant_call_address_operand (operands[1], VOIDmode))))
        {
	  return 3;
        }
      else
        {
	  return 1;
        }

    case 636:
      extract_constrain_insn_cached (insn);
      if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && ((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((which_alternative == 0) && (((get_attr_imm_disp (insn) == IMM_DISP_TRUE) && (get_attr_memory (insn) == MEMORY_BOTH)) || ((! (get_attr_imm_disp (insn) == IMM_DISP_TRUE)) && (get_attr_memory (insn) == MEMORY_BOTH))))))
        {
	  return 3;
        }
      else if (((((ix86_cpu) == (CPU_K6))) && ((which_alternative == 1) && (! (const0_operand (operands[2], DImode))))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((which_alternative == 0) && (((get_attr_imm_disp (insn) == IMM_DISP_TRUE) && ((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_STORE))) || ((! (get_attr_imm_disp (insn) == IMM_DISP_TRUE)) && ((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_STORE)))))))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 635:
      extract_constrain_insn_cached (insn);
      if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && ((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((which_alternative == 0) && (((get_attr_imm_disp (insn) == IMM_DISP_TRUE) && (get_attr_memory (insn) == MEMORY_BOTH)) || ((! (get_attr_imm_disp (insn) == IMM_DISP_TRUE)) && (get_attr_memory (insn) == MEMORY_BOTH))))))
        {
	  return 3;
        }
      else if (((((ix86_cpu) == (CPU_K6))) && ((which_alternative == 1) && (! (const0_operand (operands[2], SImode))))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((which_alternative == 0) && (((get_attr_imm_disp (insn) == IMM_DISP_TRUE) && ((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_STORE))) || ((! (get_attr_imm_disp (insn) == IMM_DISP_TRUE)) && ((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_STORE)))))))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 622:
    case 621:
      if (((ix86_cpu) == (CPU_ATHLON)))
        {
	  return 7;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUMPRO))) || (((ix86_cpu) == (CPU_PENTIUM))))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 619:
      extract_constrain_insn_cached (insn);
      if ((((ix86_cpu) == (CPU_ATHLON))) && ((which_alternative == 0) || (which_alternative == 1)))
        {
	  return 7;
        }
      else if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && ((which_alternative == 0) || (which_alternative == 1))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((which_alternative == 0) || (which_alternative == 1))))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 620:
    case 618:
      extract_constrain_insn_cached (insn);
      if ((((ix86_cpu) == (CPU_ATHLON))) && ((which_alternative == 0) || (which_alternative == 1)))
        {
	  return 7;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUMPRO))) && ((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH)))
        {
	  return 3;
        }
      else if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && ((which_alternative == 0) || (which_alternative == 1))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((which_alternative == 0) || (which_alternative == 1))))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 612:
    case 611:
    case 610:
    case 609:
    case 608:
    case 607:
    case 606:
    case 605:
    case 604:
    case 603:
    case 602:
    case 601:
    case 600:
    case 599:
    case 598:
    case 597:
    case 596:
    case 595:
      if (((ix86_cpu) == (CPU_ATHLON)))
        {
	  return 15 /* 0xf */;
        }
      else if (((ix86_cpu) == (CPU_PENTIUM)))
        {
	  return 12 /* 0xc */;
        }
      else if (((ix86_cpu) == (CPU_K6)))
        {
	  return 10 /* 0xa */;
        }
      else
        {
	  return 1;
        }

    case 594:
    case 593:
    case 592:
    case 591:
    case 590:
    case 589:
    case 588:
    case 587:
    case 586:
    case 585:
    case 584:
    case 583:
      if (((ix86_cpu) == (CPU_ATHLON)))
        {
	  return 15 /* 0xf */;
        }
      else if (((ix86_cpu) == (CPU_PENTIUM)))
        {
	  return 12 /* 0xc */;
        }
      else if (((ix86_cpu) == (CPU_K6)))
        {
	  return 10 /* 0xa */;
        }
      else if (((ix86_cpu) == (CPU_PENTIUMPRO)))
        {
	  return 3;
        }
      else
        {
	  return 1;
        }

    case 582:
      if (((ix86_cpu) == (CPU_PENTIUM)))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 581:
    case 580:
    case 579:
    case 578:
    case 577:
    case 576:
    case 575:
    case 574:
    case 573:
    case 572:
    case 571:
    case 570:
    case 569:
    case 568:
    case 567:
    case 566:
    case 565:
    case 564:
    case 561:
      if (((ix86_cpu) == (CPU_ATHLON)))
        {
	  return 100 /* 0x64 */;
        }
      else if (((ix86_cpu) == (CPU_PENTIUM)))
        {
	  return 70 /* 0x46 */;
        }
      else if ((((ix86_cpu) == (CPU_K6))) || (((ix86_cpu) == (CPU_PENTIUMPRO))))
        {
	  return 56 /* 0x38 */;
        }
      else
        {
	  return 1;
        }

    case 562:
    case 559:
      extract_constrain_insn_cached (insn);
      if ((((ix86_cpu) == (CPU_ATHLON))) && (which_alternative == 0))
        {
	  return 100 /* 0x64 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUM))) && (which_alternative == 0))
        {
	  return 70 /* 0x46 */;
        }
      else if (((((ix86_cpu) == (CPU_K6))) && (which_alternative == 0)) || ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (which_alternative == 0)))
        {
	  return 56 /* 0x38 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUMPRO))) && ((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH)))
        {
	  return 3;
        }
      else
        {
	  return 1;
        }

    case 558:
    case 556:
    case 554:
    case 552:
    case 550:
    case 548:
    case 546:
      extract_insn_cached (insn);
      if (((((ix86_cpu) == (CPU_K6))) && (get_attr_type (insn) == TYPE_FDIV)) || ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (get_attr_type (insn) == TYPE_FDIV)))
        {
	  return 56 /* 0x38 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUM))) && (get_attr_type (insn) == TYPE_FDIV))
        {
	  return 39 /* 0x27 */;
        }
      else if ((((ix86_cpu) == (CPU_ATHLON))) && (get_attr_type (insn) == TYPE_FDIV))
        {
	  return 24 /* 0x18 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (mult_operator (operands[3], TFmode)))
        {
	  return 5;
        }
      else if ((((ix86_cpu) == (CPU_ATHLON))) && ((get_attr_type (insn) == TYPE_FOP) || (mult_operator (operands[3], TFmode))))
        {
	  return 4;
        }
      else if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && (((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH)) || (get_attr_type (insn) == TYPE_FOP))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((get_attr_type (insn) == TYPE_FOP) || (mult_operator (operands[3], TFmode)))))
        {
	  return 3;
        }
      else if (((((ix86_cpu) == (CPU_K6))) && ((get_attr_type (insn) == TYPE_FOP) || (mult_operator (operands[3], TFmode)))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((mult_operator (operands[3], TFmode)) || (get_attr_type (insn) == TYPE_FOP))))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 557:
    case 555:
    case 553:
    case 551:
    case 549:
    case 547:
    case 545:
      extract_insn_cached (insn);
      if (((((ix86_cpu) == (CPU_K6))) && (get_attr_type (insn) == TYPE_FDIV)) || ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (get_attr_type (insn) == TYPE_FDIV)))
        {
	  return 56 /* 0x38 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUM))) && (get_attr_type (insn) == TYPE_FDIV))
        {
	  return 39 /* 0x27 */;
        }
      else if ((((ix86_cpu) == (CPU_ATHLON))) && (get_attr_type (insn) == TYPE_FDIV))
        {
	  return 24 /* 0x18 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (mult_operator (operands[3], XFmode)))
        {
	  return 5;
        }
      else if ((((ix86_cpu) == (CPU_ATHLON))) && ((get_attr_type (insn) == TYPE_FOP) || (mult_operator (operands[3], XFmode))))
        {
	  return 4;
        }
      else if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && (((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH)) || (get_attr_type (insn) == TYPE_FOP))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((get_attr_type (insn) == TYPE_FOP) || (mult_operator (operands[3], XFmode)))))
        {
	  return 3;
        }
      else if (((((ix86_cpu) == (CPU_K6))) && ((get_attr_type (insn) == TYPE_FOP) || (mult_operator (operands[3], XFmode)))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((mult_operator (operands[3], XFmode)) || (get_attr_type (insn) == TYPE_FOP))))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 539:
      extract_constrain_insn_cached (insn);
      if (((((ix86_cpu) == (CPU_K6))) && (get_attr_type (insn) == TYPE_FDIV)) || ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (get_attr_type (insn) == TYPE_FDIV)))
        {
	  return 56 /* 0x38 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUM))) && (get_attr_type (insn) == TYPE_FDIV))
        {
	  return 39 /* 0x27 */;
        }
      else if ((((ix86_cpu) == (CPU_ATHLON))) && (get_attr_type (insn) == TYPE_FDIV))
        {
	  return 24 /* 0x18 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUMPRO))) && ((which_alternative != 2) && (mult_operator (operands[3], DFmode))))
        {
	  return 5;
        }
      else if ((((ix86_cpu) == (CPU_ATHLON))) && ((get_attr_type (insn) == TYPE_FOP) || ((which_alternative != 2) && (mult_operator (operands[3], DFmode)))))
        {
	  return 4;
        }
      else if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && (((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH)) || (get_attr_type (insn) == TYPE_FOP))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((get_attr_type (insn) == TYPE_FOP) || ((which_alternative != 2) && (mult_operator (operands[3], DFmode))))))
        {
	  return 3;
        }
      else if (((((ix86_cpu) == (CPU_K6))) && ((get_attr_type (insn) == TYPE_FOP) || ((which_alternative != 2) && (mult_operator (operands[3], DFmode))))) || ((((ix86_cpu) == (CPU_PENTIUM))) && (((which_alternative != 2) && (mult_operator (operands[3], DFmode))) || (get_attr_type (insn) == TYPE_FOP))))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 544:
    case 543:
    case 542:
    case 541:
    case 538:
      extract_insn_cached (insn);
      if (((((ix86_cpu) == (CPU_K6))) && (get_attr_type (insn) == TYPE_FDIV)) || ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (get_attr_type (insn) == TYPE_FDIV)))
        {
	  return 56 /* 0x38 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUM))) && (get_attr_type (insn) == TYPE_FDIV))
        {
	  return 39 /* 0x27 */;
        }
      else if ((((ix86_cpu) == (CPU_ATHLON))) && (get_attr_type (insn) == TYPE_FDIV))
        {
	  return 24 /* 0x18 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (mult_operator (operands[3], DFmode)))
        {
	  return 5;
        }
      else if ((((ix86_cpu) == (CPU_ATHLON))) && ((get_attr_type (insn) == TYPE_FOP) || (mult_operator (operands[3], DFmode))))
        {
	  return 4;
        }
      else if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && (((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH)) || (get_attr_type (insn) == TYPE_FOP))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((get_attr_type (insn) == TYPE_FOP) || (mult_operator (operands[3], DFmode)))))
        {
	  return 3;
        }
      else if (((((ix86_cpu) == (CPU_K6))) && ((get_attr_type (insn) == TYPE_FOP) || (mult_operator (operands[3], DFmode)))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((mult_operator (operands[3], DFmode)) || (get_attr_type (insn) == TYPE_FOP))))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 534:
      extract_constrain_insn_cached (insn);
      if (((((ix86_cpu) == (CPU_K6))) && (get_attr_type (insn) == TYPE_FDIV)) || ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (get_attr_type (insn) == TYPE_FDIV)))
        {
	  return 56 /* 0x38 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUM))) && (get_attr_type (insn) == TYPE_FDIV))
        {
	  return 39 /* 0x27 */;
        }
      else if ((((ix86_cpu) == (CPU_ATHLON))) && (get_attr_type (insn) == TYPE_FDIV))
        {
	  return 24 /* 0x18 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUMPRO))) && ((which_alternative != 2) && (mult_operator (operands[3], SFmode))))
        {
	  return 5;
        }
      else if ((((ix86_cpu) == (CPU_ATHLON))) && ((get_attr_type (insn) == TYPE_FOP) || ((which_alternative != 2) && (mult_operator (operands[3], SFmode)))))
        {
	  return 4;
        }
      else if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && (((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH)) || (get_attr_type (insn) == TYPE_FOP))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((get_attr_type (insn) == TYPE_FOP) || ((which_alternative != 2) && (mult_operator (operands[3], SFmode))))))
        {
	  return 3;
        }
      else if (((((ix86_cpu) == (CPU_K6))) && ((get_attr_type (insn) == TYPE_FOP) || ((which_alternative != 2) && (mult_operator (operands[3], SFmode))))) || ((((ix86_cpu) == (CPU_PENTIUM))) && (((which_alternative != 2) && (mult_operator (operands[3], SFmode))) || (get_attr_type (insn) == TYPE_FOP))))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 537:
    case 536:
    case 533:
      extract_insn_cached (insn);
      if (((((ix86_cpu) == (CPU_K6))) && (get_attr_type (insn) == TYPE_FDIV)) || ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (get_attr_type (insn) == TYPE_FDIV)))
        {
	  return 56 /* 0x38 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUM))) && (get_attr_type (insn) == TYPE_FDIV))
        {
	  return 39 /* 0x27 */;
        }
      else if ((((ix86_cpu) == (CPU_ATHLON))) && (get_attr_type (insn) == TYPE_FDIV))
        {
	  return 24 /* 0x18 */;
        }
      else if ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (mult_operator (operands[3], SFmode)))
        {
	  return 5;
        }
      else if ((((ix86_cpu) == (CPU_ATHLON))) && ((get_attr_type (insn) == TYPE_FOP) || (mult_operator (operands[3], SFmode))))
        {
	  return 4;
        }
      else if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && (((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH)) || (get_attr_type (insn) == TYPE_FOP))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((get_attr_type (insn) == TYPE_FOP) || (mult_operator (operands[3], SFmode)))))
        {
	  return 3;
        }
      else if (((((ix86_cpu) == (CPU_K6))) && ((get_attr_type (insn) == TYPE_FOP) || (mult_operator (operands[3], SFmode)))) || ((((ix86_cpu) == (CPU_PENTIUM))) && ((mult_operator (operands[3], SFmode)) || (get_attr_type (insn) == TYPE_FOP))))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 532:
      extract_insn_cached (insn);
      if ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (mult_operator (operands[3], TFmode)))
        {
	  return 5;
        }
      else if (((ix86_cpu) == (CPU_ATHLON)))
        {
	  return 4;
        }
      else if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && (((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH)) || (! (mult_operator (operands[3], TFmode))))) || (((ix86_cpu) == (CPU_PENTIUM))))
        {
	  return 3;
        }
      else if (((ix86_cpu) == (CPU_K6)))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 531:
      extract_insn_cached (insn);
      if ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (mult_operator (operands[3], XFmode)))
        {
	  return 5;
        }
      else if (((ix86_cpu) == (CPU_ATHLON)))
        {
	  return 4;
        }
      else if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && (((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH)) || (! (mult_operator (operands[3], XFmode))))) || (((ix86_cpu) == (CPU_PENTIUM))))
        {
	  return 3;
        }
      else if (((ix86_cpu) == (CPU_K6)))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 529:
    case 526:
      extract_constrain_insn_cached (insn);
      if ((((ix86_cpu) == (CPU_PENTIUMPRO))) && ((which_alternative == 0) && (mult_operator (operands[3], SFmode))))
        {
	  return 5;
        }
      else if ((((ix86_cpu) == (CPU_ATHLON))) && (which_alternative == 0))
        {
	  return 4;
        }
      else if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && (((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH)) || ((which_alternative == 0) && (! (mult_operator (operands[3], SFmode)))))) || ((((ix86_cpu) == (CPU_PENTIUM))) && (which_alternative == 0)))
        {
	  return 3;
        }
      else if (((((ix86_cpu) == (CPU_K6))) && (which_alternative == 0)) || ((((ix86_cpu) == (CPU_PENTIUM))) && (which_alternative == 0)))
        {
	  return 2;
        }
      else
        {
	  return 1;
        }

    case 528:
    case 525:
      extract_insn_cached (insn);
      if ((((ix86_cpu) == (CPU_PENTIUMPRO))) && (mult_operator (operands[3], SFmode)))
        {
	  return 5;
        }
      else if (((ix86_cpu) == (CPU_ATHLON)))
        {
	  return 4;
        }
      else if (((((ix86_cpu) == (CPU_PENTIUMPRO))) && (((get_attr_memory (insn) == MEMORY_LOAD) || (get_attr_memory (insn) == MEMORY_BOTH)) || (! (mult_operator (operands[3], SFmode))))) || (((ix86_cpu) == (CPU_PENTIUM))))
        {
	  return 3;
        }
      else if (((ix86_cpu) == (CPU_K6)))
        {
	  retu