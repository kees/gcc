/* Test KCFI with retpoline thunk-extern flag forces r11 usage.  */
/* { dg-do compile { target x86_64-*-* } } */
/* { dg-options "-fsanitize=kcfi -mindirect-branch=thunk-extern -O2" } */
/* { dg-options "-fsanitize=kcfi -mindirect-branch=thunk-extern -O2 -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

extern int external_target(void);

/* Test regular call (not tail call) */
__attribute__((noinline))
int call_test(int (*func_ptr)(void)) {
    /* This indirect call should use r11 when both KCFI and
       -mindirect-branch=thunk-extern are enabled.  */
    int result = func_ptr();  /* Function parameter prevents direct optimization.  */
    return result + 1;  /* Prevent tail call optimization.  */
}

/* Reference external_target to generate the required symbol.  */
int (*external_func_ptr)(void) = external_target;

/* Test function for sibcalls (tail calls) */
__attribute__((noinline))
void sibcall_test(int (**func_ptr)(void)) {
    /* This sibcall should use r11 when both KCFI and
       -mindirect-branch=thunk-extern are enabled.  */
    (*func_ptr)();  /* Tail call - should be optimized to sibcall.  */
}

/* Should have weak symbol for external function.  */
/* { dg-final { scan-assembler "__kcfi_typeid_external_target" } } */

/* When both KCFI and -mindirect-branch=thunk-extern are enabled,
   indirect calls should always use r11 register and convert to extern thunks.  */
/* { dg-final { scan-assembler-times {call\s+__x86_indirect_thunk_r11} 1 } } */

/* Sibcalls should also use r11 register and convert to extern thunks.  */
/* { dg-final { scan-assembler-times {jmp\s+__x86_indirect_thunk_r11} 1 } } */

/* Should have exactly 2 KCFI traps (one per function) */
/* { dg-final { scan-assembler-times {ud2} 2 } } */

/* Should NOT use other registers for indirect calls.  */
/* { dg-final { scan-assembler-not {call\s+\*%rax} } } */
/* { dg-final { scan-assembler-not {call\s+\*%rcx} } } */
/* { dg-final { scan-assembler-not {call\s+\*%rdx} } } */
/* { dg-final { scan-assembler-not {call\s+\*%rdi} } } */

/* Should NOT use other registers for sibcalls.  */
/* { dg-final { scan-assembler-not {jmp\s+\*%rax} } } */
/* { dg-final { scan-assembler-not {jmp\s+\*%rcx} } } */
/* { dg-final { scan-assembler-not {jmp\s+\*%rdx} } } */
/* { dg-final { scan-assembler-not {jmp\s+\*%rdi} } } */
