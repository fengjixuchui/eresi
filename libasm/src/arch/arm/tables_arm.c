#include <libasm.h>

char *arm_instr_list[ASM_ARM_BAD + 1] = 
  {
    "(unimpl)",
    "adc",
    "add",
    "and",
    "b",
    "bic",
    "bl",
    "cdp",
    "cmn",
    "cmp",
    "eor",
    "ldc",
    "ldm",
    "ldr",
    "ldrb",
    "ldrbt",
    "ldrh",
    "ldrsb",
    "ldrsh",
    "ldrt",
    "mcr",
    "mla",
    "mov",
    "mrc",
    "mrs",
    "msr",
    "mul",
    "mvn",
    "orr",
    "rsb",
    "rsc",
    "sbc",
    "smlal",
    "smull",
    "stc",
    "stm",
    "str",
    "strb",
    "strbt",
    "strh",
    "strt",
    "sub",
    "swi",
    "swp",
    "swpb",
    "teq",
    "tst",
    "umlal",
    "umull",
    "(bad)",
  };
