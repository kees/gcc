/* Test for exact expected KCFI instrumentation instruction sequence */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi" } */

void untaken_function(int x) {
    /* Target function should get preamble */
}

void target_function(int x) {
    /* Target function should get preamble */
}

int main() {
    void (*func_ptr)(int) = target_function;

    /* This indirect call should get KCFI check */
    func_ptr(42);

    untaken_function(15);

    return 0;
}

/* Should have KCFI preamble for target */
/* { dg-final { scan-assembler "__cfi_target_function:" } } */

/* Should not have KCFI preamble for local non-address-take function */
/* { dg-final { scan-assembler-not "__cfi_untaken-function:" } } */

/* x86_64: Complete KCFI check sequence (kernel-style encoding) - updated for unified trap emission */
/* Note: Single regex to enforce exact sequence - based on actual hexdump analysis */
/* { dg-final { scan-assembler {movl\s+\$[0-9]+,\s+%r10d\s+addl\s+-4\(%rax\),\s+%r10d\s+je\s+\.L[0-9]+\s*\.L([^:]+):\s+ud2\s+\.pushsection\s+\.kcfi_traps[^\n]*\s*\.L([^:]+):\s+\.long\s+\.L\1\s+-\s+\.L\2\s+\.popsection\s*\.L[0-9]+:\s+call\s+\*%rax} { target x86_64-*-* } } } */

/* AArch64: Complete KCFI check sequence */
/* Load type ID, load expected type, compare, conditional branch, trap, branch target, call */
/* Note: Single regex to enforce exact sequence - based on actual hexdump analysis */
/* { dg-final { scan-assembler {ldur\tw16, \[x[0-9]+, #-4\]\n\tmov\tw17, #[0-9]+\n\tmovk\tw17, #[0-9]+, lsl #16\n\tcmp\tw16, w17\n\tb\.eq\t\.L[0-9]+\n\t\.L([^:]+):\n\tbrk\t#[0-9]+\n\t\.L[0-9]+:\n\t+blr\tx[0-9]+} { target aarch64*-*-* } } } */
