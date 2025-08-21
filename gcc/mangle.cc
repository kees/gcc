/* Itanium C++ ABI type mangling for GCC.
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
#include "tree.h"
#include "diagnostic-core.h"
#include "stringpool.h"
#include "stor-layout.h"
#include "mangle.h"
#include "selftest.h"
#include "dumpfile.h"
#include "print-tree.h"

/* Current function context for better error reporting.  */
static tree current_function_context = NULL_TREE;

/* Helper to update FNV-1a hash with a single character.  */
static inline void
fnv1a_hash_char (uint32_t *hash_state, unsigned char c)
{
  *hash_state ^= c;
  *hash_state *= 16777619U; /* FNV-1a 32-bit prime.  */
}

/* Helper to append character to optional string and update hash using FNV-1a.  */
static void
append_char (char c, std::string *out_str, uint32_t *hash_state)
{
  if (out_str)
    *out_str += c;
  fnv1a_hash_char (hash_state, (unsigned char) c);
}

/* Helper to append string to optional string and update hash using FNV-1a.  */
static void
append_string (const char *str, std::string *out_str, uint32_t *hash_state)
{
  if (out_str)
    *out_str += str;
  for (const char *p = str; *p; p++)
    fnv1a_hash_char (hash_state, (unsigned char) *p);
}

/* Forward declaration for recursive type mangling.  */
static void mangle_type (tree type, std::string *out_str, uint32_t *hash_state);

/* Mangle a builtin type following Itanium C++ ABI for C types.  */
static void
mangle_builtin_type (tree type, std::string *out_str, uint32_t *hash_state)
{
  gcc_assert (type != NULL_TREE);

  switch (TREE_CODE (type))
    {
    case VOID_TYPE:
      append_char ('v', out_str, hash_state);
      return;

    case BOOLEAN_TYPE:
      append_char ('b', out_str, hash_state);
      return;

    case INTEGER_TYPE:
      /* Handle standard integer types using Itanium ABI codes.  */
      if (type == char_type_node)
	append_char ('c', out_str, hash_state);
      else if (type == signed_char_type_node)
	append_char ('a', out_str, hash_state);
      else if (type == unsigned_char_type_node)
	append_char ('h', out_str, hash_state);
      else if (type == short_integer_type_node)
	append_char ('s', out_str, hash_state);
      else if (type == short_unsigned_type_node)
	append_char ('t', out_str, hash_state);
      else if (type == integer_type_node)
	append_char ('i', out_str, hash_state);
      else if (type == unsigned_type_node)
	append_char ('j', out_str, hash_state);
      else if (type == long_integer_type_node)
	append_char ('l', out_str, hash_state);
      else if (type == long_unsigned_type_node)
	append_char ('m', out_str, hash_state);
      else if (type == long_long_integer_type_node)
	append_char ('x', out_str, hash_state);
      else if (type == long_long_unsigned_type_node)
	append_char ('y', out_str, hash_state);
      else
	{
	  /* Fallback for other integer types - use precision-based encoding.  */
	  append_char ('i', out_str, hash_state);
	  append_string (std::to_string (TYPE_PRECISION (type)).c_str (), out_str, hash_state);
	}
      return;

    case REAL_TYPE:
      if (type == float_type_node)
	append_char ('f', out_str, hash_state);
      else if (type == double_type_node)
	append_char ('d', out_str, hash_state);
      else if (type == long_double_type_node)
	append_char ('e', out_str, hash_state);
      else
	{
	  /* Fallback for other real types.  */
	  append_char ('f', out_str, hash_state);
	  append_string (std::to_string (TYPE_PRECISION (type)).c_str (), out_str, hash_state);
	}
      return;

    case VECTOR_TYPE:
      {
	/* Handle vector types following Itanium C++ ABI:
	   Dv<num-elements>_<element-type-encoding>
	   Example: uint8x16_t → Dv16_h (vector of 16 unsigned char)  */
	tree vector_size = TYPE_SIZE_UNIT (type);
	tree element_type = TREE_TYPE (type);
	tree element_size = TYPE_SIZE_UNIT (element_type);

	if (vector_size && element_size &&
	    TREE_CODE (vector_size) == INTEGER_CST &&
	    TREE_CODE (element_size) == INTEGER_CST)
	  {
	    append_char ('D', out_str, hash_state);
	    append_char ('v', out_str, hash_state);

	    unsigned HOST_WIDE_INT vec_bytes = tree_to_uhwi (vector_size);
	    unsigned HOST_WIDE_INT elem_bytes = tree_to_uhwi (element_size);
	    unsigned HOST_WIDE_INT num_elements = vec_bytes / elem_bytes;

	    /* Append number of elements.  */
	    append_string (std::to_string (num_elements).c_str (), out_str, hash_state);
	    append_char ('_', out_str, hash_state);

	    /* Recursively mangle the element type.  */
	    mangle_type (element_type, out_str, hash_state);
	    return;
	  }
	/* Fail for vectors with unknown size.  */
      }
      break;

    default:
      break;
    }

  /* Unknown builtin type - this should never happen in a well-formed C program.  */
  debug_tree (type);
  internal_error ("mangle: Unknown builtin type in function %qD - please report this as a bug",
                  current_function_context);
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
mangle_type (tree type, std::string *out_str, uint32_t *hash_state)
{
  gcc_assert (type != NULL_TREE);

  /* Canonicalize typedef types to their underlying named struct types.  */
  type = canonicalize_typedef_type (type);

  switch (TREE_CODE (type))
    {
    case POINTER_TYPE:
      {
	/* Pointer type: 'P' + qualifiers + pointed-to type.  */
	append_char ('P', out_str, hash_state);

	/* Add qualifiers to the pointed-to type following Itanium C++ ABI ordering.  */
	tree pointed_to_type = TREE_TYPE (type);
	if (TYPE_QUALS (pointed_to_type) != TYPE_UNQUALIFIED)
	  {
	    /* Emit qualifiers in Itanium ABI order: restrict, volatile, const.  */
	    if (TYPE_QUALS (pointed_to_type) & TYPE_QUAL_RESTRICT)
	      append_char ('r', out_str, hash_state);
	    if (TYPE_QUALS (pointed_to_type) & TYPE_QUAL_VOLATILE)
	      append_char ('V', out_str, hash_state);
	    if (TYPE_QUALS (pointed_to_type) & TYPE_QUAL_CONST)
	      append_char ('K', out_str, hash_state);
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
	mangle_type (target_type, out_str, hash_state);
	break;
      }

    case ARRAY_TYPE:
      /* Array type: 'A' + size + '_' + element type (simplified).  */
      append_char ('A', out_str, hash_state);
      if (TYPE_DOMAIN (type) && TYPE_MAX_VALUE (TYPE_DOMAIN (type)))
	{
	  tree max_val = TYPE_MAX_VALUE (TYPE_DOMAIN (type));
	  /* Check if array size is a compile-time constant to handle VLAs safely.  */
	  if (TREE_CODE (max_val) == INTEGER_CST && tree_fits_shwi_p (max_val))
	    {
	      HOST_WIDE_INT size = tree_to_shwi (max_val) + 1;
	      append_string (std::to_string ((long) size).c_str (), out_str, hash_state);
	    }
	  /* For VLAs or non-constant dimensions, emit empty size (A_).  */
	  append_char ('_', out_str, hash_state);
	}
      else
	{
	  /* No domain or no max value - emit A_.  */
	  append_char ('_', out_str, hash_state);
	}
      mangle_type (TREE_TYPE (type), out_str, hash_state);
      break;

    case REFERENCE_TYPE:
      /* Reference type: 'R' + referenced type.
         Note: We must handle references to builtin types including compiler
         builtins like __builtin_va_list used in functions like va_start. */
      append_char ('R', out_str, hash_state);
      mangle_type (TREE_TYPE (type), out_str, hash_state);
      break;

    case FUNCTION_TYPE:
      {
	/* Function type: 'F' + return type + parameter types + 'E' */
	append_char ('F', out_str, hash_state);
	mangle_type (TREE_TYPE (type), out_str, hash_state);

	/* Add parameter types.  */
	tree param_types = TYPE_ARG_TYPES (type);

	if (param_types == NULL_TREE)
	  {
	    /* func() - no parameter list (could be variadic). */
	  }
	else
	  {
	    bool found_real_params = false;
	    for (tree param = param_types; param; param = TREE_CHAIN (param))
	      {
		tree param_type = TREE_VALUE (param);
		if (param_type == void_type_node)
		  {
		    /* Check if this is the first parameter (explicit void) or a sentinel */
		    if (!found_real_params)
		      {
			/* func(void) - explicit empty parameter list.
			   Mangle void to distinguish from variadic func(). */
			mangle_type (void_type_node, out_str, hash_state);
		      }
		    /* If we found real params before this void, it's a sentinel - stop */
		    break;
		  }

		found_real_params = true;

		/* For value parameters, ignore const/volatile qualifiers as they
		   don't affect the calling convention.  const int and int are
		   passed identically by value.  */
		tree canonical_param_type = param_type;
		if (TREE_CODE (param_type) != POINTER_TYPE
		    && TREE_CODE (param_type) != REFERENCE_TYPE
		    && TREE_CODE (param_type) != ARRAY_TYPE)
		  {
		    /* Strip qualifiers for non-pointer/reference value parameters.  */
		    canonical_param_type = TYPE_MAIN_VARIANT (param_type);
		  }

		mangle_type (canonical_param_type, out_str, hash_state);
	      }
	  }

	/* Check if this is a variadic function and add 'z' marker.  */
	if (stdarg_p (type))
	  {
	    append_char ('z', out_str, hash_state);
	  }

	append_char ('E', out_str, hash_state);
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
		/* For anonymous structs/enums, use Itanium-style Ut encoding
		   with layout info for discrimination.  */
		const char *type_prefix = "";
		if (TREE_CODE (type) == RECORD_TYPE)
		  type_prefix = "struct";
		else if (TREE_CODE (type) == ENUMERAL_TYPE)
		  type_prefix = "enum";

		/* Include size and field layout for better discrimination.  */
		HOST_WIDE_INT size = 0;
		if (TYPE_SIZE (type) && tree_fits_shwi_p (TYPE_SIZE (type)))
		  size = tree_to_shwi (TYPE_SIZE (type));

		/* Generate a hash based on field layout to distinguish same-sized
		   anonymous types.  */
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
		  snprintf (anon_name, sizeof(anon_name), "Ut_%s_%ld_%x",
			    type_prefix, (long)size, layout_hash);
		else
		  snprintf (anon_name, sizeof(anon_name), "Ut_%s_%ld",
			    type_prefix, (long)size);
	      }

	    name = anon_name;
	  }

	if (name)
	  {
	    append_string (std::to_string (strlen (name)).c_str (), out_str, hash_state);
	    append_string (name, out_str, hash_state);
	  }
	else
	  {
	    /* Always show diagnostic information for missing struct names.  */
	    debug_tree (type);
	    internal_error ("mangle: Missing case in struct name extraction - please report this as a bug");
	  }
	break;
      }

    default:
      /* Handle builtin types.  */
      mangle_builtin_type (type, out_str, hash_state);
      break;
    }
}

/* Compute canonical function type hash using Itanium C++ ABI mangling.  */
uint32_t
hash_function_type (tree fntype, tree fndecl)
{
  gcc_assert (fntype);
  gcc_assert (TREE_CODE (fntype) == FUNCTION_TYPE);

  std::string result;
  std::string *out_str = nullptr;
  uint32_t hash_state = 2166136261U; /* FNV-1a 32-bit offset basis.  */

  /* Only build string if dump is active.  */
  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      result.reserve (32);
      out_str = &result;
    }

  /* Store function context for error reporting.  */
  current_function_context = fndecl;

  /* Typeinfo for a function prototype.  */
  append_string ("_ZTS", out_str, &hash_state);

  mangle_type (fntype, out_str, &hash_state);

  /* Clear function context.  */
  current_function_context = NULL_TREE;

  /* Output to dump file if enabled.  */
  if (dump_file && (dump_flags & TDF_DETAILS))
    {
      fprintf (dump_file, "KCFI type ID: mangled='%s' typeid=0x%08x\n",
	       result.c_str (), hash_state);
    }

  return hash_state;
}
