/* Test KCFI call-site offset validation across architectures */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi" } */

void target_func_a(void) { }
void target_func_b(int x) { }
void target_func_c(int x, int y) { }

int main() {
    void (*ptr_a)(void) = target_func_a;
    void (*ptr_b)(int) = target_func_b;
    void (*ptr_c)(int, int) = target_func_c;

    /* Multiple indirect calls - each should use -4 offset in standard case */
    ptr_a();
    ptr_b(1);
    ptr_c(1, 2);

    return 0;
}

/* Should have KCFI preambles for all functions */
/* { dg-final { scan-assembler "__cfi_target_func_a:" } } */
/* { dg-final { scan-assembler "__cfi_target_func_b:" } } */
/* { dg-final { scan-assembler "__cfi_target_func_c:" } } */

/* x86_64: All call sites should use -4 offset for KCFI type ID loads (kernel-style encoding) */
/* { dg-final { scan-assembler {movl\t\$-?[0-9]+, %r10d\n\taddl\t-4\(%r[a-z0-9]+\), %r10d} { target x86_64-*-* } } } */

/* AArch64: All call sites should use -4 offset */
/* { dg-final { scan-assembler {ldur\tw16, \[x[0-9]+, #-4\]} { target aarch64*-*-* } } } */

/* Should have trap section with multiple entries */
/* { dg-final { scan-assembler "\\.kcfi_traps" { target x86_64-*-* } } } */
