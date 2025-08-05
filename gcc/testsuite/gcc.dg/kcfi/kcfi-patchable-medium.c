/* Test KCFI with medium patchable function entries */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=8,4" } */

void test_function(void) {
    /* 4 prefix NOPs, 4 entry NOPs - should affect KCFI offset */
}

int main() {
    void (*func_ptr)(void) = test_function;
    func_ptr(); /* Call site should use -(4+4) = -8 offset */
    return 0;
}

/* Should have KCFI preamble */
/* { dg-final { scan-assembler "__cfi_test_function:" } } */

/* Should have patchable function entry section */
/* { dg-final { scan-assembler "__patchable_function_entries" } } */

/* x86_64: Should have exactly 4 prefix NOPs between .LPFE and .type */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*\.type} { target x86_64-*-* } } } */

/* x86_64: Should have exactly 4 entry NOPs between .cfi_startproc and pushq */
/* { dg-final { scan-assembler {\.cfi_startproc\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*pushq} { target x86_64-*-* } } } */

/* x86_64: KCFI should have exactly 7 NOPs between __cfi_ and movl (offsetToAlignment(4+5,16)=7) */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*movl} { target x86_64-*-* } } } */

/* x86_64: Validate KCFI type ID is present */
/* { dg-final { scan-assembler {movl\t\$0x[0-9a-f]+, %eax} { target x86_64-*-* } } } */

/* x86_64: Call site should use -8 offset (kernel-style encoding) */
/* { dg-final { scan-assembler {movl\t\$-?[0-9]+, %r10d\n\taddl\t-8\(%r[a-z0-9]+\), %r10d} { target x86_64-*-* } } } */

/* AArch64: Call site should use -8 offset */
/* { dg-final { scan-assembler {ldur\tw16, \[x[0-9]+, #-8\]} { target aarch64*-*-* } } } */
