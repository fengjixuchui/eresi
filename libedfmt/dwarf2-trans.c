/*
** dwarf2.c for libedfmt (The Elf debug format library of ELFsh)
**
** Started Dec 26 2006 10:49:45 mxatone
**
*/

#include "libedfmt.h"

/* This file implements DWARF2 transformation */

#define DWARF2_HNAME_TRANS_TREF "dwarf2_trans_typeref"

/* Temporary buffer outside function to optimize function recursion */
char buf[BUFSIZ];

/* Transform hash table */
hash_t types_ref;

/* Search a type from its ckey string */
static edfmttype_t	*edfmt_dwarf2_searchtype(char *ckey)
{
  edfmttype_t 		*type;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  type = (edfmttype_t *) hash_get(&types_ref, ckey);

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, type);
}

/* Search a type from its ckey long */
static edfmttype_t 	*edfmt_dwarf2_searchtypei(long ckey)
{
  edfmttype_t 		*type;
  char			str_ckey[EDFMT_CKEY_SIZE];

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);  

  /* Create the string */
  edfmt_ckey(str_ckey, EDFMT_CKEY_SIZE, ckey);

  /* Retrieve the type */
  type = edfmt_dwarf2_searchtype(str_ckey);

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, type);
}

/* Get a type starting by try to resolve it fast */
static edfmttype_t	*edfmt_dwarf2_trans_gettype(u_int pos)
{
  edfmtdw2abbent_t   	ref;
  edfmttype_t 		*type;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  if (pos <= 0 || current_cu == NULL)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		      "Invalid parameters", NULL);

  type = edfmt_dwarf2_searchtypei(current_cu->start_pos + pos);

  if (type == NULL)
    {
      if (edfmt_dwarf2_getent(current_cu, &ref, current_cu->start_pos + pos) < 0)
	PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
			  "dwarf2 getenv failed", NULL);
      
      type = edfmt_dwarf2_transform_abbrev_parse(&ref);

      edfmt_dwarf2_freeent(&ref);
    }

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, type);
}

edfmtdw2abbattr_t 	*edfmt_dwarf2_getattr(edfmtdw2abbent_t *abbrev, u_int attr)
{
  edfmtdw2abbattr_t   	*mattr;
  u_int			index;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  if (abbrev != NULL)
    {
      for (index = 0, mattr = abbrev->attr; mattr && mattr[index].attr; index++)
	{
	  if (mattr[index].attr == attr)
	    PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, mattr + index);
	}
    }

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, NULL);
}

elfsh_Addr		edfmt_dwarf2_getaddr(char *vbuf)
{
  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  if (!vbuf)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		      "Invalid parameters", (elfsh_Addr) 0);

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, *(elfsh_Addr *) vbuf);
}

edfmttype_t		*edfmt_dwarf2_transform_abbrev_parse(edfmtdw2abbent_t *abbrev)
{
  char			*str, *comp_dir, *vbuf, *vbufs;
  long			size = 0;
  edfmttype_t		*type, *etype;
  u_long		iref, itref;
  edfmtdw2abbent_t	ref, tref;
  edfmtdw2abbattr_t	*attr;
  int			fileid, inc = 0, addtype = 1, base = 0;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  type = edfmt_dwarf2_searchtype(abbrev->ckey);

  /* Already parsed ? */
  if (type)
    PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, type);

  /* Change current file */
  if (abbrev->tag != DW_TAG_compile_unit)
    {
      DWARF2_TRANS_GETATTR(fileid, abbrev, DW_AT_decl_file, u.udata, -1);

      inc = 1;
      if (fileid > 0 && fileid < current_cu->files_number)
	{
	  fileid--;
	  str = current_cu->dirs[current_cu->files_dindex[fileid] - 1];
	  if (str != NULL 
	      && current_cu->files_dindex[fileid] < current_cu->files_number
	      && current_cu->files_dindex[fileid] > 0)
	    {
	      size = strlen(str);
	      snprintf(buf, BUFSIZ - 1, "%s%s%s",
		       str, 
		       str[size - 1] != '/' ? "/" : "",
		       current_cu->files_name[fileid]);
	    }
	  else
	    {
	      snprintf(buf, BUFSIZ - 1, "%s",
		       current_cu->files_name[fileid]);
	    }
	  
	  edfmt_add_file(edfmt_get_current_file(), buf, 0, 0);
	  
	  str = NULL;
	  size = 0;
	}
      else if (fileid >= 0)
	{
	  inc++;
	  edfmt_reset_file();
	}
    }

  DWARF2_TRANS_GETATTR(str, abbrev, DW_AT_name, u.str, NULL);

  switch(abbrev->tag)
    {
    case DW_TAG_compile_unit:
      if (!str)
	break;

      if (str[0] != '/')
	{
	  DWARF2_TRANS_GETATTR(comp_dir, abbrev, DW_AT_comp_dir, u.str, NULL);

	  if (comp_dir)
	    {
	      snprintf(buf, BUFSIZ - 1, "%s%s%s",
		       comp_dir, 
		       comp_dir[strlen(comp_dir) - 1] != '/' ? "" : "/",
		       str);
	      str = buf;
	    }
	}

      DWARF2_TRANS_GETATTR(vbuf, abbrev, DW_AT_low_pc, u.vbuf, NULL);
      DWARF2_TRANS_GETATTR(vbufs, abbrev, DW_AT_high_pc, u.vbuf, NULL);

      /* Add a new file */
      edfmt_add_file(NULL, str, 
		     edfmt_dwarf2_getaddr(vbuf),
		     edfmt_dwarf2_getaddr(vbufs));
      break;
    case DW_TAG_variable:
      attr = edfmt_dwarf2_getattr(abbrev, DW_AT_location);

      if (!attr || attr->loc.op != DW_OP_addr)
	break;

      DWARF2_TRANS_GETATTR(iref, abbrev, DW_AT_type, u.udata, 0);

      type = edfmt_dwarf2_trans_gettype(iref);

      if (!type)
	break;

      edfmt_add_var_global(type, str, attr->loc.value);
      break;
    case DW_TAG_base_type:
      DWARF2_TRANS_GETATTR(size, abbrev, DW_AT_byte_size, u.udata, -1);
      type = edfmt_add_type_basic(str, (int) size);
      break;
    case DW_TAG_typedef:
      DWARF2_TRANS_GETATTR(iref, abbrev, DW_AT_type, u.udata, 0);

      type = edfmt_dwarf2_trans_gettype(iref);

      if (!type)
	break;

      type = edfmt_add_type_link(str, type);
      break;
    case DW_TAG_pointer_type:
    case DW_TAG_reference_type:
      DWARF2_TRANS_GETATTR(iref, abbrev, DW_AT_type, u.udata, 0);

      type = edfmt_dwarf2_trans_gettype(iref);

      if (type)
	{
	  snprintf(buf, BUFSIZ - 1, "*%s", type->name);
	}
      else /* We've got a void* */
	{
	  type = edfmt_add_type_void("void");
	  snprintf(buf, BUFSIZ - 1, "void*");
	}

      type = edfmt_add_type_ptr(buf, type);
      break;
    case DW_TAG_array_type:
      if (abbrev->child > 0 && edfmt_dwarf2_getent(current_cu, &tref, abbrev->child) == 0
	  && tref.tag == DW_TAG_subrange_type)
	{
	  DWARF2_TRANS_GETATTR(iref, abbrev, DW_AT_type, u.udata, 0);
	  
	  type = edfmt_dwarf2_trans_gettype(iref);

	  if (!type)
	    break;

	  DWARF2_TRANS_GETATTR(size, &tref, DW_AT_upper_bound, u.udata, -1);

	  snprintf(buf, BUFSIZ - 1, "%s[%d]", type->name, (int) size+1);
	  type = edfmt_add_type_array(buf, (int) size, type);

	  edfmt_dwarf2_freeent(&tref);
	}
      break;
    case DW_TAG_structure_type:
    case DW_TAG_union_type:
      if (abbrev->child)
	{
	  DWARF2_TRANS_GETATTR(size, abbrev, DW_AT_byte_size, u.udata, -1);

	  if (!str)
	    {
	      snprintf(buf, BUFSIZ - 1, "s_(%u)", abbrev->key);
	      str = buf;
	    }
	  
	  //printf("Struct: %s ================ \n", str);

	  if (abbrev->tag == DW_TAG_union_type)
	    type = edfmt_add_type_union(str, (int) size);
	  else
	    type = edfmt_add_type_struct(str, (int) size);
	  
	  /* We add our structure right before members to avoid pointers infinite loop */
	  if (type)
	    HASH_ADDX(&types_ref, abbrev->ckey, (void *) type);

	  if (edfmt_dwarf2_getent(current_cu, &ref, abbrev->child) < 0)
	    break;

	  for (iref = abbrev->child; iref > 0; iref = ref.sib)
	    {
	      if (edfmt_dwarf2_getent(current_cu, &ref, iref) < 0)
		break;

	      /* Do we have a member */
	      if (ref.tag == DW_TAG_member)
		{
		  DWARF2_TRANS_GETATTR(str, &ref, DW_AT_name, u.str, NULL);
		  DWARF2_TRANS_GETATTR(itref, &ref, DW_AT_type, u.udata, 0);

		  //printf("attr: %s\n", str);

		  if (itref == 0)
		    continue;

		  if (edfmt_dwarf2_getent(current_cu, &tref, current_cu->start_pos + itref) < 0)
		    continue;

		  etype = edfmt_dwarf2_transform_abbrev_parse(&tref);

		  edfmt_dwarf2_freeent(&tref);

		  if (!etype)
		    continue;

		  if (DW_TAG_union_type != abbrev->tag)
		    {
		      attr = edfmt_dwarf2_getattr(&ref, DW_AT_data_member_location);
		      
		      if (!str || !attr)
			continue;

		      base = (int) attr->loc.value;
		    }

		  edfmt_add_type_attr(type, str, base, 0, etype);
		}
	      else
		{
		  edfmt_dwarf2_transform_abbrev_parse(&ref);
		}

	      edfmt_dwarf2_freeent(&ref);
	    }
	  
	  /* We already add our type and we want to return the right information */
	  addtype = 0;
	}
      break;
    }

  /* Restore a parent for a specific include */
  if (inc == 1)
    edfmt_restore_parent_file();
  else if (inc == 2)
    edfmt_reactive_file();

  if (type && addtype)
    HASH_ADDX(&types_ref, abbrev->ckey, (void *) type);

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, type);
}

int	   	  	edfmt_dwarf2_transform_abbrev(u_int pos)
{
  edfmtdw2abbent_t     	abbrev;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  if (pos <= 0)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		 "Invalid parameters", -1);

  while (pos > 0 && pos < current_cu->end_pos)
    {
      if (edfmt_dwarf2_getent(current_cu, &abbrev, pos) < 0)
	{
	  printf("Can't retrieve @ %d\n", pos);
	  PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
			    "Can't retrieve abbrev entitie", -1);
	}

      edfmt_dwarf2_transform_abbrev_parse(&abbrev);

      /* Parse childs too */
      if (abbrev.child > 0)
	edfmt_dwarf2_transform_abbrev(abbrev.child);

      pos = abbrev.sib;
    }

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);
}

/* Transform dwarf2 informations */
int			edfmt_dwarf2_transform(elfshobj_t *file)
{
  edfmtdw2info_t 	*tinfo;
  edfmtdw2cu_t 		*tcu;

  PROFILER_IN(__FILE__, __FUNCTION__, __LINE__);

  if (!file)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		      "Invalid parameters", -1); 

  tinfo = (edfmtdw2info_t *) file->debug_format.dwarf2;

  if (tinfo == NULL)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		      "No debug information for dwarf2", -1); 

  tcu = (edfmtdw2cu_t *) tinfo->cu_list;

  if (tcu == NULL)
    PROFILER_ERR(__FILE__, __FUNCTION__, __LINE__, 
		      "No compile unit information for dwarf2", -1);   

  if (types_ref.ent == NULL)
    hash_init(&types_ref, DWARF2_HNAME_TRANS_TREF, 50, ASPECT_TYPE_UNKNOW);

  edfmt_add_init(file);

  while (tcu != NULL)
    {
      current_cu = tcu;

      edfmt_dwarf2_transform_abbrev(tcu->info_pos);

      hash_empty(DWARF2_HNAME_TRANS_TREF);

      tcu = tcu->next;
    }
  current_cu = NULL;

  edfmt_add_end();

  PROFILER_ROUT(__FILE__, __FUNCTION__, __LINE__, 0);
}
