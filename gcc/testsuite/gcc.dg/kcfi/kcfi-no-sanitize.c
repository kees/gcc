/* Test KCFI with no_sanitize attribute */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi" } */

void target_function(void) {
    /* This should get KCFI preamble */
}

void caller_with_checks(void) {
    /* This function should generate KCFI checks */
    void (*func_ptr)(void) = target_function;
    func_ptr();
}

__attribute__((no_sanitize("kcfi")))
void caller_no_checks(void) {
    /* This function should NOT generate KCFI checks due to no_sanitize */
    void (*func_ptr)(void) = target_function;
    func_ptr();
}

int main() {
    caller_with_checks();    /* This should generate checks inside */
    caller_no_checks();      /* This should NOT generate checks inside */
    return 0;
}

/* All functions should get preambles regardless of no_sanitize */
/* { dg-final { scan-assembler "__cfi_target_function:" } } */
/* { dg-final { scan-assembler "__cfi_caller_with_checks:" } } */
/* { dg-final { scan-assembler "__cfi_caller_no_checks:" } } */
/* { dg-final { scan-assembler "__cfi_main:" } } */

/* caller_with_checks() should generate KCFI check (1 check) */
/* caller_no_checks() should NOT generate KCFI check due to no_sanitize attribute */
/* Total: exactly 1 KCFI check in the entire program */
/* { dg-final { scan-assembler-times {addl\t-4\(%r[ad]x\), %r1[01]d} 1 { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-times {ldur\tw16, \[x[0-9]+, #-4\]} 1 { target aarch64-*-* } } } */
