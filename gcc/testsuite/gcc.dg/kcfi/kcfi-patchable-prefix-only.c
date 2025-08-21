/* Test KCFI with patchable function entries - prefix NOPs only.  */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=3,3" } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=3,3 -falign-functions=16" { target x86_64-*-* } } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=3,3 -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

void test_function(void) {
}

int main() {
    test_function();
    return 0;
}

/* Should have KCFI preamble.  */
/* { dg-final { scan-assembler "__cfi_test_function:" } } */

/* x86_64: All 3 NOPs are prefix NOPs - should have exactly 3 prefix NOPs.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*\.type\t*test_function} { target x86_64-*-* } } } */

/* x86_64: No entry NOPs - function should start immediately with prologue. */
/* { dg-final { scan-assembler {test_function:\n\.LFB[0-9]+:\n\t*\.cfi_startproc\n\t*pushq\t*%rbp} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-not {\t*\.weak\t*__kcfi_typeid_test_function\n} { target x86_64-*-* } } } */

/* x86_64: should have exactly 8 alignment NOPs.  */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*movl} { target x86_64-*-* } } } */

/* AArch64: All 3 NOPs are prefix NOPs - should have exactly 3 prefix NOPs.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*\.type\t*test_function} { target aarch64*-*-* } } } */

/* AArch64: No entry NOPs - function should start immediately with prologue.  */
/* { dg-final { scan-assembler {test_function:\n\.LFB[0-9]+:\n\t*\.cfi_startproc\n\t*nop\n\t*ret} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler-not {\t*\.weak\t*__kcfi_typeid_test_function\n} { target aarch64*-*-* } } } */

/* AArch64: KCFI type ID should have 1 alignment NOP then word.  */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*nop\n\t*\.word\t0x[0-9a-f]+} { target aarch64*-*-* } } } */

/* ARM 32-bit: All 3 NOPs are prefix NOPs - should have exactly 3 prefix NOPs.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop} { target arm32 } } } */

/* ARM 32-bit: No entry NOPs - function should start immediately with
   prologue.  */
/* { dg-final { scan-assembler {test_function:} { target arm32 } } } */
/* { dg-final { scan-assembler-not {\t*\.weak\t*__kcfi_typeid_test_function\n} { target arm32 } } } */

/* ARM 32-bit: KCFI type ID should have 1 alignment NOP then word.  */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*nop\n\t*\.word\t0x[0-9a-f]+} { target arm32 } } } */

/* RISC-V: All 3 NOPs are prefix NOPs - should have exactly 3 prefix NOPs.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*\.type\t*test_function} { target riscv*-*-* } } } */

/* RISC-V: No entry NOPs - function should start immediately with
   .cfi_startproc.  */
/* { dg-final { scan-assembler {test_function:\n\.LFB[0-9]+:\n\t*\.cfi_startproc} { target riscv*-*-* } } } */
/* { dg-final { scan-assembler-not {\t*\.weak\t*__kcfi_typeid_test_function\n} { target riscv*-*-* } } } */

/* RISC-V: KCFI type ID should have 1 alignment NOP then word.  */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*nop\n\t*\.word\t0x[0-9a-f]+} { target riscv*-*-* } } } */

/* Should have patchable function entry section.  */
/* { dg-final { scan-assembler "__patchable_function_entries" } } */
