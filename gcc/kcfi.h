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

#ifndef GCC_KCFI_H
#define GCC_KCFI_H

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "rtl.h"

/* Get KCFI type ID for a function declaration.  */
extern uint32_t get_function_kcfi_type_id (tree fndecl);

/* KCFI target hooks for architecture-specific functionality.  */

struct kcfi_target_hooks {
  /* Apply architecture-specific masking to type ID.  */
  uint32_t (*mask_type_id) (uint32_t type_id);

  /* Generate bundled KCFI checked call (atomic check + call to prevent optimizer separation) */
  rtx (*gen_kcfi_checked_call) (rtx call_insn, rtx target_reg, uint32_t expected_type, HOST_WIDE_INT prefix_nops);

  /* Add architecture-specific register clobbers for KCFI calls.  */
  void (*add_kcfi_clobbers) (rtx_insn *call_insn);

  /* Calculate architecture-specific prefix NOPs count (optional, returns prefix_nops unchanged if NULL) */
  int (*calculate_prefix_nops) (HOST_WIDE_INT prefix_nops);

  /* Emit architecture-specific type ID instruction (required for common preamble helper) */
  void (*emit_type_id_instruction) (FILE *file, uint32_t type_id);
};

/* Global KCFI target hooks.  */
extern struct kcfi_target_hooks kcfi_target;

/* Common helper for RTL patterns to emit .kcfi_traps section entry.
   Call AFTER emitting trap label and instruction with the RTX label operand.  */
extern void kcfi_emit_trap_with_section (FILE *file, rtx trap_label_rtx);

/* RTL note management for KCFI.  */

/* Add KCFI type ID note to call instruction.  */
extern void add_kcfi_type_note (rtx_insn *call_insn, uint32_t type_id);

/* Emit KCFI type ID symbol for address-taken functions.  */
extern void emit_kcfi_typeid_symbol (FILE *asm_file, tree decl, const char *name);

/* KCFI preamble emission coordination.  */

/* Mark that KCFI preamble has been emitted to prevent duplication.  */
extern void mark_kcfi_preamble_emitted (void);

/* Central manager for all KCFI preamble generation decisions.  */
extern void kcfi_emit_preamble_if_needed (FILE *file, tree decl,
					  bool is_patchable_context,
					  HOST_WIDE_INT prefix_nops,
					  const char *actual_fname);

/* Pass creation functions.  */
class gimple_opt_pass;
class rtl_opt_pass;
namespace gcc { class context; }

extern gimple_opt_pass *make_pass_kcfi (gcc::context *ctxt);
extern gimple_opt_pass *make_pass_kcfi_O0 (gcc::context *ctxt);
extern rtl_opt_pass *make_pass_kcfi_final_instrumentation (gcc::context *ctxt);

#endif /* GCC_KCFI_H */
