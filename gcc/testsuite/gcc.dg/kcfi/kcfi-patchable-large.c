/* Test KCFI with large patchable function entries.  */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=11,11" } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=11,11 -falign-functions=16" { target x86_64-*-* } } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=11,11 -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

void test_function(void) {
}

int main() {
    void (*func_ptr)(void) = test_function;
    func_ptr();
    return 0;
}

/* Should have KCFI preamble.  */
/* { dg-final { scan-assembler "__cfi_test_function:" } } */

/* Should have patchable function entry section.  */
/* { dg-final { scan-assembler "__patchable_function_entries" } } */

/* x86_64: Should have exactly 11 alignment NOPs between .LPFE and .type.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*\.type} { target x86_64-*-* } } } */

/* x86_64: Should have 0 entry NOPs - function starts immediately with
   pushq.  */
/* { dg-final { scan-assembler {test_function:\n\.LFB[0-9]+:\n\t*\.cfi_startproc\n\t*pushq\t*%rbp} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-not {\t*\.weak\t*__kcfi_typeid_test_function\n} { target x86_64-*-* } } } */

/* x86_64: KCFI should have 0 entry NOPs - goes directly to typeid movl.  */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*movl\t\$0x[0-9a-f]+, %eax} { target x86_64-*-* } } } */

/* x86_64: Call site should use -15 offset.  */
/* { dg-final { scan-assembler {movl\t\$-?[0-9]+, %r10d\n\taddl\t-15\(%r[a-z0-9]+\), %r10d} { target x86_64-*-* } } } */

/* AArch64: Should have exactly 11 prefix NOPs between .LPFE and .type.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*\.type} { target aarch64*-*-* } } } */

/* ARM 32-bit: Should have exactly 11 prefix NOPs between .LPFE and .type.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop} { target arm32 } } } */

/* AArch64: Call site should use -15 offset.  */
/* { dg-final { scan-assembler {ldur\tw16, \[x[0-9]+, #-15\]} { target aarch64*-*-* } } } */

/* ARM 32-bit: Call site should use -15 offset.  */
/* { dg-final { scan-assembler {ldr\tr0, \[r[0-9]+, #-15\]} { target arm32 } } } */

/* RISC-V: Should have 11 prefix NOPs between .LPFE and .type.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*\.type} { target riscv*-*-* } } } */

/* RISC-V: Call site should use -15 offset (same as x86/AArch64).  */
/* { dg-final { scan-assembler {lw\tt1, -15\(} { target riscv*-*-* } } } */
