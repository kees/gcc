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

/* Common helper for RTL patterns to emit .kcfi_traps section entry.
   Call after emitting trap label and instruction with the trap symbol
   reference.  */
extern void kcfi_emit_traps_section (FILE *file, rtx trap_label_sym);

/* Extract KCFI type ID from current GIMPLE statement.  */
extern rtx kcfi_get_call_type_id (void);

/* Emit KCFI type ID symbol for address-taken functions.  */
extern void emit_kcfi_typeid_symbol (FILE *asm_file, tree decl,
				     const char *name);

/* Emit KCFI preamble.  */
extern void kcfi_emit_preamble (FILE *file, tree decl,
				const char *actual_fname);

/* For calculating callsite offset.  */
extern HOST_WIDE_INT kcfi_patchable_entry_prefix_nops;

#endif /* GCC_KCFI_H */
