/* Test AArch64 KCFI ESR encoding in BRK instructions */
/* { dg-do compile { target aarch64*-*-* } } */
/* { dg-options "-fsanitize=kcfi" } */

void target_function(int x, char y) {
    /* Different signature to get different type ID */
}

int main() {
    void (*func_ptr)(int, char) = target_function;

    /* This should generate BRK with ESR encoding */
    func_ptr(42, 'a');

    return 0;
}

/* Should have KCFI preamble */
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

/* Should have KCFI check with type comparison */
/* { dg-final { scan-assembler {ldur\t*w16, \[x[0-9]+, #-4\]} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler {cmp\t*w16, w17} { target aarch64*-*-* } } } */
