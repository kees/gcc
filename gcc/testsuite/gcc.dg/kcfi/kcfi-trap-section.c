/* Test KCFI trap section generation.  */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi" } */
/* { dg-options "-fsanitize=kcfi -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

void target_function(void) {}

int main() {
    void (*func_ptr)(void) = target_function;

    /* Multiple indirect calls to generate multiple trap entries.  */
    func_ptr();
    func_ptr();

    return 0;
}

/* Should have KCFI preamble.  */
/* { dg-final { scan-assembler "__cfi_target_function:" } } */

/* Should have exactly 2 trap labels in code.  */
/* { dg-final { scan-assembler-times {\.L[^:]+:\n\s*ud2} 2 { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-times {\.L[^:]+:\n\s*brk} 2 { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler-times {\.L[^:]+:\n\s*udf} 2 { target arm32 } } } */
/* { dg-final { scan-assembler-times {\.L[^:]+:\n\s*ebreak} 2 { target riscv*-*-* } } } */

/* x86_64: Should have complete .kcfi_traps section sequence with relative
   offset and 2 entries.  */
/* { dg-final { scan-assembler {\.section\t\.kcfi_traps,"ao",@progbits,\.text\n\.Lkcfi_entry([^:]+):\n\t\.long\t\.Lkcfi_trap([^\s\n]+)-\.Lkcfi_entry\1\n\t\.text} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-times {\.section\t\.kcfi_traps,"ao",@progbits,\.text} 2 { target x86_64-*-* } } } */

/* AArch64 should NOT have .kcfi_traps section (uses brk immediate instead) */
/* { dg-final { scan-assembler-not {\.section\t+\.kcfi_traps} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler-not {\.long.*-\.L} { target aarch64*-*-* } } } */

/* ARM 32-bit should NOT have .kcfi_traps section (uses udf immediate instead) */
/* { dg-final { scan-assembler-not {\.section\t+\.kcfi_traps} { target arm32 } } } */
/* { dg-final { scan-assembler-not {\.long.*-\.L} { target arm32 } } } */

/* RISC-V: Should have complete .kcfi_traps section sequence with relative
   offset and 2 entries.  */
/* { dg-final { scan-assembler {\.section\t\.kcfi_traps,"ao",@progbits,\.text\n\.Lkcfi_entry([^:]+):\n\t\.4byte\t\.L([^\s\n]+)-\.Lkcfi_entry\1\n\t\.text} { target riscv*-*-* } } } */
/* { dg-final { scan-assembler-times {\.section\t\.kcfi_traps,"ao",@progbits,\.text} 2 { target riscv*-*-* } } } */
