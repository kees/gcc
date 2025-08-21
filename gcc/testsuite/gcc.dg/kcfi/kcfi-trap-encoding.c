/* Test AArch64 and ARM32 KCFI trap encoding in BRK/UDF instructions.  */
/* { dg-do compile { target { aarch64*-*-* || arm32 } } } */
/* { dg-options "-fsanitize=kcfi" } */
/* { dg-options "-fsanitize=kcfi -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

void target_function(int x, char y) {
}

int main() {
    void (*func_ptr)(int, char) = target_function;

    /* This should generate trap with immediate encoding.  */
    func_ptr(42, 'a');

    return 0;
}

/* Should have KCFI preamble.  */
/* { dg-final { scan-assembler "__cfi_target_function:" } } */

/* AArch64 specific: Should have BRK instruction with proper ESR encoding
   ESR format: 0x8000 | ((type_reg & 31) << 5) | (addr_reg & 31)

   Test the ESR encoding by checking for the expected value.
   Since we know this test uses x2, we expect ESR = 0x8000 | (17<<5) | 2 = 33314

   A truly dynamic test would need to extract the register from blr and compute
   the corresponding ESR, but DejaGnu's regex limitations make this complex.
   This test validates the specific case and documents the encoding.
   */
/* { dg-final { scan-assembler "blr\\s+x2" { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler "brk\\s+#33314" { target aarch64*-*-* } } } */

/* Should have KCFI check with type comparison.  */
/* { dg-final { scan-assembler {ldur\t*w16, \[x[0-9]+, #-4\]} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler {cmp\t*w16, w17} { target aarch64*-*-* } } } */

/* ARM32 specific: Should have UDF instruction with proper encoding
   UDF format: 0x8000 | ((type_reg & 31) << 5) | (addr_reg & 31)

   Since ARM32 spills and restores r0/r1 before the trap, the type_reg
   field uses 0x1F (31) to indicate "register was spilled" rather than
   pointing to a live register. The addr_reg field contains the actual
   target register number.

   For this test case using r3, we expect:
   UDF = 0x8000 | (31 << 5) | 3 = 0x8000 | 0x3E0 | 3 = 33763
   */
/* { dg-final { scan-assembler "blx\\s+r3" { target arm32 } } } */
/* { dg-final { scan-assembler "udf\\s+#33763" { target arm32 } } } */

/* Should have register spilling and restoration around type check.  */
/* { dg-final { scan-assembler {push\t*\{r0, r1\}} { target arm32 } } } */
/* { dg-final { scan-assembler {pop\t*\{r0, r1\}} { target arm32 } } } */
/* { dg-final { scan-assembler {ldr\t*r0, \[r[0-9]+, #-4\]} { target arm32 } } } */
/* { dg-final { scan-assembler {cmp\t*r0, r1} { target arm32 } } } */
