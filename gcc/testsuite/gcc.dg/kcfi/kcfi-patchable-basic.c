/* Test KCFI with patchable function entries - basic case */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=5,2" } */

void test_function(int x) {
    /* Function should get both KCFI preamble and patchable entries */
}

int main() {
    test_function(42);
    return 0;
}

/* Should have KCFI preamble */
/* { dg-final { scan-assembler "__cfi_test_function:" } } */

/* Should have patchable function entry section */
/* { dg-final { scan-assembler "__patchable_function_entries" } } */

/* x86_64: Should have exactly 2 prefix NOPs between .LPFE and .type */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*\.type} { target x86_64-*-* } } } */

/* x86_64: Should have exactly 3 entry NOPs between .cfi_startproc and pushq */
/* { dg-final { scan-assembler {\.cfi_startproc\n\t*nop\n\t*nop\n\t*nop\n\t*pushq} { target x86_64-*-* } } } */

/* x86_64: KCFI should have exactly 9 NOPs between __cfi_ and movl */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*movl} { target x86_64-*-* } } } */

/* x86_64: Validate KCFI type ID is present */
/* { dg-final { scan-assembler {movl\t\$0x[0-9a-f]+, %eax} { target x86_64-*-* } } } */

/* AArch64: Should have exactly 2 prefix NOPs between .LPFE and .type */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*\.type} { target aarch64*-*-* } } } */

/* AArch64: Should have exactly 3 entry NOPs between .cfi_startproc and sub sp */
/* { dg-final { scan-assembler {\.cfi_startproc\n\t*nop\n\t*nop\n\t*nop\n\t*sub\t*sp} { target aarch64*-*-* } } } */

/* AArch64: KCFI should have only .word immediate (no NOPs) */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t\.word 0x[0-9a-f]+} { target aarch64*-*-* } } } */

/* AArch64: Validate clean KCFI boundary - .word then immediate end/size */
/* { dg-final { scan-assembler {\.word 0x[0-9a-f]+\n\.Lcfi_func_end_test_function:\n\t\.size\t__cfi_test_function, \.-__cfi_test_function} { target aarch64*-*-* } } } */
