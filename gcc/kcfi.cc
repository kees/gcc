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

/* KCFI ABI Design:

The Linux Kernel Control Flow Integrity ABI provides a function prototype
based forward edge control flow integrity protection by instrumenting
every indirect call to check for a hash value before the target function
address. If the hash at the call site and the hash at the target do not
match, execution will trap.

The general CFI ideas are discussed here, but focuses more on a CFG
analysis to construct valid call destinations, which tends to require LTO:
https://users.soe.ucsc.edu/~abadi/Papers/cfi-tissec-revised.pdf

Later refinement for using jump tables (constructed via CFG analysis
during LTO) was proposed here:
https://www.usenix.org/system/files/conference/usenixsecurity14/sec14-paper-tice.pdf

Linux used the above implementation from 2018 to 2022:
https://android-developers.googleblog.com/2018/10/control-flow-integrity-in-android-kernel.html
but the corner cases for target addresses not being the actual functions
(i.e. pointing into the jump table) was a continual source of problems,
and generating the jump tables required full LTO, which had its own set
of problems.

Looking at function prototypes as the source of call validity was
presented here, though still relied on LTO:
https://www.blackhat.com/docs/asia-17/materials/asia-17-Moreira-Drop-The-Rop-Fine-Grained-Control-Flow-Integrity-For-The-Linux-Kernel-wp.pdf

The KCFI approach built on the function-prototype idea, but avoided
needing LTO, and could be further updated to deal with CPU errata
(retpolines, etc):
https://lpc.events/event/16/contributions/1315/

KCFI has a number of specific constraints. Some are tied to the
backend architecture, which are covered in arch-specific code.
The constraints are:

- The KCFI scheme generates a unique 32-bit hash for each unique function
  prototype, allowing for indirect call sites to verify that they are
  calling into a matching _type_ of function pointer. This changes the
  semantics of some optimization logic because now indirect calls to
  different types cannot be merged. For example:

    if (p->func_type_1)
	return p->func_type_1();
    if (p->func_type_2)
	return p->func_type_2();

  In final asm, the optimizer may collapse the second indirect call
  into a jump to the first indirect call once it has loaded the function
  pointer. KCFI must block cross-type merging otherwise there will be a
  single KCFI check happening for only 1 type but being used by 2 target
  types. The distinguishing characteristic for call merging becomes the
  type, not the address/register usage.

- The check-call instruction sequence must be treated a single unit: it
  cannot be rearranged or split or optimized. The pattern is that
  indirect calls, "call *$target", get converted into:

    mov $target_expression, %target ; only present if the expression was
                                    ; not already %target register
    load -$offset(%target), %tmp    ; load the typeid hash at target
    cmp $hash, %tmp                 ; compare expected typeid with loaded
    je .Lcheck_passed               ; jump to the indirect call
  .Lkcfi_trap$N:                    ; label of trap insn
    trap                            ; trap on failure, but arranged so
                                    ; "permissive mode" falls through
  .Lkcfi_call$N:                    ; label of call insn
    call *%target                   ; actual indirect call

  This pattern of call immediately after trap provides for the
  "permissive" checking mode automatically: the trap gets handled,
  a warning emitted, and then execution continues after the trap to
  the call.

- KCFI check-call instrumentation must survive tail call optimization.
  If an indirect call is turned into an indirect jump, KCFI checking
  must still happen (but will still use the jmp).

- Functions that may be called indirectly have a preamble added,
  __cfi_$original_func_name, which contains the $hash value:

    __cfi_target_func:
      .word $hash
    target_func:
       [regular function entry...]

- The preamble needs to interact with patchable function entry so that
  the hash appears further away from the actual start of the function
  (leaving the prefix NOPs of the patchable function entry unchanged).
  This means only _globally defined_ patchable function entry is supported
  with KCFI (indrect call sites must know in advance what the offset is,
  which may not be possible with extern functions). For example, a "4,4"
  patchable function entry would end up like:

    __cfi_target_func:
      .data $hash
      nop nop nop nop
    target_func:
       [regular function entry...]

  Architectures may need to add alignment nops prior to the hash to keep things
  aligned for function call conventions.

- External functions that are address-taken have a weak __kcfi_typeid_$funcname
  symbol added with the hash value available so that the hash can be referenced
  from assembly linkages, etc, where the hash values cannot be calculated (i.e
  where C type information is missing):

    .weak   __kcfi_typeid_$func
    .set    __kcfi_typeid_$func, $hash

- On architectures that do not have a good way to encode additional
  details in their trap insn (e.g. x86_64 and riscv64), the trap location
  is identified as a KCFI trap via a relative address offset entry
  emitted into the .kcfi_traps section for each indirect call site's
  trap instruction. The previous check-call example's insn sequence has
  a section push/pop inserted between the trap and call:

  ...
  .Lkcfi_trap$N:
    trap
  .section	.kcfi_traps,"ao",@progbits,.text
    .Lkcfi_entry$N:
        .long	.Lkcfi_trap$N - .Lkcfi_entry$N
  .text
  .Lkcfi_call$N:
    call *%target

  For architectures that can encode immediates in their trap function
  (e.g. aarch64 and arm32), this isn't needed: they just use immediate
  codes that indicate a KCFI trap.

- The no_sanitize("kcfi") function attribute means that the marked
  function must not produce KCFI checking for indirect calls, and this
  attribute must survive inlining. This is used rarely by Linux, but
  is required to make BPF JIT trampolines work on older Linux kernel
  versions.

As a result of these constraints, there are some behavioral aspects
that need to be preserved across the middle-end and back-end.

For indirect call sites:

- Keeping indirect calls from being merged (see above) by adding a
  wrapping type so that equality was tested based on type-id.

- Keeping typeid information available through to the RTL expansion
  phase was done via a new KCFI insn that wraps CALL and the typeid.

- To make sure KCFI expansion is skipped for inline functions, the
  inlining is marked during GIMPLE with a new flag which is checked
  during expansion.

For indirect call targets:

- kcfi_emit_preamble() uses function_needs_kcfi_preamble(),
  to emit the preablem, which interacts with patchable function
  entry to add any needed alignment.

- gcc/varasm.cc, assemble_external_real() calls emit_kcfi_typeid_symbol()
  to add the __kcfi_typeid symbols (see get_function_kcfi_type_id()
  below).

*/

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "target.h"
#include "function.h"
#include "tree.h"
#include "tree-pass.h"
#include "dumpfile.h"
#include "basic-block.h"
#include "gimple.h"
#include "gimple-iterator.h"
#include "cgraph.h"
#include "kcfi.h"
#include "stringpool.h"
#include "attribs.h"
#include "rtl.h"
#include "cfg.h"
#include "cfgrtl.h"
#include "asan.h"
#include "diagnostic-core.h"
#include "memmodel.h"
#include "print-tree.h"
#include "emit-rtl.h"
#include "output.h"
#include "builtins.h"
#include "varasm.h"
#include "opts.h"
#include "mangle.h"
#include "target.h"
#include "flags.h"

HOST_WIDE_INT kcfi_patchable_entry_prefix_nops = 0;  /* For callsite offset */
static HOST_WIDE_INT kcfi_patchable_entry_arch_alignment_nops = 0;  /* For preamble alignment */

/* Common helper for RTL patterns to emit .kcfi_traps section entry.  */
void
kcfi_emit_traps_section (FILE *file, rtx trap_label_sym)
{
  /* Generate entry label internally and get its number.  */
  rtx entry_label = gen_label_rtx ();
  int entry_labelno = CODE_LABEL_NUMBER (entry_label);

  /* Generate entry label name with custom prefix.  */
  char entry_name[32];
  ASM_GENERATE_INTERNAL_LABEL (entry_name, "Lkcfi_entry", entry_labelno);

  /* Save current section to restore later.  */
  section *saved_section = in_section;

  /* Use varasm infrastructure for section handling.  */
  section *kcfi_traps_section = get_section (".kcfi_traps",
					     SECTION_LINK_ORDER, NULL);
  switch_to_section (kcfi_traps_section);

  /* Emit entry label.  */
  ASM_OUTPUT_LABEL (file, entry_name);

  /* Generate address difference using RTL infrastructure.  */
  rtx entry_label_sym = gen_rtx_SYMBOL_REF (Pmode, entry_name);
  rtx addr_diff = gen_rtx_MINUS (Pmode, trap_label_sym, entry_label_sym);

  /* Emit the address difference as a 4-byte value.  */
  assemble_integer (addr_diff, 4, BITS_PER_UNIT, 1);

  /* Restore the previous section.  */
  switch_to_section (saved_section);
}

/* Compute KCFI type ID for a function declaration or function type (internal) */
static uint32_t
compute_kcfi_type_id (tree fntype, tree fndecl = NULL_TREE)
{
  gcc_assert (fntype);
  gcc_assert (TREE_CODE (fntype) == FUNCTION_TYPE);

  uint32_t type_id = hash_function_type (fntype, fndecl);

  /* Apply target-specific masking if supported.  */
  if (targetm.kcfi.mask_type_id)
    type_id = targetm.kcfi.mask_type_id (type_id);

  return type_id;
}

/* Check if a function needs KCFI preamble generation.
   ALL functions get preambles when -fsanitize=kcfi is enabled, regardless
   of no_sanitize("kcfi") attribute.  */
static bool
function_needs_kcfi_preamble (tree fndecl)
{
  /* Only instrument if KCFI is globally enabled.  */
  if (!(flag_sanitize & SANITIZE_KCFI))
    return false;

  struct cgraph_node *node = cgraph_node::get (fndecl);

  /* Ignore cold partition functions: not reached via indirect call.  */
  if (node && node->split_part)
    return false;

  /* Ignore cold partition sections: cold partitions are never indirect call
     targets.  Only skip preambles for cold partitions (has_bb_partition = true)
     not for entire cold-attributed functions (has_bb_partition = false).  */
  if (in_cold_section_p && crtl && crtl->has_bb_partition)
    return false;

  /* Check if function is truly address-taken using cgraph node analysis.  */
  bool addr_taken = (node && node->address_taken);

  /* Only instrument functions that can be targets of indirect calls:
     - Public functions (can be called externally)
     - External declarations (from other modules)
     - Functions with true address-taken status from cgraph analysis.  */
  return TREE_PUBLIC (fndecl) || DECL_EXTERNAL (fndecl) || addr_taken;
}

/* Function attribute to store KCFI type ID.  */
static tree kcfi_type_id_attr = NULL_TREE;

/* Set KCFI type ID for a function declaration during IPA phase.
   Fatal error if type ID is already set.  */
static void
set_function_kcfi_type_id (tree fndecl)
{
  if (!kcfi_type_id_attr)
    kcfi_type_id_attr = get_identifier ("kcfi_type_id");

  /* Fatal error if type ID already set - nothing should set it twice.  */
  if (lookup_attribute_by_prefix ("kcfi_type_id",
				  DECL_ATTRIBUTES (fndecl)))
    internal_error ("KCFI type ID already set for function %qD", fndecl);

  /* Compute type ID using FUNCTION_TYPE to preserve typedef information.  */
  uint32_t type_id = compute_kcfi_type_id (TREE_TYPE (fndecl), fndecl);

  tree type_id_tree = build_int_cst (unsigned_type_node, type_id);
  tree attr_value = build_tree_list (NULL_TREE, type_id_tree);
  tree attr = build_tree_list (kcfi_type_id_attr, attr_value);

  DECL_ATTRIBUTES (fndecl) = chainon (DECL_ATTRIBUTES (fndecl), attr);
}

/* Get KCFI type ID for a function declaration during assembly output phase.
   Fatal error if type ID was not previously set during IPA phase.  */
static uint32_t
get_function_kcfi_type_id (tree fndecl)
{
  if (!kcfi_type_id_attr)
    kcfi_type_id_attr = get_identifier ("kcfi_type_id");

  tree attr = lookup_attribute_by_prefix ("kcfi_type_id",
					   DECL_ATTRIBUTES (fndecl));
  if (attr && TREE_VALUE (attr) && TREE_VALUE (TREE_VALUE (attr)))
    {
      tree value = TREE_VALUE (TREE_VALUE (attr));
      if (TREE_CODE (value) == INTEGER_CST)
	return (uint32_t) TREE_INT_CST_LOW (value);
    }

  internal_error ("KCFI type ID not found for function %qD - "
		  "should have been set during GIMPLE phase", fndecl);
}

/* Prepare the global KCFI alignment NOPs calculation.
   Called once during IPA pass to set global variable.  */
static void
kcfi_prepare_alignment_nops (void)
{
  /* Only use global patchable-function-entry flag, not function attributes.
     KCFI callsites cannot know about function-specific attributes.  */
  if (flag_patchable_function_entry)
    {
      HOST_WIDE_INT total_nops, prefix_nops = 0;
      parse_and_check_patch_area (flag_patchable_function_entry, false,
				  &total_nops, &prefix_nops);
      /* Store value for callsite offset calculation */
      kcfi_patchable_entry_prefix_nops = prefix_nops;
    }

  /* Calculate architecture-specific alignment NOPs.
     KCFI preamble layout:
     __cfi_func: [alignment_nops][typeid][prefix_nops] func: [entry_nops]

     The alignment NOPs ensure __cfi_func stays at proper function alignment
     when prefix NOPs are added.  */
  HOST_WIDE_INT arch_alignment = 0;

  /* Calculate alignment NOPs based on function alignment setting.
     Use explicit -falign-functions if set, otherwise default to 4 bytes. */
  int alignment_bytes = 4;
  if (align_functions.levels[0].log > 0)
    {
      /* Use explicit -falign-functions setting */
      alignment_bytes = align_functions.levels[0].get_value();
    }

  /* Get typeid instruction size from target hook, default to 4 bytes */
  int typeid_size = targetm.kcfi.emit_type_id
                    ? targetm.kcfi.emit_type_id (NULL, 0) : 4;

  /* Calculate alignment NOPs needed */
  arch_alignment = (alignment_bytes - ((kcfi_patchable_entry_prefix_nops + typeid_size) % alignment_bytes)) % alignment_bytes;

  /* Use the calculated alignment NOPs */
  kcfi_patchable_entry_arch_alignment_nops = arch_alignment;
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

  /* Everything else must be indirect calls needing KCFI.  */
  return true;
}

/* Extract KCFI type ID from indirect call GIMPLE statement.
   Returns RTX constant with type ID, or NULL_RTX if no KCFI needed.  */
rtx
kcfi_get_call_type_id (void)
{
  if (!sanitize_flags_p (SANITIZE_KCFI) || !currently_expanding_gimple_stmt)
    return NULL_RTX;

  if (!is_gimple_call (currently_expanding_gimple_stmt))
    return NULL_RTX;

  gcall *call_stmt = as_a <gcall *> (currently_expanding_gimple_stmt);

  /* Only indirect calls need KCFI instrumentation.  */
  if (gimple_call_fndecl (call_stmt))
    return NULL_RTX;

  tree fn_type = gimple_call_fntype (call_stmt);
  if (!fn_type)
    return NULL_RTX;

  tree attr = lookup_attribute ("kcfi_type_id", TYPE_ATTRIBUTES (fn_type));
  if (!attr || !TREE_VALUE (attr))
    return NULL_RTX;

  if (gimple_call_inlined_from_kcfi_nosantize_p (call_stmt))
    return NULL_RTX;

  uint32_t kcfi_type_id = (uint32_t) tree_to_uhwi (TREE_VALUE (attr));
  return GEN_INT (kcfi_type_id);
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

void
kcfi_emit_preamble (FILE *file, tree decl, const char *actual_fname)
{
  /* Check if KCFI is enabled and function needs preamble.  */
  if (!function_needs_kcfi_preamble (decl))
    return;

  /* Use actual function name if provided, otherwise fall back to DECL_ASSEMBLER_NAME.  */
  const char *fname = actual_fname ? actual_fname
				   : IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (decl));

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

  /* Emit architecture-specific prefix NOPs.  */
  for (int i = 0; i < kcfi_patchable_entry_arch_alignment_nops; i++)
    {
      fprintf (file, "\tnop\n");
    }

  /* Emit type ID bytes.  */
  if (targetm.kcfi.emit_type_id)
    targetm.kcfi.emit_type_id (file, type_id);
  else
    fprintf (file, "\t.word\t0x%08x\n", type_id);

  /* Mark end of __cfi_ symbol and emit size directive.  */
  char cfi_end_label[256];
  snprintf (cfi_end_label, sizeof(cfi_end_label), ".Lcfi_func_end_%s", fname);
  ASM_OUTPUT_LABEL (file, cfi_end_label);

  ASM_OUTPUT_MEASURED_SIZE (file, cfi_symbol_name);
}

/* KCFI GIMPLE pass implementation.  */

static bool
gate_kcfi (void)
{
  /* Always process functions when KCFI is globally enabled to set type IDs.
     Individual function processing (call instrumentation) will check no_sanitize("kcfi").  */
  return sanitize_flags_p (SANITIZE_KCFI);
}

/* Create a KCFI wrapper function type that embeds the type ID.  */
static tree
create_kcfi_wrapper_type (tree original_fn_type, uint32_t type_id)
{
  /* Create a unique type name incorporating the type ID.  */
  char wrapper_name[32];
  snprintf (wrapper_name, sizeof (wrapper_name), "__kcfi_wrapper_%x", type_id);

  /* Build a new function type that's structurally identical but nominally different.  */
  tree wrapper_type = build_function_type (TREE_TYPE (original_fn_type),
					   TYPE_ARG_TYPES (original_fn_type));

  /* Set the type name to make it distinct.  */
  TYPE_NAME (wrapper_type) = get_identifier (wrapper_name);

  /* Attach kcfi_type_id attribute to the original function type for cfgexpand.cc */
  tree attr_name = get_identifier ("kcfi_type_id");
  tree attr_value = build_int_cst (unsigned_type_node, type_id);
  tree attr = build_tree_list (attr_name, attr_value);
  TYPE_ATTRIBUTES (original_fn_type) = chainon (TYPE_ATTRIBUTES (original_fn_type), attr);

  return wrapper_type;
}

/* Wrap indirect calls with KCFI type for anti-merging.  */
static unsigned int
kcfi_instrument (void)
{
  /* Process current function for call instrumentation only.
     Type ID setting is handled by the separate IPA pass.  */

  basic_block bb;

  FOR_EACH_BB_FN (bb, cfun)
    {
      gimple_stmt_iterator gsi;
      for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
	{
	  gimple *stmt = gsi_stmt (gsi);

	  if (!is_gimple_call (stmt))
	    continue;

	  gcall *call_stmt = as_a <gcall *> (stmt);

	  // Skip internal calls - we only instrument indirect calls
	  if (gimple_call_internal_p (call_stmt))
	    continue;

	  tree fndecl = gimple_call_fndecl (call_stmt);

	  // Only process indirect calls (no fndecl)
	  if (fndecl)
	    continue;

	  tree fn = gimple_call_fn (call_stmt);
	  if (!is_kcfi_indirect_call (fn))
	    continue;

	  // Get the function type to compute KCFI type ID
	  tree fn_type = gimple_call_fntype (call_stmt);
	  gcc_assert (fn_type);
	  if (TREE_CODE (fn_type) != FUNCTION_TYPE)
	    continue;

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

/* IPA pass for KCFI type ID setting - runs once per compilation unit.  */

namespace {

const pass_data pass_data_ipa_kcfi =
{
  SIMPLE_IPA_PASS, /* type */
  "ipa_kcfi", /* name */
  OPTGROUP_NONE, /* optinfo_flags */
  TV_IPA_OPT, /* tv_id */
  0, /* properties_required */
  0, /* properties_provided */
  0, /* properties_destroyed */
  0, /* todo_flags_start */
  0, /* todo_flags_finish */
};

/* Set KCFI type IDs for all functions in the compilation unit.  */
static unsigned int
ipa_kcfi_execute (void)
{
  struct cgraph_node *node;

  /* Prepare global KCFI alignment NOPs calculation once for all functions.  */
  kcfi_prepare_alignment_nops ();

  /* Process all functions - both local and external.
     This preserves typedef information using DECL_ARGUMENTS.  */
  FOR_EACH_FUNCTION (node)
    {
      tree fndecl = node->decl;

      /* Skip all non-NORMAL builtins (MD, FRONTEND) entirely.
         For NORMAL builtins, skip those that lack an implicit
	 implementation (closest way to distinguishing DEF_LIB_BUILTIN
	 from others). E.g. we need to have typeids for memset().  */
      if (fndecl_built_in_p (fndecl))
        {
          if (DECL_BUILT_IN_CLASS (fndecl) != BUILT_IN_NORMAL)
            continue;
          if (!builtin_decl_implicit_p (DECL_FUNCTION_CODE (fndecl)))
            continue;
        }

      set_function_kcfi_type_id (fndecl);
    }

  return 0;
}

class pass_ipa_kcfi : public simple_ipa_opt_pass
{
public:
  pass_ipa_kcfi (gcc::context *ctxt)
    : simple_ipa_opt_pass (pass_data_ipa_kcfi, ctxt)
  {}

  /* opt_pass methods: */
  bool gate (function *) final override
  {
    return sanitize_flags_p (SANITIZE_KCFI);
  }

  unsigned int execute (function *) final override
  {
    return ipa_kcfi_execute ();
  }

}; // class pass_ipa_kcfi

} // anon namespace

simple_ipa_opt_pass *
make_pass_ipa_kcfi (gcc::context *ctxt)
{
  return new pass_ipa_kcfi (ctxt);
}
