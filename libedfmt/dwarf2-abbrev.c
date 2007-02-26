/*
** dwarf2-abbrev.c for libedfmt (The Elf debug format library of ELFsh)
**
** Started Dec 26 2006 10:49:45 mxatone
**
*/

#include "libedfmt.h"

hash_t abbrev_table;

char *alloc_pool = NULL;
int alloc_pos = 0;
int alloc_size = 0;

#define DW2_GETPTR(_size) \
edfmt_alloc_pool(&alloc_pool, &alloc_pos, &alloc_size, DWARF2_ALLOC_STEP, _size)

/* Lookup on abbrev_table */
u_long		 	*edfmt_dwarf2_lookup_abbrev(u_int num_fetch)
{
  char			ckey[EDFMT_CKEY_SIZE];
  u_long		*pos;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  edfmt_ckey(ckey, EDFMT_CKEY_SIZE, num_fetch);
  pos = (u_long *) hash_get(&(current_cu->abbrev_table), ckey);

  //printf("Retrieve pos = %x, %ld\n", pos, *pos);

  if (pos == NULL)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		      "Abbrev number not found", NULL);

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, pos);
}

/* Read an abbrev entry @ this position */
int			edfmt_dwarf2_abbrev_read(edfmtdw2abbent_t *ent,
						  u_long pos, u_long ipos)
{
  u_int			num;
  u_int			allocattr = 0;
  u_int			attri = 0;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  if (!ent)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		      "Invalid parameters", -1);

  /* Do we have this section ? */
  if (dwarf2_data(abbrev) == NULL)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		 ".debug_abbrev section not available", -1);

  /* Update position to read the entry */
  dwarf2_pos(abbrev) = pos;

  /* Fetch the key number */
  dwarf2_iuleb128(num, abbrev);
  
  /* A level end */
  if (num == 0)
    PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);

  /* Fill main data */
  ent->key = ipos;
  edfmt_ckey(ent->ckey, EDFMT_CKEY_SIZE, ipos);

  dwarf2_iuleb128(ent->tag, abbrev);
  dwarf2_iread_1(ent->children, abbrev);

  attri = allocattr = 0;
  
  /* Parse attributes */
  do {
    /* Fill attribute structure */
    dwarf2_iuleb128(ent->attr[attri].attr, abbrev);
    dwarf2_iuleb128(ent->attr[attri].form, abbrev);
  } while (ent->attr[attri++].attr && attri < DW2_MAX_ATTR);
  
  ent->attrsize = attri > 1 ? attri - 1 : attri;
  
  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);
}

/* Clean allocated buffer for enumeration */
int			edfmt_dwarf2_abbrev_enum_clean(hash_t *abbrev_table)
{
  char			**keys;
  u_int			index;
  int			keynbr;
  u_long		*value;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);

  if (!abbrev_table)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		      "Invalid arguments",  -1);

  keys = hash_get_keys(abbrev_table, &keynbr);

  if (keys)
    {
      for (index = 0; index < keynbr; index++)
	{
	  value = (u_long *) hash_get(abbrev_table, keys[index]);

	  if (value)
	    XFREE(__FILE__, __FUNCTION__, __LINE__,value);
	}
    }

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);
}

/* Parse the .debug_abbrev format */
int			edfmt_dwarf2_abbrev_enum(hash_t *abbrev_table)
{
  u_int			num;
  u_int			base;
  u_long	      	*start_pos;
  u_int			tag, children;
  u_int			attr, form;
  char			ckey[EDFMT_CKEY_SIZE];

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  if (!abbrev_table)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		      "Invalid arguments",  -1);

  /* Do we have this section ? */
  if (dwarf2_data(abbrev) == NULL)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		      ".debug_abbrev section not available",  -1);

  base = dwarf2_pos(abbrev);

  do {
    start_pos = DW2_GETPTR(sizeof(u_long));

    *start_pos = dwarf2_pos(abbrev);

    //printf("Setpos = %x / %ld\n", start_pos, *start_pos);
    /* Fetch the key number */
    dwarf2_iuleb128(num, abbrev);
    
    /* A level end */
    if (num == 0)
      PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);
    
    dwarf2_iuleb128(tag, abbrev);
    dwarf2_iread_1(children, abbrev);
    
    /* Parse attributes */
    do {
      dwarf2_iuleb128(attr, abbrev);
      dwarf2_iuleb128(form, abbrev);
    } while (attr);
      
    /* Add into abbrev table */
    edfmt_ckey(ckey, EDFMT_CKEY_SIZE, num);
    hash_add(abbrev_table, strdup(ckey), (void *) start_pos);
  } while (dwarf2_pos(abbrev) < dwarf2_size(abbrev));

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);
}



/* Parse a form value */
static int    		edfmt_dwarf2_form_value(edfmtdw2abbattr_t *attr)
{
  u_int			i;
  char			*ptr;
  u_char 		addr[20]; /* 20 should be enough */
  u_int			data;
  long			ddata;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  switch (attr->form)
    {
    case DW_FORM_ref_addr:
      for (i = 0; i < current_cu->addr_size; i++)
	dwarf2_ipos(addr[i], info, u_char);

      attr->u.udata = *(u_long *) addr;
      break;
    case DW_FORM_addr:
      attr->u.vbuf = DW2_GETPTR(current_cu->addr_size);
      for (i = 0; i < current_cu->addr_size; i++)
	dwarf2_ipos(attr->u.vbuf[i], info, u_char);
      break;
    case DW_FORM_block1:
    case DW_FORM_block2:
    case DW_FORM_block4:
      switch (attr->form)
	{
	case DW_FORM_block1:
	  dwarf2_iread_1(attr->asize, info);
	  break;
	case DW_FORM_block2:
	  dwarf2_iread_2(attr->asize, info);
	  break;
	case DW_FORM_block4:
	  dwarf2_iread_4(attr->asize, info);
	  break;
	}

      if (attr->asize > 0)
	{
	  attr->u.vbuf = DW2_GETPTR(attr->asize);
	  
	  for (i = 0; i < attr->asize; i++)
	    dwarf2_ipos(attr->u.vbuf[i], info, u_char);
	}
      break;
    case DW_FORM_data1:
      dwarf2_iread_1(ddata, info);
      attr->u.udata = (u_long) ddata & 0xFF;
      break;
    case DW_FORM_data2:
      dwarf2_iread_2(ddata, info);
      attr->u.udata = (u_long) ddata & 0xFFFF;
      break;
    case DW_FORM_data4:
      dwarf2_iread_4(ddata, info);
      attr->u.udata = (u_long) ddata & 0xFFFFFFFF;
      break;
    case DW_FORM_data8:
      dwarf2_iread_8(ddata, info);
      attr->u.udata = (u_long) ddata;
      break;
    case DW_FORM_string:
      attr->u.str = dwarf2_ac_pos(info);
      dwarf2_inc_pos(info, strlen(attr->u.str)+1);
      break;
    case DW_FORM_block:
      dwarf2_iuleb128(attr->asize, info);

      attr->u.vbuf = DW2_GETPTR(attr->asize);

      for (i = 0; i < attr->asize; i++)
	dwarf2_ipos(attr->u.vbuf[i], info, char);
      break;
    case DW_FORM_flag:
      dwarf2_iread_1(ddata, info);
      attr->u.udata = (u_long) ddata & 0xFF;
      break;
    case DW_FORM_strp:
      ptr = (char *) dwarf2_data(str);
      dwarf2_iread_4(data, info);
      attr->u.str = (char *) ptr + data;
      break;
    case DW_FORM_udata:
      dwarf2_iuleb128(attr->u.udata, info);
      break;
    case DW_FORM_sdata:
      dwarf2_ileb128(attr->u.sdata, info);
      break;
    case DW_FORM_ref1:
      dwarf2_iread_1(attr->u.udata, info);
      break;
    case DW_FORM_ref2:
      dwarf2_iread_2(attr->u.udata, info);
      break;
    case DW_FORM_ref4:
      dwarf2_iread_4(attr->u.udata, info);
      break;
    case DW_FORM_ref8:
      dwarf2_iread_8(attr->u.udata, info);
      break;
    case DW_FORM_ref_udata:
      dwarf2_iuleb128(attr->u.udata, info);
      break;
    case DW_FORM_indirect:
      /* Read form from .debug_info */
      dwarf2_ileb128(data, info);
     
      /* Update attribute value and relaunch the function */
      attr->form = data;
      PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 
			 edfmt_dwarf2_form_value(attr));
      break;
    default:
      PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
			"Unknown form", -1);
      break;
    }

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);
}


/* Follow .debug_info form using abbrev_table as structure reference */
int			edfmt_dwarf2_form(edfmtdw2abbent_t *abbent, u_int pos)
{  
  u_int			num_fetch, i;
  u_long		*fetch;
  
  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  /* Do we have this section ? */
  if (dwarf2_data(abbrev) == NULL)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		 ".debug_info section not available", -1);

  dwarf2_pos(info) = pos;

  /* Retrieve abbrev number */
  dwarf2_iuleb128(num_fetch, info);

  /* We didn't use num_fetch == 0 */
  if (num_fetch == 0)
    PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);

  fetch = edfmt_dwarf2_lookup_abbrev(num_fetch);

  if (fetch == NULL)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		 "Impossible to retrieve abbrev position", -1);

  if (edfmt_dwarf2_abbrev_read(abbent, *fetch, pos) < 0)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		      "Impossible to read the structure", -1);

  /* Resolv form value for each attribute */
  for (i = 0; abbent->attr[i].attr; i++)
    {
      edfmt_dwarf2_form_value(abbent->attr + i);

      switch(abbent->attr[i].attr)
	{
	case DW_AT_sibling:
	  abbent->sib = current_cu->start_pos + abbent->attr[i].u.udata;
	  break;
	case DW_AT_frame_base:
	case DW_AT_data_member_location:
	case DW_AT_location:
	  edfmt_dwarf2_loc(&(abbent->attr[i].loc), abbent->attr[i].u.vbuf, abbent->attr[i].asize);
	  break;
	  /* TODO: reuse when it will be usefull
	     case DW_AT_stmt_list:
	     edfmt_dwarf2_line(ent->attr[i].u.udata);
	     break;
	     case DW_AT_macro_info:
	     edfmt_dwarf2_mac(ent->attr[i].u.udata);
	     break;
	  */
	}
    }

  if (abbent->children)
    abbent->child = dwarf2_pos(info);
  else if (abbent->sib <= 0)
    abbent->sib = dwarf2_pos(info);

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);
}

/* Read macinfo data */
int			edfmt_dwarf2_mac(u_long offset)
{
  int			line = 0, file = 0;
  u_char      		type;
  u_int			i;
  char			*str = NULL;
  edfmtdw2macro_t 	*mac;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  /* Do we have this section ? */
  if (dwarf2_data(macinfo) == NULL)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		      ".debug_macinfo section not available",  -1);

  // Update position offset
  dwarf2_pos(macinfo) = offset;

  do {
    dwarf2_iread_1(type, macinfo);

    switch(type)
      {
      case DW_MACINFO_define:
      case DW_MACINFO_undef:
	mac = DW2_GETPTR(sizeof(edfmtdw2macro_t));

	mac->fileno = file;
	mac->def = type == DW_MACINFO_define ? 1 : 0;

	dwarf2_iuleb128(mac->line, macinfo);
	mac->str = dwarf2_ac_pos(macinfo);
	dwarf2_inc_pos(macinfo, strlen(mac->str)+1);

	/* Find the first space or ( */
	for (i = 0; i < DW2_MACRON_SIZE && mac->str[i]; i++)
	  if (mac->str[i] == ' ' || mac->str[i] == '(')
	    break;
	
	/* Copy the name */
	strncpy(mac->name, mac->str, i);
	mac->name[DW2_MACRON_SIZE - 1] = 0x00;

	/* Add macro into compil unit */
	/* TODO: Reuse when it will be usefull
	if (current_cu->macro == NULL)
	  current_cu->macro = mac;
	else
	  current_cu->last_macro->next = mac;

	current_cu->last_macro = mac;
	*/
	break;
      case DW_MACINFO_start_file:
	dwarf2_iuleb128(line, macinfo);
	dwarf2_iuleb128(file, macinfo);
	break;
      case DW_MACINFO_end_file:
	file = 0;
	line = 0;
	break;
      case DW_MACINFO_vendor_ext:
	dwarf2_iuleb128(line, macinfo);
	dwarf2_istr(str, macinfo);

	/* Add vendor parse engine here ? 
	   But I don't know any vendor extension so ... */
	break;
      }
  } while (type != 0 && dwarf2_pos(macinfo) < dwarf2_size(macinfo));

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);
}

int	      		edfmt_dwarf2_loc(edfmtdw2loc_t *loc, char *buf, u_int size)
{
  u_int			i, spos, bsize, tmp_op;
  elfsh_Addr		tmp_value;
  edfmtdw2loc_t		stack[75];
  int			bra;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  if (buf == NULL || size == 0 || loc == NULL)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		      "Invalid paramters", -1);

  spos = 0;
  stack[spos].op = 0;
  stack[spos].value = 0;

  /* Iterate for each part of the block */
  for (i = 0; i < size; i++)
    {
      /* Push a stack operand */
      switch(buf[i] & 0xFF)
	{
	case DW_OP_addr:
	case DW_OP_const1u:
	case DW_OP_const1s:
	case DW_OP_const2u:
	case DW_OP_const2s:
	case DW_OP_const4u:
	case DW_OP_const4s:
	case DW_OP_const8u:
	case DW_OP_const8s:	
	case DW_OP_constu:	
	case DW_OP_consts:	
	case DW_OP_regx:
	case DW_OP_lit0:	
	case DW_OP_lit1:	
	case DW_OP_lit2:	
	case DW_OP_lit3:	
	case DW_OP_lit4:	
	case DW_OP_lit5:	
	case DW_OP_lit6:	
	case DW_OP_lit7:	
	case DW_OP_lit8:	
	case DW_OP_lit9:	
	case DW_OP_lit10:	
	case DW_OP_lit11:	
	case DW_OP_lit12:	
	case DW_OP_lit13:	
	case DW_OP_lit14:	
	case DW_OP_lit15:	
	case DW_OP_lit16:	
	case DW_OP_lit17:	
	case DW_OP_lit18:	
	case DW_OP_lit19:	
	case DW_OP_lit20:	
	case DW_OP_lit21:	
	case DW_OP_lit22:	
	case DW_OP_lit23:	
	case DW_OP_lit24:	
	case DW_OP_lit25:	
	case DW_OP_lit26:	
	case DW_OP_lit27:	
	case DW_OP_lit28:	
	case DW_OP_lit29:	
	case DW_OP_lit30:	
	case DW_OP_lit31:	
	case DW_OP_reg0:	
	case DW_OP_reg1:	
	case DW_OP_reg2:	
	case DW_OP_reg3:	
	case DW_OP_reg4:	
	case DW_OP_reg5:	
	case DW_OP_reg6:	
	case DW_OP_reg7:	
	case DW_OP_reg8:	
	case DW_OP_reg9:	
	case DW_OP_reg10:	
	case DW_OP_reg11:	
	case DW_OP_reg12:	
	case DW_OP_reg13:	
	case DW_OP_reg14:	
	case DW_OP_reg15:	
	case DW_OP_reg16:	
	case DW_OP_reg17:	
	case DW_OP_reg18:	
	case DW_OP_reg19:	
	case DW_OP_reg20:	
	case DW_OP_reg21:	
	case DW_OP_reg22:	
	case DW_OP_reg23:	
	case DW_OP_reg24:	
	case DW_OP_reg25:	
	case DW_OP_reg26:	
	case DW_OP_reg27:	
	case DW_OP_reg28:	
	case DW_OP_reg29:	
	case DW_OP_reg30:	
	case DW_OP_reg31:	
	case DW_OP_breg0:	
	case DW_OP_breg1:	
	case DW_OP_breg2:	
	case DW_OP_breg3:	
	case DW_OP_breg4:	
	case DW_OP_breg5:	
	case DW_OP_breg6:	
	case DW_OP_breg7:	
	case DW_OP_breg8:	
	case DW_OP_breg9:	
	case DW_OP_breg10:	
	case DW_OP_breg11:	
	case DW_OP_breg12:	
	case DW_OP_breg13:	
	case DW_OP_breg14:	
	case DW_OP_breg15:	
	case DW_OP_breg16:	
	case DW_OP_breg17:	
	case DW_OP_breg18:	
	case DW_OP_breg19:	
	case DW_OP_breg20:	
	case DW_OP_breg21:	
	case DW_OP_breg22:	
	case DW_OP_breg23:	
	case DW_OP_breg24:	
	case DW_OP_breg25:	
	case DW_OP_breg26:	
	case DW_OP_breg27:	
	case DW_OP_breg28:	
	case DW_OP_breg29:	
	case DW_OP_breg30:	
	case DW_OP_breg31:	
	case DW_OP_bregx:
	case DW_OP_fbreg:	
	  stack[++spos].op = buf[i];
	  break;
	}

      /* Thread value */
      switch(buf[i] & 0xFF)
	{
	case DW_OP_addr:
	  stack[spos].value = *(elfsh_Addr *) (buf + (++i));
	  i += sizeof(elfsh_Addr) - 1;
	  break;
	case DW_OP_deref:
	  stack[spos].op = DW_OP_addr;
	  if (stack[spos].value != NULL)
	    stack[spos].value = *(elfsh_Addr *) stack[spos].value;
	  break;
	case DW_OP_const1u:
	  stack[spos].value = *(u_char *) (buf + (++i));
	  i++;
	  break;
	case DW_OP_const1s:
	  stack[spos].value = *(char *) (buf + (++i));
	  i++;
	  break;
	case DW_OP_const2u:
	  stack[spos].value = *(u_short *) (buf + (++i));
	  i += 2;
	  break;
	case DW_OP_const2s:
	  stack[spos].value = *(short *) (buf + (++i));
	  i += 2;
	  break;
	case DW_OP_const4u:
	  stack[spos].value = *(u_int *) (buf + (++i));
	  i += 4;
	  break;
	case DW_OP_const4s:
	  stack[spos].value = *(int *) (buf + (++i));
	  i += 4;
	  break;
	case DW_OP_const8u:
	  stack[spos].value = *(u_long *) (buf + (++i));
	  i += 8;
	  break;
	case DW_OP_const8s:	
	  stack[spos].value = *(long *) (buf + (++i));
	  i += 8;
	  break;
	case DW_OP_constu:	
	  stack[spos].value = edfmt_read_uleb128(buf + (++i), &bsize);
	  i += bsize;
	  break;
	case DW_OP_consts:	
	  stack[spos].value = edfmt_read_leb128(buf + (++i), &bsize);
	  i += bsize;
	  break;
	case DW_OP_dup:
	  stack[spos+1].op = stack[spos].op;
	  stack[spos+1].value = stack[spos].value;
	  spos++;
	  break;
	case DW_OP_drop:	
	  spos--;
	  break;
	case DW_OP_over:
	  stack[spos+1].op = stack[spos - 1].op;
	  stack[spos+1].value = stack[spos - 1].value;
	  spos++;
	  break;
	case DW_OP_pick:	
	  if (spos + buf[++i] < 0)
	    break;

	  stack[spos+1].op = stack[spos - buf[i]].op;
	  stack[spos+1].value = stack[spos - buf[i]].value;
	  spos++;
	  break;
	case DW_OP_swap:	
	  tmp_op = stack[spos].op;
	  tmp_value = stack[spos].value;
	  stack[spos].op = stack[spos - 1].op;
	  stack[spos].value = stack[spos - 1].value;
	  stack[spos - 1].op = tmp_op;
	  stack[spos - 1].value = tmp_value;
	  break;
	case DW_OP_rot:	
	  tmp_op = stack[spos].op;
	  tmp_value = stack[spos].value;
	  stack[spos].op = stack[spos - 1].op;
	  stack[spos].value = stack[spos - 1].value;
	  stack[spos - 1].op = tmp_op;
	  stack[spos - 1].value = tmp_value;

	  tmp_op = stack[spos+1].op;
	  tmp_value = stack[spos+1].value;
	  stack[spos+1].op = stack[spos - 2].op;
	  stack[spos+1].value = stack[spos - 2].value;
	  stack[spos - 2].op = tmp_op;
	  stack[spos - 2].value = tmp_value;
	  break;
	case DW_OP_abs:	
	  if (stack[spos].value < 0)
	    stack[spos].value = - stack[spos].value;
	  break;
	case DW_OP_and:	
	  stack[spos - 1].value &= stack[spos].value;
	  spos--;
	  break;
	case DW_OP_div:	
	  stack[spos - 1].value /= stack[spos].value;
	  spos--;
	  break;
	case DW_OP_minus:	
	  stack[spos - 1].value -= stack[spos].value;
	  spos--;
	  break;
	case DW_OP_mod:	
	  stack[spos - 1].value = stack[spos - 1].value % stack[spos].value;
	  spos--;
	  break;
	case DW_OP_mul:	
	  stack[spos - 1].value *= stack[spos].value;
	  spos--;
	  break;
	case DW_OP_neg:	
	  stack[spos].value = -stack[spos].value;
	  break;
	case DW_OP_not:	
	  stack[spos].value = !stack[spos].value;
	  break;
	case DW_OP_or:	
	  stack[spos - 1].value |= stack[spos].value;
	  spos--;
	  break;
	case DW_OP_plus:	
	  stack[spos - 1].value += stack[spos].value;
	  spos--;
	  break;
	case DW_OP_plus_uconst:	
	  stack[spos].value += edfmt_read_uleb128(buf + (++i), &bsize);
	  i += bsize;
	  break;
	case DW_OP_shl:	
	  stack[spos - 1].value <<= stack[spos].value;
	  spos--;
	  break;
	case DW_OP_shr:	
	  stack[spos - 1].value >>= stack[spos].value;
	  spos--;
	  break;
	case DW_OP_xor:	
	  stack[spos - 1].value ^= stack[spos].value;
	  spos--;
	  break;
	case DW_OP_bra:
	  bra = *(short *) (buf + (++i));
	  i += 2;

	  if (stack[spos].value != 0)
	    i += bra;
	  spos--;
	  break;
	case DW_OP_eq:	
	  stack[spos - 1].value = (stack[spos].value == stack[spos - 1].value);
	  spos--;
	  break;
	case DW_OP_ge:
	  stack[spos - 1].value = (stack[spos].value >= stack[spos - 1].value);
	  spos--;
	  break;
	case DW_OP_gt:	
	  stack[spos - 1].value = (stack[spos].value > stack[spos - 1].value);
	  spos--;
	  break;
	case DW_OP_le:	
	  stack[spos - 1].value = (stack[spos].value <= stack[spos - 1].value);
	  spos--;
	  break;
	case DW_OP_lt:	
	  stack[spos - 1].value = (stack[spos].value < stack[spos - 1].value);
	  spos--;
	  break;
	case DW_OP_ne:	
	  stack[spos - 1].value = (stack[spos].value != stack[spos - 1].value);
	  spos--;
	  break;
	case DW_OP_skip:	
	  bra = *(short *) (buf + (++i));
	  i += 2;
	  i += bra;
	  break;
	case DW_OP_lit0:	
	case DW_OP_lit1:	
	case DW_OP_lit2:	
	case DW_OP_lit3:	
	case DW_OP_lit4:	
	case DW_OP_lit5:	
	case DW_OP_lit6:	
	case DW_OP_lit7:	
	case DW_OP_lit8:	
	case DW_OP_lit9:	
	case DW_OP_lit10:	
	case DW_OP_lit11:	
	case DW_OP_lit12:	
	case DW_OP_lit13:	
	case DW_OP_lit14:	
	case DW_OP_lit15:	
	case DW_OP_lit16:	
	case DW_OP_lit17:	
	case DW_OP_lit18:	
	case DW_OP_lit19:	
	case DW_OP_lit20:	
	case DW_OP_lit21:	
	case DW_OP_lit22:	
	case DW_OP_lit23:	
	case DW_OP_lit24:	
	case DW_OP_lit25:	
	case DW_OP_lit26:	
	case DW_OP_lit27:	
	case DW_OP_lit28:	
	case DW_OP_lit29:	
	case DW_OP_lit30:	
	case DW_OP_lit31:	
	  stack[spos].value = buf[i] - DW_OP_lit0;
	  break;
	case DW_OP_reg0:	
	case DW_OP_reg1:	
	case DW_OP_reg2:	
	case DW_OP_reg3:	
	case DW_OP_reg4:	
	case DW_OP_reg5:	
	case DW_OP_reg6:	
	case DW_OP_reg7:	
	case DW_OP_reg8:	
	case DW_OP_reg9:	
	case DW_OP_reg10:	
	case DW_OP_reg11:	
	case DW_OP_reg12:	
	case DW_OP_reg13:	
	case DW_OP_reg14:	
	case DW_OP_reg15:	
	case DW_OP_reg16:	
	case DW_OP_reg17:	
	case DW_OP_reg18:	
	case DW_OP_reg19:	
	case DW_OP_reg20:	
	case DW_OP_reg21:	
	case DW_OP_reg22:	
	case DW_OP_reg23:	
	case DW_OP_reg24:	
	case DW_OP_reg25:	
	case DW_OP_reg26:	
	case DW_OP_reg27:	
	case DW_OP_reg28:	
	case DW_OP_reg29:	
	case DW_OP_reg30:	
	case DW_OP_reg31:	
	  stack[spos].value = buf[i] - DW_OP_reg0;
	  break;
	case DW_OP_breg0:	
	case DW_OP_breg1:	
	case DW_OP_breg2:	
	case DW_OP_breg3:	
	case DW_OP_breg4:	
	case DW_OP_breg5:	
	case DW_OP_breg6:	
	case DW_OP_breg7:	
	case DW_OP_breg8:	
	case DW_OP_breg9:	
	case DW_OP_breg10:	
	case DW_OP_breg11:	
	case DW_OP_breg12:	
	case DW_OP_breg13:	
	case DW_OP_breg14:	
	case DW_OP_breg15:	
	case DW_OP_breg16:	
	case DW_OP_breg17:	
	case DW_OP_breg18:	
	case DW_OP_breg19:	
	case DW_OP_breg20:	
	case DW_OP_breg21:	
	case DW_OP_breg22:	
	case DW_OP_breg23:	
	case DW_OP_breg24:	
	case DW_OP_breg25:	
	case DW_OP_breg26:	
	case DW_OP_breg27:	
	case DW_OP_breg28:	
	case DW_OP_breg29:	
	case DW_OP_breg30:	
	case DW_OP_breg31:	
	  stack[spos].value = buf[i] - DW_OP_breg0;
	  break;
	case DW_OP_regx:
	  stack[spos].value = edfmt_read_uleb128(buf + (++i), &bsize);
	  i += bsize;
	  break;
	case DW_OP_fbreg:
	  stack[spos].value = edfmt_read_leb128(buf + (++i), &bsize);
	  i += bsize;
	  break;
	case DW_OP_bregx:	
	  stack[spos].value = edfmt_read_uleb128(buf + (++i), &bsize);
	  i += bsize;
	  break;
	  /*
	    TODO: Find how we can parse that correctly
	case DW_OP_deref_size:	
	  stack[spos].op = DW_OP_addr;
	  tmp_value = stack[spos].value;
	  stack[spos].value = 0;
	  for (z = 0, i++; z < buf[i]; z++)
	    stack[spos].value = ((char*)&tmp_value)[z];
	  break;
	  */
	case DW_OP_nop:	
	  break;
	case DW_OP_shra:	
	case DW_OP_piece:	
	case DW_OP_xderef_size:	
	case DW_OP_xderef:
	  /* Unimplemented, for multiple addresse space architecture ? */
	  break;
	}      
    }

  /* Save location value */
  loc->op = stack[spos].op;
  loc->value = stack[spos].value;

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);
}

/* Parse data part of .debug_line format */
static int		edfmt_dwarf2_line_data(edfmtdw2linehead_t *header)
{
  elfsh_Addr  		addr;
  u_int			file;
  u_int			line;
  u_int			column;
  u_int	      		is_stmt;
  u_int	       		basic_block;
  u_int			end_sequence;
  u_long		tmpf;

  u_int			i, bsize;
  u_char       		opc, ext_opc;
  char			*read_addr;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  if (header == NULL)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		       "Invalid parameters", -1);

  /* Loop until end position */
  while (dwarf2_pos(line) < header->end_pos)
    {
      addr = 0;
      file = 1;
      line = 1;
      column = 0;
      is_stmt = header->default_stmt;
      basic_block = 0;
      end_sequence = 0;

      while (!end_sequence)
	{
	  dwarf2_iread_1(opc, line);

	  if (opc >= header->opcode_base)
	    {
	      opc -= header->opcode_base;
	      line += header->line_base + (opc % header->line_range);
	      addr += (opc / header->line_range) * header->min_length_instr;

	      edfmt_dwarf2_line_rec(current_cu, line, column, addr, file - 1);
	      basic_block = 1;
	    }
	  else
	    {
	      switch(opc)
		{
		case DW_LNS_extended_op:
		  dwarf2_iuleb128(tmpf, line);

		  dwarf2_iread_1(ext_opc, line);
		  switch(ext_opc)
		    {
		    case DW_LNE_end_sequence:
		      end_sequence = 1;
		      edfmt_dwarf2_line_rec(current_cu, line, column, addr, file - 1);
		      break;
		    case DW_LNE_set_address:
		      read_addr = DW2_GETPTR(current_cu->addr_size);
		      
		      for (i = 0; i < current_cu->addr_size; i++)
			dwarf2_ipos(read_addr[i], line, u_char);

		      addr = *(elfsh_Addr *) read_addr;
		      break;
		    case DW_LNE_define_file:
		      header->files_number++;
		      /* Realloc pointers */
		      XREALLOC(__FILE__, __FUNCTION__, __LINE__,header->files_name, header->files_name, 
			       sizeof(char*)*header->files_number, -1);
		      XREALLOC(__FILE__, __FUNCTION__, __LINE__,header->files_dindex, header->files_dindex, 
			       sizeof(u_int)*header->files_number, -1);
		      XREALLOC(__FILE__, __FUNCTION__, __LINE__,header->files_time, header->files_time, 
			       sizeof(u_int)*header->files_number, -1);
		      XREALLOC(__FILE__, __FUNCTION__, __LINE__,header->files_len, header->files_len, 
			       sizeof(u_int)*header->files_number, -1);

		      /* Read information and fill new entrie */
		      dwarf2_istr(header->files_name[header->files_number], line);
		      dwarf2_iuleb128(header->files_dindex[header->files_number], line);
		      dwarf2_iuleb128(header->files_time[header->files_number], line);
		      dwarf2_iuleb128(header->files_len[header->files_number], line);
		      break;
		    }
		  break;
		case DW_LNS_copy:
		  basic_block = 0;
		  edfmt_dwarf2_line_rec(current_cu, line, column, addr, file - 1);
		  break;
		case DW_LNS_advance_pc:
		  dwarf2_iuleb128(tmpf, line);
		  addr += tmpf * header->min_length_instr;
		  break;
		case DW_LNS_advance_line:
		  dwarf2_ileb128(tmpf, line);
		  line += tmpf;
		  break;
		case DW_LNS_set_file:
		  dwarf2_iuleb128(file, line);
		  break;
		case DW_LNS_set_column:
		  dwarf2_iuleb128(column, line);
		  break;
		case DW_LNS_negate_stmt:
		  is_stmt = !is_stmt;
		  break;
		case DW_LNS_set_basic_block:
		  basic_block = 1;
		  break;
		case DW_LNS_const_add_pc:
		  addr += ((255 - header->opcode_base) / header->line_range) 
		    * header->min_length_instr;
		  break;
		case DW_LNS_fixed_advance_pc:
		  dwarf2_iread_2(bsize, line);
		  addr += bsize;
		  break;
		default:
		  for (i = 0; i < header->std_opcode_length[opc]; i++)
		    dwarf2_iuleb128(tmpf, line);
		}
	    }
	}
    }

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);
}

/* Parse .debug_line */
int			edfmt_dwarf2_line(u_long offset)
{
  u_int			i;
  edfmtdw2linehead_t 	header;
  u_long      		prev_pos, tmpf;
  char			*strf;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  /* Update position offset */
  dwarf2_pos(line) = offset;

  /* Parse the header */
  dwarf2_iread_4(header.total_length, line);
  header.end_pos = dwarf2_pos(line) + header.total_length;
  dwarf2_iread_2(header.version, line);
  dwarf2_iread_4(header.prologue_length, line);
  header.prologue_pos = dwarf2_pos(line);
  dwarf2_iread_1(header.min_length_instr, line);
  dwarf2_iread_1(header.default_stmt, line);
  dwarf2_iread_1(header.line_base, line);
  dwarf2_iread_1(header.line_range, line);
  dwarf2_iread_1(header.opcode_base, line);
  header.std_opcode_length = dwarf2_ac_pos(line);
  dwarf2_inc_pos(line, header.opcode_base - 1);
  
  /* Dir first pass */
  prev_pos = dwarf2_pos(line);
  for (i = 0; (dwarf2_ac_pos(line))[0] != '\0'; i++)
    dwarf2_inc_pos(line, strlen(dwarf2_ac_pos(line))+1);
  dwarf2_pos(line) = prev_pos;

  header.dirs = DW2_GETPTR(sizeof(char*)*i);
  header.dirs_number = i+1;

  /* Dir second pass */
  for (i = 0; (dwarf2_ac_pos(line))[0] != '\0'; i++)
    {
      header.dirs[i] = dwarf2_ac_pos(line);
      dwarf2_inc_pos(line, strlen(dwarf2_ac_pos(line))+1);
    }
  dwarf2_inc_pos(line, 1);

  /* Filename first pass */
  prev_pos = dwarf2_pos(line);
  for (i = 0; (dwarf2_ac_pos(line))[0] != '\0'; i++)
    {
      dwarf2_istr(strf, line);
      dwarf2_iuleb128(tmpf, line);
      dwarf2_iuleb128(tmpf, line);
      dwarf2_iuleb128(tmpf, line);
    }
  dwarf2_pos(line) = prev_pos;

  header.files_name = DW2_GETPTR(sizeof(char*)*i);
  header.files_dindex = DW2_GETPTR(sizeof(u_int)*i);
  header.files_time = DW2_GETPTR(sizeof(u_int)*i);
  header.files_len = DW2_GETPTR(sizeof(u_int)*i);

  header.files_number = i+1;

  /* Filename second pass */
  for (i = 0; (dwarf2_ac_pos(line))[0] != '\0'; i++)
    {
      dwarf2_istr(header.files_name[i], line);
      dwarf2_iuleb128(header.files_dindex[i], line);
      dwarf2_iuleb128(header.files_time[i], line);
      dwarf2_iuleb128(header.files_len[i], line);
    }
  dwarf2_inc_pos(line, 1);

  /* Check position validity */
  if (header.prologue_pos + header.prologue_length != dwarf2_pos(line))
    dwarf2_pos(line) = header.prologue_pos + header.prologue_length;

  /* Save files & directories list */
  current_cu->dirs 	   = header.dirs;
  current_cu->files_name   = header.files_name;
  current_cu->files_dindex = header.files_dindex;
  current_cu->files_time   = header.files_time;
  current_cu->files_len    = header.files_len;
  current_cu->files_number = header.files_number;

  /* Parse the rest */
  edfmt_dwarf2_line_data(&header);

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);
}

int			edfmt_dwarf2_line_rec(edfmtdw2cu_t *cu, u_int line, u_int column, 
					      elfsh_Addr addr, u_int fid)
{
  edfmtdw2line_t 	*pline;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  pline = DW2_GETPTR(sizeof(edfmtdw2line_t));

  pline->addr = addr;
  pline->line = line;
  pline->column = column;
  pline->fileid = fid;
  pline->cu = cu;

  /* Add line informations */
  /* TODO: Reuse when it will be usefull 
  if (cu->line == NULL)
    cu->line = pline;
  else
    cu->last_line->next = pline;

  cu->last_line = pline;
  */
  
  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);
}
