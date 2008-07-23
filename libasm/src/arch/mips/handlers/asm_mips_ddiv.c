/* Adam 'pi3' Zabrocki */
/* Manuel Martin - 2007 */

#include <libasm.h>

int asm_mips_ddiv(asm_instr *ins, u_char *buf, u_int len,
                  asm_processor *proc)
{
   struct s_mips_decode_trap temp;

   ins->instr = ASM_MIPS_DDIV;
   ins->type = ASM_TYPE_ARITH;
   mips_convert_format_t(&temp,buf);
   ins->op[0].baser = temp.rs;
   asm_mips_operand_fetch(&ins->op[0], buf, ASM_MIPS_OTYPE_REGISTER, ins);
   ins->op[1].baser = temp.rt;
   asm_mips_operand_fetch(&ins->op[1], buf, ASM_MIPS_OTYPE_REGISTER, ins);

   /* Exceptions: Reserved Instruction */

   return 4;
}
