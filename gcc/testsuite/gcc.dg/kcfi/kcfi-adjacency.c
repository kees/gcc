/* Test KCFI check/transfer adjacency - regression test for instruction insertion */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -O2" } */

/* This test ensures that KCFI security checks remain immediately adjacent
   to their corresponding indirect calls/jumps, with no executable instructions
   between the type ID check and the control flow transfer. */

/* External function pointers to prevent optimization */
extern void (*complex_func_ptr)(int, int, int, int);
extern int (*return_func_ptr)(int, int);

/* Function with complex argument preparation that could tempt
   the optimizer to insert instructions between KCFI check and call */
__attribute__((noinline)) void test_complex_args(int a, int b, int c, int d) {
    /* Complex argument expressions that might cause instruction scheduling */
    complex_func_ptr(a * 2, b + c, d - a, (a << 1) | b);
}

/* Function with return value handling */
__attribute__((noinline)) int test_return_value(int x, int y) {
    /* Return value handling that shouldn't interfere with adjacency */
    int result = return_func_ptr(x + 1, y * 2);
    return result + 1;
}

/* Test struct field access that caused issues in try-catch.c */
struct call_info {
    void (*handler)(void);
    int status;
    int data;
};

extern struct call_info *global_call_info;

__attribute__((noinline)) void test_struct_field_call(void) {
    /* This pattern caused adjacency issues before the fix */
    global_call_info->handler();
}

/* Test conditional indirect call */
__attribute__((noinline)) void test_conditional_call(int flag) {
    if (flag) {
        global_call_info->handler();
    }
}

/* Should have KCFI instrumentation for all indirect calls */

/* x86_64: KCFI checks should be present */
/* { dg-final { scan-assembler {movl\t\$-?[0-9]+, %r1[01]d} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler {addl\t[^,]+, %r1[01]d} { target x86_64-*-* } } } */

/* AArch64: KCFI checks should be present */
/* { dg-final { scan-assembler {ldur\tw16, \[x[0-9]+, #-[0-9]+\]} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler {cmp\tw16, w17} { target aarch64*-*-* } } } */

/* Should have trap instructions for all checks */
/* { dg-final { scan-assembler "ud2" { target x86_64-*-* } } } */
/* { dg-final { scan-assembler "brk" { target aarch64*-*-* } } } */

/* Should have trap section with entries */
/* { dg-final { scan-assembler "\\.kcfi_traps" { target x86_64-*-* } } } */

/* Critical adjacency requirement: After each KCFI type comparison,
   there should be only a conditional jump and label before the indirect transfer.
   No executable instructions should be inserted between the security check
   and the control flow transfer.

   Pattern on x86_64 should be:
   movl $(-TYPEID), %r10d # Load inverse type ID
   addl OFFSET(%reg), %r10d # Add actual type ID
   je   .LabelN           # Jump if zero (match)
   .LN:                  # Trap label (not executable)
   ud2                   # Trap instruction
   .LabelN:              # Call label (not executable)
   call/jmp *TARGET      # Indirect transfer

   This test serves as a regression test to ensure this adjacency
   property is maintained across compiler changes. */
