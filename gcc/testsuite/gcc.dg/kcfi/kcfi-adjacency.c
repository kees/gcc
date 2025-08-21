/* Test KCFI check/transfer adjacency - regression test for instruction
   insertion.  */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -O2" } */
/* { dg-options "-fsanitize=kcfi -O2 -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

/* This test ensures that KCFI security checks remain immediately adjacent
   to their corresponding indirect calls/jumps, with no executable instructions
   between the type ID check and the control flow transfer. */

/* External function pointers to prevent optimization.  */
extern void (*complex_func_ptr)(int, int, int, int);
extern int (*return_func_ptr)(int, int);

/* Function with complex argument preparation that could tempt
   the optimizer to insert instructions between KCFI check and call.  */
__attribute__((noinline)) void test_complex_args(int a, int b, int c, int d) {
    /* Complex argument expressions that might cause instruction scheduling.  */
    complex_func_ptr(a * 2, b + c, d - a, (a << 1) | b);
}

/* Function with return value handling.  */
__attribute__((noinline)) int test_return_value(int x, int y) {
    /* Return value handling that shouldn't interfere with adjacency.  */
    int result = return_func_ptr(x + 1, y * 2);
    return result + 1;
}

/* Test struct field access that caused issues in try-catch.c.  */
struct call_info {
    void (*handler)(void);
    int status;
    int data;
};

extern struct call_info *global_call_info;

__attribute__((noinline)) void test_struct_field_call(void) {
    /* This pattern caused adjacency issues before the fix.  */
    global_call_info->handler();
}

/* Test conditional indirect call.  */
__attribute__((noinline)) void test_conditional_call(int flag) {
    if (flag) {
        global_call_info->handler();
    }
}

/* Should have KCFI instrumentation for all indirect calls.  */

/* x86_64: Complete KCFI check sequence should be present.  */
/* { dg-final { scan-assembler {movl\t\$-?[0-9]+, %r1[01]d\n\taddl\t[^,]+, %r1[01]d\n\tje\t\.Lkcfi_call[0-9]+\n\.Lkcfi_trap[0-9]+:\n\tud2} { target x86_64-*-* } } } */

/* AArch64: Complete KCFI check sequence should be present.  */
/* { dg-final { scan-assembler {ldur\tw16, \[x[0-9]+, #-[0-9]+\]\n\tmov\tw17, #[0-9]+\n\tmovk\tw17, #[0-9]+, lsl #16\n\tcmp\tw16, w17\n\tb\.eq\t(\.Lkcfi_call[0-9]+)\n\.Lkcfi_trap[0-9]+:\n\tbrk\t#[0-9]+\n\1:\n\tblr\tx[0-9]+} { target aarch64*-*-* } } } */

/* ARM 32-bit: Complete KCFI check sequence should be present with stack
   spilling.  */
/* { dg-final { scan-assembler {push\t\{r0, r1\}\n\tldr\tr0, \[r[0-9]+, #-[0-9]+\]\n\tmovw\tr1, #[0-9]+\n\tmovt\tr1, #[0-9]+\n\tcmp\tr0, r1\n\tpop\t\{r0, r1\}\n\tbeq\t\.Lkcfi_call[0-9]+\n\.Lkcfi_trap[0-9]+:\n\tudf\t#[0-9]+\n\.Lkcfi_call[0-9]+:\n\tblx\tr[0-9]+} { target arm32 } } } */

/* RISC-V: Complete KCFI check sequence should be present.  */
/* { dg-final { scan-assembler {lw\tt1, -4\([a-z0-9]+\)\n\tlui\tt2, [0-9]+\n\taddiw\tt2, t2, -?[0-9]+\n\tbeq\tt1, t2, \.Lkcfi_call[0-9]+\n\.Lkcfi_trap[0-9]+:\n\tebreak} { target riscv*-*-* } } } */

/* Should have trap section with entries.  */
/* { dg-final { scan-assembler {\.kcfi_traps} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler {\.kcfi_traps} { target riscv*-*-* } } } */

/* AArch64 should NOT have trap section (uses brk immediate instead) */
/* { dg-final { scan-assembler-not {\.kcfi_traps} { target aarch64*-*-* } } } */

/* ARM 32-bit should NOT have trap section (uses udf immediate instead) */
/* { dg-final { scan-assembler-not {\.kcfi_traps} { target arm32 } } } */
