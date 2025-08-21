/* Test KCFI with patchable function entries - entry NOPs only.  */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=4,0" } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=4,0 -falign-functions=16" { target x86_64-*-* } } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=4,0 -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

void test_function(void) {
}

static void caller(void) {
    /* Make an indirect call to test callsite offset calculation.  */
    void (*func_ptr)(void) = test_function;
    func_ptr();
}

int main() {
    test_function();  /* Direct call.  */
    caller();         /* Indirect call via static function.  */
    return 0;
}

/* x86_64: Should have KCFI preamble with architecture alignment NOPs (11).  */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t+nop\n\t+nop\n\t+nop\n\t+nop\n\t+nop\n\t+nop\n\t+nop\n\t+nop\n\t+nop\n\t+nop\n\t+nop\n\t+movl\t+\$0x[0-9a-f]+, %eax} { target x86_64-*-* } } } */

/* AArch64: Should have KCFI preamble with no alignment NOPs.  */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*\.word\t0x[0-9a-f]+} { target aarch64*-*-* } } } */

/* ARM 32-bit: Should have KCFI preamble with no alignment NOPs.  */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t\.word\t0x[0-9a-f]+} { target arm32 } } } */

/* RISC-V: Should have KCFI preamble with no alignment NOPs.  */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t\.word\t0x[0-9a-f]+} { target riscv*-*-* } } } */

/* x86_64: Indirect call should use original prefix NOPs (0) for offset
   calculation: -4 offset.  */
/* { dg-final { scan-assembler {movl\t\$-?[0-9]+, %r10d\n\taddl\t-4\(%r[a-z0-9]+\), %r10d\n\tje\t(\.Lkcfi_call[0-9]+)\n\.Lkcfi_trap[0-9]+:\n\tud2\n.*\n\1:\n\tcall} { target x86_64-*-* } } } */

/* x86_64: All 4 NOPs are entry NOPs - should have exactly 4 entry NOPs.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*pushq} { target x86_64-*-* } } } */


/* AArch64: All 4 NOPs are entry NOPs - should have exactly 4 entry NOPs.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*stp} { target aarch64*-*-* } } } */

/* AArch64: No alignment NOPs - function type should come immediately before
   function.  */
/* { dg-final { scan-assembler {\.type\t*test_function, %function\n*test_function:} { target aarch64*-*-* } } } */

/* ARM 32-bit: All 4 NOPs are entry NOPs - should have exactly 4 entry NOPs.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*nop} { target arm32 } } } */

/* ARM 32-bit: No alignment NOPs - function type should come immediately
   before function.  */
/* { dg-final { scan-assembler {\.type\t*test_function, %function\n*test_function:} { target arm32 } } } */

/* RISC-V: All 4 NOPs are entry NOPs.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\.LFB} { target riscv*-*-* } } } */

/* RISC-V: No alignment NOPs - function type should come immediately
   before function.  */
/* { dg-final { scan-assembler {\.type\t*test_function, @function\n*test_function:} { target riscv*-*-* } } } */

/* Should have patchable function entry section.  */
/* { dg-final { scan-assembler "__patchable_function_entries" } } */
