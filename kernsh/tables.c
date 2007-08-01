/*
** tables.c for kernsh
**
** $Id: tables.c,v 1.2 2007-08-01 18:38:31 pouik Exp $
**
*/
#include "kernsh.h"


/* Setup the command hash table */
void	setup_local_cmdhash()
{
  revm_command_add(CMD_OPENMEM , (void *) cmd_openmem   , (void *) NULL, 0, HLP_OPENMEM);
  revm_command_add(CMD_CLOSEMEM, (void *) cmd_closemem , (void *) NULL, 0, HLP_CLOSEMEM);
  revm_command_add(CMD_KMODE, (void *)cmd_kmode, (void*) revm_getvarparams, 0, HLP_KMODE);
  revm_command_add(CMD_SCT, (void *)cmd_sct, (void *) NULL, 0, HLP_SCT);
  revm_command_add(CMD_IDT, (void *)cmd_idt, (void *) NULL, 0, HLP_IDT);
  revm_command_add(CMD_GDT, (void *)cmd_gdt, (void *) NULL, 0, HLP_GDT);
  revm_command_add(CMD_MEM, (void *)cmd_mem, (void*) revm_getvarparams, 0, HLP_MEM);
}
