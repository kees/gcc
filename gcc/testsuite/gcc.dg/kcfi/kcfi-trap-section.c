/* Test KCFI trap section generation */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi" } */

void target_function(void) {}

int main() {
    void (*func_ptr)(void) = target_function;

    /* Multiple indirect calls to generate trap entries */
    func_ptr();
    func_ptr();

    return 0;
}

/* Should have KCFI preamble */
/* { dg-final { scan-assembler "__cfi_target_function:" } } */

/* Should have exactly 2 trap labels in code */
/* { dg-final { scan-assembler-times {\.L[^:]+:\n\s*ud2} 2 { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-times {\.L[^:]+:\n\s*brk} 2 { target aarch64*-*-* } } } */

/* Should have .kcfi_traps section with exactly 2 entries (x86 only - AArch64 uses brk immediate) */
/* { dg-final { scan-assembler {\.pushsection\t*\.kcfi_traps,"ao",@progbits,\.text} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-times {\.L[^:]+:\n\t*\.long} 2 { target x86_64-*-* } } } */

/* Each section entry must reference its corresponding trap label directly (x86 only) */
/* Entry references trap label directly (no offset needed) */
/* { dg-final { scan-assembler {\.L([^:]+):\n\t*\.long\t*\.L([^\s]+)\s+-\s+\.L\1} { target x86_64-*-* } } } */
/* Entry references trap label directly (no offset needed) */
/* { dg-final { scan-assembler {\.L([^:]+):\n\t*\.long\t*\.L([^\s]+)\s+-\s+\.L\1} { target x86_64-*-* } } } */

/* Section must be properly closed (x86 only) */
/* { dg-final { scan-assembler {\.popsection} { target x86_64-*-* } } } */
