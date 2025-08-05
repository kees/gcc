/* Test KCFI with patchable function entries - prefix NOPs only */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=3,3" } */

void test_function(void) {
    /* All NOPs are prefix NOPs, should integrate with KCFI */
}

int main() {
    test_function();
    return 0;
}

/* Should have KCFI preamble */
/* { dg-final { scan-assembler "__cfi_test_function:" } } */

/* x86_64: All 3 NOPs are prefix NOPs - should have exactly 3 prefix NOPs */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*\.type\t*test_function} { target x86_64-*-* } } } */

/* x86_64: No entry NOPs - function should start immediately with prologue (no __kcfi_typeid for definitions) */
/* { dg-final { scan-assembler {test_function:\n\.LFB[0-9]+:\n\t*\.cfi_startproc\n\t*pushq\t*%rbp} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-not {\t*\.weak\t*__kcfi_typeid_test_function\n} { target x86_64-*-* } } } */

/* x86_64: KCFI padding should have exactly 8 NOPs (offsetToAlignment(3+5,16)=8) */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*movl} { target x86_64-*-* } } } */

/* AArch64: All 3 NOPs are prefix NOPs - should have exactly 3 prefix NOPs */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*\.type\t*test_function} { target aarch64*-*-* } } } */

/* AArch64: No entry NOPs - function should start immediately with prologue (no __kcfi_typeid for definitions) */
/* { dg-final { scan-assembler {test_function:\n\.LFB[0-9]+:\n\t*\.cfi_startproc\n\t*nop\n\t*ret} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler-not {\t*\.weak\t*__kcfi_typeid_test_function\n} { target aarch64*-*-* } } } */

/* AArch64: KCFI type ID should be immediate word (no alignment NOPs needed) */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t\.word 0x[0-9a-f]+} { target aarch64*-*-* } } } */

/* Should have patchable function entry section */
/* { dg-final { scan-assembler "__patchable_function_entries" } } */
