/* Test KCFI with patchable function entries - entry NOPs only */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=4,0" } */

void test_function(void) {
    /* All NOPs are entry NOPs, no prefix NOPs for KCFI */
}

int main() {
    test_function();
    return 0;
}

/* Should NOT have KCFI preamble (no prefix NOPs) */
/* { dg-final { scan-assembler-not "__cfi_test_function:" } } */

/* x86_64: All 4 NOPs are entry NOPs - should have exactly 4 entry NOPs */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*pushq} { target x86_64-*-* } } } */

/* x86_64: No prefix NOPs - function type should come immediately before function */
/* { dg-final { scan-assembler {\.type\t*test_function, @function\n*test_function:} { target x86_64-*-* } } } */

/* AArch64: All 4 NOPs are entry NOPs - should have exactly 4 entry NOPs */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*stp} { target aarch64*-*-* } } } */

/* AArch64: No prefix NOPs - function type should come immediately before function */
/* { dg-final { scan-assembler {\.type\t*test_function, %function\n*test_function:} { target aarch64*-*-* } } } */

/* Should have patchable function entry section */
/* { dg-final { scan-assembler "__patchable_function_entries" } } */
