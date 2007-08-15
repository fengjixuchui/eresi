/*
** $Id: op_stc.c,v 1.4 2007-08-15 21:30:21 strauss Exp $
**
*/
#include <libasm.h>
#include <libasm-int.h>

/*
  <instruction func="op_stc" opcode="0xf9"/>
*/

int op_stc(asm_instr *new, u_char *opcode, u_int len, asm_processor *proc) 
{
  new->len += 1;
  new->ptr_instr = opcode;
  new->instr = ASM_STC;
  new->type = ASM_TYPE_WRITEFLAG;
  new->flagswritten = ASM_FLAG_CF;
  return (new->len);
}
