/* Test KCFI without patchable function entries - standard case */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi" } */

void test_function(void) {
    /* Standard KCFI without patchable entries */
}

int main() {
    test_function();
    return 0;
}

/* Should have KCFI preamble */
/* { dg-final { scan-assembler "__cfi_test_function:" } } */

/* Should NOT have patchable function entry section */
/* { dg-final { scan-assembler-not "__patchable_function_entries" } } */
