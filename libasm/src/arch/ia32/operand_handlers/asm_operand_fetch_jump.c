/**
 * @file asm_operand_fetch_jump.c
 * @ingroup operand_handler
 * $Id: asm_operand_fetch_jump.c,v 1.4 2007/10/14 00:01:41 heroine Exp $
 */

#include <libasm.h>
#include <libasm-int.h>
/**
 *
 *
 */
/**
 * Decode data for operand type ASM_OTYPE_YDEST
 * @ingroup operand_handler
 * @param operand Pointer to operand structure to fill.
 * @param opcode Pointer to operand data
 * @param otype
 * @param ins Pointer to instruction structure.
 * @return Operand length
 */

#if WIP
int     asm_operand_fetch_jump(asm_operand *operand, u_char *opcode, int otype, 
				asm_instr *ins, int opt)
#else
int     asm_operand_fetch_jump(asm_operand *operand, u_char *opcode, 
			       int otype, asm_instr *ins)
#endif
{ 
  u_int	len;

  operand->type = ASM_OTYPE_JUMP;
  operand->content = ASM_OP_VALUE | ASM_OP_ADDRESS;
  operand->ptr = opcode;
  operand->imm = 0;

  //operand->len = 4;
  //memcpy(&operand->imm, opcode, 4);

  len = asm_proc_opsize(ins->proc) ? 2 : 4;
  operand->len = len;
  memcpy(&operand->imm, opcode, len);

  operand->sbaser = get_reg_intel(operand->baser, operand->regset);
  return (len);
}
