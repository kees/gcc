/* Kernel Control Flow Integrity (KCFI) support for GCC.
   Copyright (C) 2025 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "target.h"
#include "function.h"
#include "tree.h"
#include "tree-pass.h"
#include "basic-block.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "gimple-walk.h"
#include "cgraph.h"
#include "ipa-ref.h"
#include "kcfi.h"
#include "stringpool.h"
#include "attribs.h"
#include "fold-const.h"
#include "rtl.h"
#include "cfghooks.h"
#include "cfg.h"
#include "asan.h"
#include "diagnostic-core.h"
#include "memmodel.h"
#include "emit-rtl.h"
#include "expr.h"
#include "output.h"
#include "varasm.h"
#include "opts.h"

/* Global KCFI target hooks structure - zero-initialized for safe defaults.  */
struct kcfi_target_hooks kcfi_target = { };

/* Common KCFI utilities.  */

/* Common helper for RTL patterns to emit .kcfi_traps section entry.  */
void
kcfi_emit_trap_with_section (FILE *file, rtx trap_label_rtx)
{
  /* Convert trap label to string using standard GCC helper.  */
  char trap_name[64];
  ASM_GENERATE_INTERNAL_LABEL (trap_name, "L", CODE_LABEL_NUMBER (trap_label_rtx));

  /* Generate entry label name from trap label number.  */
  char entry_name[64];
  ASM_GENERATE_INTERNAL_LABEL (entry_name, "Lentry", CODE_LABEL_NUMBER (trap_label_rtx));

  /* Emit .kcfi_traps section entry using the converted labels.  */
  fprintf (file, "\t.pushsection\t.kcfi_traps,\"ao\",@progbits,.text\n");
  assemble_name (file, entry_name);
  fprintf (file, ":\n");
  fprintf (file, "\t.long\t");
  assemble_name (file, trap_name);
  fprintf (file, " - ");
  assemble_name (file, entry_name);
  fprintf (file, "\n");
  fprintf (file, "\t.popsection\n");
}

/* Hash function for KCFI type ID computation.
   This implements a simple hash similar to FNV-1a.  */
static uint32_t
kcfi_hash_string (const char *str)
{
  uint32_t hash = 2166136261U; /* FNV-1a 32-bit offset basis.  */
  for (const char *p = str; *p; p++)
    {
      hash ^= (unsigned char) *p;
      hash *= 16777619U; /* FNV-1a 32-bit prime.  */
    }
  return hash;
}

/* Forward declaration for recursive type mangling.  */
static void mangle_type_to_buffer (tree type, char **p, char *end);

/* Mangle a builtin type following Itanium C++ ABI for C types.  */
static void
mangle_builtin_type_to_buffer (tree type, char **p, char *end)
{
  gcc_assert (type != NULL_TREE);
  gcc_assert (p != NULL && *p != NULL && end != NULL);
  gcc_assert (*p < end);

  if (*p >= end)
    return;

  switch (TREE_CODE (type))
    {
    case VOID_TYPE:
      **p = 'v';
      (*p)++;
      break;

    case BOOLEAN_TYPE:
      **p = 'b';
      (*p)++;
      break;

    case INTEGER_TYPE:
      /* Handle standard integer types using Itanium ABI codes.  */
      if (type == char_type_node)
	{
	  **p = 'c';
	  (*p)++;
	}
      else if (type == signed_char_type_node)
	{
	  **p = 'a';
	  (*p)++;
	}
      else if (type == unsigned_char_type_node)
	{
	  **p = 'h';
	  (*p)++;
	}
      else if (type == short_integer_type_node)
	{
	  **p = 's';
	  (*p)++;
	}
      else if (type == short_unsigned_type_node)
	{
	  **p = 't';
	  (*p)++;
	}
      else if (type == integer_type_node)
	{
	  **p = 'i';
	  (*p)++;
	}
      else if (type == unsigned_type_node)
	{
	  **p = 'j';
	  (*p)++;
	}
      else if (type == long_integer_type_node)
	{
	  **p = 'l';
	  (*p)++;
	}
      else if (type == long_unsigned_type_node)
	{
	  **p = 'm';
	  (*p)++;
	}
      else if (type == long_long_integer_type_node)
	{
	  **p = 'x';
	  (*p)++;
	}
      else if (type == long_long_unsigned_type_node)
	{
	  **p = 'y';
	  (*p)++;
	}
      else
	{
	  /* Fallback for other integer types - use precision-based encoding.  */
	  *p += snprintf (*p, end - *p, "i%d", TYPE_PRECISION (type));
	}
      break;

    case REAL_TYPE:
      if (type == float_type_node)
	{
	  **p = 'f';
	  (*p)++;
	}
      else if (type == double_type_node)
	{
	  **p = 'd';
	  (*p)++;
	}
      else if (type == long_double_type_node)
	{
	  **p = 'e';
	  (*p)++;
	}
      else
	{
	  /* Fallback for other real types.  */
	  *p += snprintf (*p, end - *p, "f%d", TYPE_PRECISION (type));
	}
      break;

    default:
      /* Unknown builtin type - this should never happen in a well-formed C program.  */
      error ("KCFI: Unknown builtin type with TREE_CODE %<%E%>", TREE_CODE (type));
      error ("KCFI: %<TYPE_MODE%> = %d, %<TYPE_PRECISION%> = %d", TYPE_MODE (type), TYPE_PRECISION (type));
      error ("KCFI: Please report this as a bug with the above diagnostic information");
      gcc_unreachable ();
    }
}

/* Canonicalize typedef types to their underlying named struct/union types.  */
static tree
canonicalize_typedef_type (tree type)
{
  /* Handle typedef types - canonicalize to named structs when possible.  */
  if (TYPE_NAME (type) && TREE_CODE (TYPE_NAME (type)) == TYPE_DECL)
    {
      tree type_decl = TYPE_NAME (type);

      /* Check if this is a typedef (not the original struct declaration) */
      if (DECL_ORIGINAL_TYPE (type_decl))
	{
	  tree original_type = DECL_ORIGINAL_TYPE (type_decl);

	  /* If the original type is a named struct/union/enum, use that instead.  */
	  if ((TREE_CODE (original_type) == RECORD_TYPE
	       || TREE_CODE (original_type) == UNION_TYPE
	       || TREE_CODE (original_type) == ENUMERAL_TYPE)
	      && TYPE_NAME (original_type)
	      && ((TREE_CODE (TYPE_NAME (original_type)) == TYPE_DECL
		   && DECL_NAME (TYPE_NAME (original_type)))
		  || TREE_CODE (TYPE_NAME (original_type)) == IDENTIFIER_NODE))
	    {
	      /* Recursively canonicalize in case the original type is also a typedef.  */
	      return canonicalize_typedef_type (original_type);
	    }

	  /* For basic type typedefs (e.g., u8 -> unsigned char), canonicalize to original type.  */
	  if (TREE_CODE (original_type) == INTEGER_TYPE
	      || TREE_CODE (original_type) == REAL_TYPE
	      || TREE_CODE (original_type) == POINTER_TYPE
	      || TREE_CODE (original_type) == ARRAY_TYPE
	      || TREE_CODE (original_type) == FUNCTION_TYPE
	      || TREE_CODE (original_type) == METHOD_TYPE
	      || TREE_CODE (original_type) == BOOLEAN_TYPE
	      || TREE_CODE (original_type) == COMPLEX_TYPE
	      || TREE_CODE (original_type) == VECTOR_TYPE)
	    {
	      /* Recursively canonicalize in case the original type is also a typedef.  */
	      return canonicalize_typedef_type (original_type);
	    }
	}
    }

  return type;
}

/* Recursively mangle a type following Itanium C++ ABI conventions.  */
static void
mangle_type_to_buffer (tree type, char **p, char *end)
{
  gcc_assert (type != NULL_TREE);
  gcc_assert (p != NULL && *p != NULL && end != NULL);
  gcc_assert (*p < end);

  if (*p >= end)
    return;

  /* Canonicalize typedef types to their underlying named struct types.  */
  type = canonicalize_typedef_type (type);

  switch (TREE_CODE (type))
    {
    case POINTER_TYPE:
      {
	/* Pointer type: 'P' + qualifiers + pointed-to type.  */
	**p = 'P';
	(*p)++;

	/* Add qualifiers to the pointed-to type following Itanium C++ ABI ordering.  */
	tree pointed_to_type = TREE_TYPE (type);
	if (TYPE_QUALS (pointed_to_type) != TYPE_UNQUALIFIED)
	  {
	    /* Emit qualifiers in Itanium ABI order: restrict, volatile, const.  */
	    if (TYPE_QUALS (pointed_to_type) & TYPE_QUAL_RESTRICT)
	      {
		**p = 'r';
		(*p)++;
	      }
	    if (TYPE_QUALS (pointed_to_type) & TYPE_QUAL_VOLATILE)
	      {
		**p = 'V';
		(*p)++;
	      }
	    if (TYPE_QUALS (pointed_to_type) & TYPE_QUAL_CONST)
	      {
		**p = 'K';
		(*p)++;
	      }
	    /* Note: _Atomic is not typically used in kernel code.  */
	  }

	/* For KCFI's hybrid type system: preserve typedef names for compound types,
	   but use canonical forms for primitive types.  */
	tree target_type;
	if (TREE_CODE (pointed_to_type) == RECORD_TYPE
	    || TREE_CODE (pointed_to_type) == UNION_TYPE
	    || TREE_CODE (pointed_to_type) == ENUMERAL_TYPE)
	  {
	    /* Compound type: preserve typedef information by using original type.  */
	    target_type = pointed_to_type;
	  }
	else
	  {
	    /* Primitive type: use canonical form to ensure structural typing.  */
	    target_type = TYPE_MAIN_VARIANT (pointed_to_type);
	  }
	mangle_type_to_buffer (target_type, p, end);
	break;
      }

    case ARRAY_TYPE:
      /* Array type: 'A' + size + '_' + element type (simplified).  */
      **p = 'A';
      (*p)++;
      if (TYPE_DOMAIN (type) && TYPE_MAX_VALUE (TYPE_DOMAIN (type)))
	{
	  HOST_WIDE_INT size = tree_to_shwi (TYPE_MAX_VALUE (TYPE_DOMAIN (type))) + 1;
	  *p += snprintf (*p, end - *p, "%ld_", (long) size);
	}
      else
	{
	  **p = '_';
	  (*p)++;
	}
      mangle_type_to_buffer (TREE_TYPE (type), p, end);
      break;

    case FUNCTION_TYPE:
      {
	/* Function type: 'F' + return type + parameter types + 'E' */
	**p = 'F';
	(*p)++;
	mangle_type_to_buffer (TREE_TYPE (type), p, end);

	/* Add parameter types.  */
	tree param_types = TYPE_ARG_TYPES (type);
	for (tree param = param_types; param && *p < end; param = TREE_CHAIN (param))
	  {
	    tree param_type = TREE_VALUE (param);
	    if (param_type == void_type_node)
	      break;
	    mangle_type_to_buffer (param_type, p, end);
	  }

	**p = 'E';
	(*p)++;
	break;
      }

    case RECORD_TYPE:
    case UNION_TYPE:
    case ENUMERAL_TYPE:
      {
	/* Struct/union/enum: use simplified representation for C types.  */
	const char *name = NULL;

	if (TYPE_NAME (type))
	  {
	    if (TREE_CODE (TYPE_NAME (type)) == TYPE_DECL)
	      {
		/* TYPE_DECL case: both named structs and typedef structs.  */
		tree decl_name = DECL_NAME (TYPE_NAME (type));
		if (decl_name && TREE_CODE (decl_name) == IDENTIFIER_NODE)
		  {
		    name = IDENTIFIER_POINTER (decl_name);
		  }
	      }
	    else if (TREE_CODE (TYPE_NAME (type)) == IDENTIFIER_NODE)
	      {
		/* Direct identifier case.  */
		name = IDENTIFIER_POINTER (TYPE_NAME (type));
	      }
	  }

	/* If no name found through normal extraction, handle anonymous types following Itanium C++ ABI.  */
	if (!name && !TYPE_NAME (type))
	  {
	    static char anon_name[128];

	    if (TREE_CODE (type) == UNION_TYPE)
	      {
		/* For anonymous unions, try to find first named field (Itanium ABI approach).  */
		tree field = TYPE_FIELDS (type);
		while (field && !DECL_NAME (field))
		  field = DECL_CHAIN (field);

		if (field && DECL_NAME (field))
		  {
		    const char *field_name = IDENTIFIER_POINTER (DECL_NAME (field));
		    snprintf (anon_name, sizeof(anon_name), "anon_union_by_%s", field_name);
		  }
		else
		  {
		    /* No named fields - use Itanium-style Ut encoding.  */
		    snprintf (anon_name, sizeof(anon_name), "Ut_unnamed_union");
		  }
	      }
	    else
	      {
		/* For anonymous structs/enums, use Itanium-style Ut encoding with layout info for discrimination.  */
		const char *type_prefix = "";
		if (TREE_CODE (type) == RECORD_TYPE)
		  type_prefix = "struct";
		else if (TREE_CODE (type) == ENUMERAL_TYPE)
		  type_prefix = "enum";

		/* Include size and field layout for better discrimination.  */
		HOST_WIDE_INT size = 0;
		if (TYPE_SIZE (type) && tree_fits_shwi_p (TYPE_SIZE (type)))
		  size = tree_to_shwi (TYPE_SIZE (type));

		/* Generate a hash based on field layout to distinguish same-sized anonymous types.  */
		unsigned layout_hash = 0;
		if (TREE_CODE (type) == RECORD_TYPE)
		  {
		    for (tree field = TYPE_FIELDS (type); field; field = DECL_CHAIN (field))
		      {
			if (TREE_CODE (field) == FIELD_DECL)
			  {
			    /* Hash field offset and type.  */
			    if (DECL_FIELD_OFFSET (field))
			      {
				HOST_WIDE_INT offset = tree_to_shwi (DECL_FIELD_OFFSET (field));
				layout_hash = layout_hash * 31 + (unsigned)offset;
			      }

			    /* Hash field type.  */
			    tree field_type = TREE_TYPE (field);
			    if (field_type && TYPE_MODE (field_type) != VOIDmode)
			      layout_hash = layout_hash * 37 + (unsigned)TYPE_MODE (field_type);
			  }
		      }
		  }

		if (layout_hash != 0)
		  snprintf (anon_name, sizeof(anon_name), "Ut_%s_%ld_%x", type_prefix, (long)size, layout_hash);
		else
		  snprintf (anon_name, sizeof(anon_name), "Ut_%s_%ld", type_prefix, (long)size);
	      }

	    name = anon_name;
	  }

	if (name)
	  {
	    *p += snprintf (*p, end - *p, "%zu%s", strlen (name), name);
	  }
	else
	  {
	    /* Always show diagnostic information for missing struct names.  */
	    error ("KCFI: No struct/union/enum name found for type code %<%E%> (%qs)",
		   TREE_CODE (type), get_tree_code_name (TREE_CODE (type)));
	    if (TYPE_NAME (type))
	      {
		error ("KCFI: %<TYPE_NAME%> exists but extraction failed");
		error ("KCFI: %<TYPE_NAME%> tree code = %<%E%>", TREE_CODE (TYPE_NAME (type)));
		if (TREE_CODE (TYPE_NAME (type)) == TYPE_DECL)
		  {
		    tree decl_name = DECL_NAME (TYPE_NAME (type));
		    error ("KCFI: %<TYPE_DECL%> %<DECL_NAME%> = %p", (void*)decl_name);
		    if (decl_name && TREE_CODE (decl_name) == IDENTIFIER_NODE)
		      error ("KCFI: %<IDENTIFIER_NODE%> name = '%s'", IDENTIFIER_POINTER (decl_name));
		  }
		else if (TREE_CODE (TYPE_NAME (type)) == IDENTIFIER_NODE)
		  {
		    error ("KCFI: %<IDENTIFIER_NODE%> name = '%s'", IDENTIFIER_POINTER (TYPE_NAME (type)));
		  }
		else
		  {
		    error ("KCFI: Unknown %<TYPE_NAME%> tree code %<%E%>", TREE_CODE (TYPE_NAME (type)));
		  }
	      }
	    else
	      {
		error ("KCFI: %<TYPE_NAME%> is NULL - anonymous struct/union/enum detected");
	      }

	    /* This indicates a missing case in our struct name extraction.  */
	    error ("KCFI: Please report this as a bug with the above diagnostic information");
	    gcc_unreachable ();
	  }
	break;
      }

    default:
      /* Handle builtin types.  */
      mangle_builtin_type_to_buffer (type, p, end);
      break;
    }
}

/* Compute canonical type name for KCFI type ID generation using Itanium C++ ABI mangling.
   Accepts either FUNCTION_DECL (preferred for typedef preservation) or FUNCTION_TYPE.  */
static const char *
get_canonical_type_name (tree fntype_or_fndecl)
{
  gcc_assert (fntype_or_fndecl);

  tree fndecl = NULL;
  tree fntype = NULL;

  /* Determine input type and extract function type.  */
  if (TREE_CODE (fntype_or_fndecl) == FUNCTION_DECL)
    {
      fndecl = fntype_or_fndecl;
      fntype = TREE_TYPE (fndecl);
    }
  else if (TREE_CODE (fntype_or_fndecl) == FUNCTION_TYPE)
    {
      fntype = fntype_or_fndecl;
      /* fndecl remains NULL - will trigger fallback to TYPE_ARG_TYPES.  */
    }
  else
    {
      gcc_unreachable (); /* Should only be called with FUNCTION_DECL or FUNCTION_TYPE.  */
    }

  static char name_buf[512];
  char *p = name_buf;
  char *end = name_buf + sizeof (name_buf) - 1;

  /* Handle simple FUNCTION_TYPE case - use mangle_type_to_buffer directly.  */
  if (!fndecl)
    {
      /* Use Itanium ABI function type mangling: F + return type + param types + E.  */
      mangle_type_to_buffer (fntype, &p, end);

      /* Ensure we didn't overflow the buffer.  */
      gcc_assert (p <= end);
      *p = '\0';
      return name_buf;
    }

  /* FUNCTION_DECL case - use enhanced processing for typedef preservation.  */

  /* Start with function type prefix.  */
  *p++ = 'F';

  /* Add return type using canonicalized FUNCTION_TYPE.  */
  mangle_type_to_buffer (TREE_TYPE (fntype), &p, end);

  /* Process parameters using original DECL_ARGUMENTS to preserve typedef names.  */
  tree parm = DECL_ARGUMENTS (fndecl);
  if (!parm)
    {
      /* No parameter declarations - use TYPE_ARG_TYPES but try to preserve typedef info.  */
      tree param_types = TYPE_ARG_TYPES (fntype);
      if (param_types && TREE_VALUE (param_types) == void_type_node)
	{
	  /* Explicit void parameter list - don't add anything.  */
	}
      else if (param_types)
	{
	  /* Process TYPE_ARG_TYPES for external declaration.  */
	  for (tree param = param_types; param && p < end; param = TREE_CHAIN (param))
	    {
	      tree param_type = TREE_VALUE (param);
	      if (param_type == void_type_node)
		break;

	      /* Use the parameter type from TYPE_ARG_TYPES.  */
	      mangle_type_to_buffer (param_type, &p, end);
	    }
	}
    }
  else
    {
      /* Process each parameter declaration.  */
      for (; parm && p < end; parm = DECL_CHAIN (parm))
	{
	  tree parm_type = TREE_TYPE (parm);

	  /* Use the original parameter type which may preserve typedef information.  */
	  mangle_type_to_buffer (parm_type, &p, end);
	}
    }

  /* End function type.  */
  *p++ = 'E';

  /* Ensure we didn't overflow the buffer.  */
  gcc_assert (p <= end);

  *p = '\0';
  return name_buf;
}

/* Compute KCFI type ID for a function declaration or function type (internal) */
static uint32_t
compute_kcfi_type_id (tree fntype_or_fndecl)
{
  if (!fntype_or_fndecl)
    return 0;

  const char *canonical_name = get_canonical_type_name (fntype_or_fndecl);
  uint32_t base_type_id = kcfi_hash_string (canonical_name);

  /* Apply target-specific masking if supported.  */
  if (kcfi_target.mask_type_id)
    return kcfi_target.mask_type_id (base_type_id);

  return base_type_id;
}

/* Check if a function needs KCFI preamble generation.
   ALL functions get preambles when -fsanitize=kcfi is enabled, regardless
   of no_sanitize("kcfi") attribute.  */
bool
function_needs_kcfi_preamble (tree fndecl)
{
  /* Only instrument if KCFI is globally enabled.  */
  if (!(flag_sanitize & SANITIZE_KCFI))
    return false;

  /* Check if function is truly address-taken using cgraph node analysis.  */
  struct cgraph_node *node = cgraph_node::get (fndecl);
  bool addr_taken = (node && node->address_taken);

  /* Only instrument functions that can be targets of indirect calls:
     - Public functions (can be called externally)
     - External declarations (from other modules)
     - Functions with true address-taken status from cgraph analysis.  */
  return TREE_PUBLIC (fndecl) || DECL_EXTERNAL (fndecl) || addr_taken;
}

/* Function attribute to store KCFI type ID.  */
static tree kcfi_type_id_attr = NULL_TREE;

/* Get KCFI type ID for a function declaration.  */
uint32_t
get_function_kcfi_type_id (tree fndecl)
{
  if (!kcfi_type_id_attr)
    kcfi_type_id_attr = get_identifier ("kcfi_type_id");

  tree attr = lookup_attribute_by_prefix ("kcfi_type_id", DECL_ATTRIBUTES (fndecl));
  if (attr && TREE_VALUE (attr) && TREE_VALUE (TREE_VALUE (attr)))
    {
      tree value = TREE_VALUE (TREE_VALUE (attr));
      if (TREE_CODE (value) == INTEGER_CST)
	return (uint32_t) TREE_INT_CST_LOW (value);
    }

  /* Compute and cache type ID using original parameter declarations.  */
  uint32_t type_id = compute_kcfi_type_id (fndecl);
  tree type_id_tree = build_int_cst (unsigned_type_node, type_id);
  tree attr_value = build_tree_list (NULL_TREE, type_id_tree);
  attr = build_tree_list (kcfi_type_id_attr, attr_value);

  DECL_ATTRIBUTES (fndecl) = chainon (DECL_ATTRIBUTES (fndecl), attr);

  return type_id;
}

/* Get the number of patchable prefix NOPs for the current function.  */
static HOST_WIDE_INT
get_current_function_patchable_prefix_nops (void)
{
  HOST_WIDE_INT prefix_nops = 0;

  /* Check for function-specific patchable_function_entry attribute.  */
  tree patchable_attr = lookup_attribute ("patchable_function_entry",
					 DECL_ATTRIBUTES (current_function_decl));
  if (patchable_attr)
    {
      tree pp_val = TREE_VALUE (patchable_attr);
      /* total_nops = tree_to_uhwi (TREE_VALUE (pp_val)); */
      if (TREE_CHAIN (pp_val))
	prefix_nops = tree_to_uhwi (TREE_VALUE (TREE_CHAIN (pp_val)));
    }
  else
    {
      /* Use global configuration if no function-specific attribute.  */
      HOST_WIDE_INT total_nops, patch_area_entry;
      parse_and_check_patch_area (flag_patchable_function_entry, false,
				  &total_nops, &patch_area_entry);
      prefix_nops = patch_area_entry;
    }

  return prefix_nops;
}

/* Check if this is an indirect call that needs KCFI instrumentation.  */
static bool
is_kcfi_indirect_call (tree fn)
{
  if (!fn)
    return false;

  /* Only functions WITHOUT no_sanitize("kcfi") should generate KCFI checks at
     indirect call sites.  */
  if (!sanitize_flags_p (SANITIZE_KCFI, current_function_decl))
    return false;

  /* Direct function calls via ADDR_EXPR don't need KCFI checks.  */
  if (TREE_CODE (fn) == ADDR_EXPR)
    return false;

  /* Function pointers, variables, and other indirect calls need KCFI.  */
  return true;
}

/* Extract target from call instruction RTL pattern.
   Handles the RTL pattern matching needed to find the rtx containing
   the function pointer for indirect calls.  */
static rtx
kcfi_find_call_target (rtx_insn *call_insn)
{
  if (!call_insn || (!CALL_P (call_insn) && !JUMP_P (call_insn)))
    return NULL_RTX;

  rtx call_pattern = PATTERN (call_insn);
  rtx target_reg = NULL_RTX;

  if (CALL_P (call_insn) && GET_CODE (call_pattern) == PARALLEL)
    {
      /* Handle PARALLEL patterns (need to extract CALL) */
      rtx call_part = XVECEXP (call_pattern, 0, 0);
      call_pattern = call_part;
    }

  if (CALL_P (call_insn) && GET_CODE (call_pattern) == CALL)
    {
      /* Handle CALL patterns.  */
      rtx mem = XEXP (call_pattern, 0);
      if (GET_CODE (mem) == MEM)
	{
	  target_reg = XEXP (mem, 0);
	}
    }
  else if (CALL_P (call_insn) && GET_CODE (call_pattern) == SET)
    {
      /* Handle SET patterns (function calls with return values) */
      rtx src = XEXP (call_pattern, 1);
      if (GET_CODE (src) == CALL)
	{
	  rtx mem = XEXP (src, 0);
	  if (GET_CODE (mem) == MEM)
	    {
	      target_reg = XEXP (mem, 0);
	    }
	}
    }
  else if (JUMP_P (call_insn) && GET_CODE (call_pattern) == SET)
    {
      /* Handle jump patterns.  */
      rtx src = SET_SRC (call_pattern);
      if (GET_CODE (src) == MEM)  /* Direct indirect jump.  */
	{
	  target_reg = XEXP (src, 0);
	}
    }

  return target_reg;
}

/* Add KCFI type ID note to call instruction.  */
void
add_kcfi_type_note (rtx_insn *call_insn, uint32_t type_id)
{
  if (!call_insn || !CALL_P (call_insn))
    return;

  /* Create integer constant for type ID.  */
  rtx type_id_rtx = gen_int_mode (type_id, SImode);

  /* Add note to call instruction.  */
  add_reg_note (call_insn, REG_CALL_KCFI_TYPE, type_id_rtx);
}

/* Global flag to coordinate KCFI preamble emission.  */
static bool kcfi_preamble_emitted = false;

/* Mark that KCFI preamble has been emitted to prevent duplication.  */
void
mark_kcfi_preamble_emitted (void)
{
  kcfi_preamble_emitted = true;
}

/* Emit KCFI type ID symbol for an address-taken function.
   Centralized emission point to avoid duplication between
   assemble_external_real() and assemble_start_function(). */
void
emit_kcfi_typeid_symbol (FILE *asm_file, tree decl, const char *name)
{
  uint32_t type_id = get_function_kcfi_type_id (decl);
  fprintf (asm_file, "\t.weak\t__kcfi_typeid_%s\n", name);
  fprintf (asm_file, "\t.set\t__kcfi_typeid_%s, 0x%08x\n", name, type_id);
}

/* Parse patchable function entry configuration from global flags.  */
static bool
parse_patchable_function_entry_config (void)
{
  HOST_WIDE_INT total_nops, prefix_nops;

  if (!flag_patchable_function_entry)
    return false;

  parse_and_check_patch_area (flag_patchable_function_entry, false, &total_nops, &prefix_nops);
  return (total_nops > 0);
}

/* Common KCFI preamble helper that handles shared logic across architectures.  */
static void
kcfi_emit_cfi_preamble (FILE *file, tree decl, HOST_WIDE_INT prefix_nops, const char *fname)
{
  /* Get type ID.  */
  uint32_t type_id = get_function_kcfi_type_id (decl);

  /* Create symbol name for reuse.  */
  char cfi_symbol_name[256];
  snprintf (cfi_symbol_name, sizeof(cfi_symbol_name), "__cfi_%s", fname);

  /* Emit __cfi_ symbol with proper visibility.  */
  if (TREE_PUBLIC (decl))
    {
      if (DECL_WEAK (decl))
	ASM_WEAKEN_LABEL (file, cfi_symbol_name);
      else
	targetm.asm_out.globalize_label (file, cfi_symbol_name);
    }

  /* Emit .type directive.  */
  ASM_OUTPUT_TYPE_DIRECTIVE (file, cfi_symbol_name, "function");
  fprintf (file, "%s:\n", cfi_symbol_name);

  /* Calculate architecture-specific prefix NOPs if hook provided.  */
  int final_nops = prefix_nops;
  if (kcfi_target.calculate_prefix_nops)
    final_nops = kcfi_target.calculate_prefix_nops (prefix_nops);

  /* Emit architecture-specific prefix NOPs.  */
  for (int i = 0; i < final_nops; i++)
    {
      fprintf (file, "\tnop\n");
    }

  /* Emit architecture-specific type ID instruction.  */
  if (kcfi_target.emit_type_id_instruction)
    kcfi_target.emit_type_id_instruction (file, type_id);

  /* Mark end of __cfi_ symbol and emit size directive.  */
  char cfi_end_label[256];
  snprintf (cfi_end_label, sizeof(cfi_end_label), ".Lcfi_func_end_%s", fname);
  ASM_OUTPUT_LABEL (file, cfi_end_label);

  ASM_OUTPUT_MEASURED_SIZE (file, cfi_symbol_name);
}

void
kcfi_emit_preamble_if_needed (FILE *file, tree decl,
			      bool is_patchable_context,
			      HOST_WIDE_INT prefix_nops,
			      const char *actual_fname)
{
  /* Check if KCFI is enabled and function needs preamble.  */
  if (!function_needs_kcfi_preamble (decl))
    return;

  /* Use actual function name if provided, otherwise fall back to DECL_ASSEMBLER_NAME.  */
  const char *fname = actual_fname ? actual_fname : IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (decl));

  /* Determine if we should emit here based on context.  */
  bool has_patchable_entries = parse_patchable_function_entry_config ();

  if (is_patchable_context && has_patchable_entries)
    {
      /* Use the provided prefix_nops parameter.  */
    }
  else if (!is_patchable_context && !has_patchable_entries)
    {
      /* Emit for standard non-patchable functions.  */
      prefix_nops = 0;
    }
  else
    {
      /* Skip to avoid duplication:
	 - Patchable context but no global patchable config
	 - Non-patchable context but has global patchable config.  */
      return;
    }

  kcfi_emit_cfi_preamble (file, decl, prefix_nops, fname);
}

/* KCFI Final Instrumentation Pass - Post-Optimization Implementation

   This pass runs after all optimizations to insert KCFI checks immediately
   before indirect calls/jumps, solving the fundamental problem where
   optimization would insert code between KCFI checks and their protected
   indirect transfers.  */

namespace {

const pass_data pass_data_kcfi_final_instrumentation =
{
  RTL_PASS,                    /* type */
  "kcfi_final_instrumentation", /* name */
  OPTGROUP_NONE,              /* optinfo_flags */
  TV_NONE,                    /* tv_id */
  0,                          /* properties_required */
  0,                          /* properties_provided */
  0,                          /* properties_destroyed */
  0,                          /* todo_flags_start */
  0,                          /* todo_flags_finish */
};

class pass_kcfi_final_instrumentation : public rtl_opt_pass
{
public:
  pass_kcfi_final_instrumentation (gcc::context *ctxt)
    : rtl_opt_pass (pass_data_kcfi_final_instrumentation, ctxt)
  {}

  /* opt_pass methods: */
  bool gate (function *) final override
  {
    /* Note: Target KCFI support is validated in toplev.cc during option processing.
       If the target doesn't provide gen_kcfi_checked_call, compilation stops with
       a "sorry" message before any passes run.  */
    return sanitize_flags_p (SANITIZE_KCFI);
  }

  unsigned int execute (function *) final override;
};

/* Check if an RTL instruction is an indirect call/jump marked for KCFI.  */
static bool
is_marked_indirect_transfer (rtx_insn *insn, uint32_t *type_id)
{
  if (!insn || (!CALL_P (insn) && !JUMP_P (insn)))
    return false;

  /* Check for KCFI type ID note.  */
  rtx note = find_reg_note (insn, REG_CALL_KCFI_TYPE, NULL_RTX);
  if (!note)
    return false;

  rtx type_rtx = XEXP (note, 0);
  if (!CONST_INT_P (type_rtx))
    return false;

  *type_id = (uint32_t) INTVAL (type_rtx);

  /* Verify it's actually an indirect transfer.  */
  rtx pattern = PATTERN (insn);

  if (CALL_P (insn))
    {
      /* Handle call patterns.  */
      if (GET_CODE (pattern) == CALL)
	{
	  rtx target = XEXP (pattern, 0);
	  return GET_CODE (target) == MEM;
	}
      else if (GET_CODE (pattern) == SET)
	{
	  rtx src = SET_SRC (pattern);
	  if (GET_CODE (src) == CALL)
	    {
	      rtx target = XEXP (src, 0);
	      return GET_CODE (target) == MEM;
	    }
	}
      else if (GET_CODE (pattern) == PARALLEL)
	{
	  for (int i = 0; i < XVECLEN (pattern, 0); i++)
	    {
	      rtx elem = XVECEXP (pattern, 0, i);
	      if (GET_CODE (elem) == CALL
		  || (GET_CODE (elem) == SET && GET_CODE (SET_SRC (elem)) == CALL))
		{
		  rtx call_rtx = (GET_CODE (elem) == SET) ? SET_SRC (elem) : elem;
		  rtx target = XEXP (call_rtx, 0);
		  return GET_CODE (target) == MEM;
		}
	    }
	}
    }
  else if (JUMP_P (insn))
    {
      /* Handle jump patterns.  */
      if (GET_CODE (pattern) == SET)
	{
	  rtx src = SET_SRC (pattern);
	  if (GET_CODE (src) == MEM)  /* Direct indirect jump.  */
	    return true;
	  /* Could also handle other indirect jump patterns here.  */
	}
    }

  return false;
}

/* Replace marked indirect transfer with KCFI-protected version.  */
unsigned int
pass_kcfi_final_instrumentation::execute (function *fun)
{
  basic_block bb;

  /* Scan all basic blocks for marked indirect transfers.  */
  FOR_EACH_BB_FN (bb, fun)
    {
      rtx_insn *insn, *next_insn;

      /* Use safe iteration since we'll be modifying the instruction stream.  */
      for (insn = BB_HEAD (bb); insn && insn != NEXT_INSN (BB_END (bb)); insn = next_insn)
	{
	  next_insn = NEXT_INSN (insn);

	  uint32_t type_id;
	  if (is_marked_indirect_transfer (insn, &type_id))
	    {
	      /* Extract target address from the call instruction.  */
	      rtx target_addr = kcfi_find_call_target (insn);
	      if (!target_addr)
		{
		  error ("KCFI: cannot find target for indirect call");
		  gcc_unreachable ();
		}

	      HOST_WIDE_INT prefix_nops = get_current_function_patchable_prefix_nops ();

	      /* Generate bundled KCFI checked call.  */
	      start_sequence ();

	      /* Pass the target as-is to the RTL pattern, which will handle
		 moving non-register targets to a register internally if needed.  */
	      rtx bundled_call = kcfi_target.gen_kcfi_checked_call (insn, target_addr, type_id, prefix_nops);
	      if (!bundled_call)
		{
		  error ("KCFI: instruction sequence creation failed");
		  gcc_unreachable ();
		}

	      /* Emit as call_insn, not generic insn.  */
	      emit_call_insn (bundled_call);
	      rtx replacement_seq = get_insns ();
	      if (!replacement_seq)
		{
		  error ("KCFI: instruction sequence insertion failed");
		  gcc_unreachable ();
		}

	      end_sequence ();

	      /* Check if original was a sibling call and preserve that flag.  */
	      bool was_sibcall = CALL_P (insn) && SIBLING_CALL_P (insn);

	      /* Replace the original call entirely with bundled version.  */
	      rtx_insn *new_insn = emit_insn_before (replacement_seq, insn);

	      /* If original was a sibling call, mark the new instruction as such.  */
	      if (was_sibcall && new_insn && CALL_P (new_insn))
		SIBLING_CALL_P (new_insn) = 1;

	      set_insn_deleted (insn);  /* Mark original call as deleted.  */
	    }
	}
    }

  return 0;
}

} // anon namespace

rtl_opt_pass *
make_pass_kcfi_final_instrumentation (gcc::context *ctxt)
{
  return new pass_kcfi_final_instrumentation (ctxt);
}

/* KCFI GIMPLE pass implementation.  */

static bool
gate_kcfi (void)
{
  return sanitize_flags_p (SANITIZE_KCFI);
}

/* Create a KCFI wrapper function type that embeds the type ID.  */
static tree
create_kcfi_wrapper_type (tree original_fn_type, uint32_t type_id)
{
  /* Create a unique type name incorporating the type ID.  */
  char wrapper_name[64];
  snprintf (wrapper_name, sizeof (wrapper_name), "__kcfi_wrapper_%x", type_id);

  /* Build a new function type that's structurally identical but nominally different.  */
  tree wrapper_type = build_function_type (TREE_TYPE (original_fn_type),
					   TYPE_ARG_TYPES (original_fn_type));

  /* Set the type name to make it distinct.  */
  TYPE_NAME (wrapper_type) = get_identifier (wrapper_name);

  /* Store the type ID as an attribute for later retrieval.  */
  tree attr_name = get_identifier ("kcfi_type_id");
  tree attr_value = build_int_cst (unsigned_type_node, type_id);
  tree attr = build_tree_list (attr_name, attr_value);
  TYPE_ATTRIBUTES (wrapper_type) = attr;

  return wrapper_type;
}

/* Replace indirect calls with KCFI wrapper types for anti-merging and clobber tracking.  */
static unsigned int
kcfi_instrument (void)
{
  basic_block bb;

  FOR_EACH_BB_FN (bb, cfun)
    {
      gimple_stmt_iterator gsi;
      for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
	{
	  gimple *stmt = gsi_stmt (gsi);

	  if (is_gimple_call (stmt))
	    {
	      gcall *call_stmt = as_a <gcall *> (stmt);

	      // Skip internal calls - we only instrument indirect calls
	      if (gimple_call_internal_p (call_stmt))
		{
		  continue;
		}

	      tree fndecl = gimple_call_fndecl (call_stmt);

	      // Only process indirect calls (no fndecl)
	      if (!fndecl)
		{
		  tree fn = gimple_call_fn (call_stmt);
		  if (fn && is_kcfi_indirect_call (fn))
		    {
		      // Get the function type to compute KCFI type ID
		      tree fn_type = TREE_TYPE (TREE_TYPE (fn));
		      if (fn_type && TREE_CODE (fn_type) == FUNCTION_TYPE)
			{
			  uint32_t type_id = compute_kcfi_type_id (fn_type);

			  // Create KCFI wrapper type for this call
			  tree wrapper_type = create_kcfi_wrapper_type (fn_type, type_id);

			  // Create a temporary variable for the wrapped function pointer
			  tree wrapper_ptr_type = build_pointer_type (wrapper_type);
			  tree wrapper_tmp = create_tmp_var (wrapper_ptr_type, "kcfi_wrapper");

			  // Create assignment: wrapper_tmp = (wrapper_ptr_type) fn
			  tree cast_expr = build1 (NOP_EXPR, wrapper_ptr_type, fn);
			  gimple *cast_stmt = gimple_build_assign (wrapper_tmp, cast_expr);
			  gsi_insert_before (&gsi, cast_stmt, GSI_SAME_STMT);

			  // Update the call to use the wrapped function pointer
			  gimple_call_set_fn (call_stmt, wrapper_tmp);
			}
		    }
		}
	    }
	}
    }

  return 0;
}

namespace {

const pass_data pass_data_kcfi =
{
  GIMPLE_PASS, /* type */
  "kcfi", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_NONE, /* tv_id */
  ( PROP_ssa | PROP_cfg | PROP_gimple_leh ), /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  TODO_update_ssa, /* todo_flags_finish */
};

class pass_kcfi : public gimple_opt_pass
{
public:
  pass_kcfi (gcc::context *ctxt)
    : gimple_opt_pass (pass_data_kcfi, ctxt)
  {}

  /* opt_pass methods: */
  opt_pass * clone () final override { return new pass_kcfi (m_ctxt); }
  bool gate (function *) final override
  {
    return gate_kcfi ();
  }
  unsigned int execute (function *) final override
  {
    return kcfi_instrument ();
  }

}; // class pass_kcfi

} // anon namespace

gimple_opt_pass *
make_pass_kcfi (gcc::context *ctxt)
{
  return new pass_kcfi (ctxt);
}

namespace {

const pass_data pass_data_kcfi_O0 =
{
  GIMPLE_PASS, /* type */
  "kcfi0", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_NONE, /* tv_id */
  ( PROP_ssa | PROP_cfg | PROP_gimple_leh ), /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  TODO_update_ssa, /* todo_flags_finish */
};

class pass_kcfi_O0 : public gimple_opt_pass
{
public:
  pass_kcfi_O0 (gcc::context *ctxt)
    : gimple_opt_pass (pass_data_kcfi_O0, ctxt)
  {}

  /* opt_pass methods: */
  bool gate (function *) final override
    {
      return !optimize && gate_kcfi ();
    }
  unsigned int execute (function *) final override
  {
    return kcfi_instrument ();
  }

}; // class pass_kcfi_O0

} // anon namespace

gimple_opt_pass *
make_pass_kcfi_O0 (gcc::context *ctxt)
{
  return new pass_kcfi_O0 (ctxt);
}
