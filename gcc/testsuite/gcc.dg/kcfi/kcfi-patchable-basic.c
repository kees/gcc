/* Test KCFI with patchable function entries - basic case.  */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=5,2" } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=5,2 -falign-functions=16" { target x86_64-*-* } } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=5,2 -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

void test_function(int x) {
    /* Function should get both KCFI preamble and patchable entries.  */
}

int main() {
    test_function(42);
    return 0;
}

/* Should have KCFI preamble.  */
/* { dg-final { scan-assembler "__cfi_test_function:" } } */

/* Should have patchable function entry section.  */
/* { dg-final { scan-assembler "__patchable_function_entries" } } */

/* x86_64: Should have exactly 2 prefix NOPs between .LPFE and .type.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*\.type} { target x86_64-*-* } } } */

/* x86_64: Should have exactly 3 entry NOPs between .cfi_startproc and
   pushq.  */
/* { dg-final { scan-assembler {\.cfi_startproc\n\t*nop\n\t*nop\n\t*nop\n\t*pushq} { target x86_64-*-* } } } */

/* x86_64: KCFI should have exactly 9 NOPs between __cfi_ and movl.  */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*movl} { target x86_64-*-* } } } */

/* x86_64: Validate KCFI type ID is present.  */
/* { dg-final { scan-assembler {movl\t\$0x[0-9a-f]+, %eax} { target x86_64-*-* } } } */

/* AArch64: Should have exactly 2 prefix NOPs between .LPFE and .type.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*\.type} { target aarch64*-*-* } } } */

/* AArch64: Should have exactly 3 entry NOPs between .cfi_startproc and
   stack manipulation.  */
/* { dg-final { scan-assembler {\.cfi_startproc\n\t*nop\n\t*nop\n\t*nop\n\t*sub\t*sp} { target aarch64*-*-* } } } */

/* AArch64: KCFI should have alignment NOPs then .word immediate.  */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*nop\n\t*nop\n\t*\.word\t0x[0-9a-f]+} { target aarch64*-*-* } } } */

/* AArch64: Validate clean KCFI boundary - .word then immediate end/size.  */
/* { dg-final { scan-assembler {\.word\t0x[0-9a-f]+\n\.Lcfi_func_end_test_function:\n\t\.size\t__cfi_test_function, \.-__cfi_test_function} { target aarch64*-*-* } } } */

/* ARM 32-bit: Should have exactly 2 prefix NOPs between .LPFE and .syntax.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*\.syntax} { target arm32 } } } */

/* ARM 32-bit: Should have exactly 3 entry NOPs after function label.  */
/* { dg-final { scan-assembler {test_function:\n\t*nop\n\t*nop\n\t*nop} { target arm32 } } } */

/* ARM 32-bit: KCFI should have alignment NOPs then .word immediate.  */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*nop\n\t*nop\n\t*\.word\t0x[0-9a-f]+} { target arm32 } } } */

/* ARM 32-bit: Validate clean KCFI boundary - .word then immediate end/size.  */
/* { dg-final { scan-assembler {\.word\t0x[0-9a-f]+\n\.Lcfi_func_end_test_function:\n\t\.size\t__cfi_test_function, \.-__cfi_test_function} { target arm32 } } } */

/* RISC-V: Should have exactly 2 prefix NOPs between .LPFE and .type.  */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*\.type} { target riscv*-*-* } } } */

/* RISC-V: Should have exactly 3 entry NOPs before .cfi_startproc followed
   by addi sp.  */
/* { dg-final { scan-assembler {nop\n\t*nop\n\t*nop\n\.LFB[0-9]+:\n\t*\.cfi_startproc\n\t*addi\t*sp} { target riscv*-*-* } } } */

/* RISC-V: KCFI should have alignment NOPs then .word immediate.  */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*nop\n\t*nop\n\t*\.word\t0x[0-9a-f]+} { target riscv*-*-* } } } */

/* RISC-V: Validate clean KCFI boundary - .word then immediate end/size.  */
/* { dg-final { scan-assembler {\.word\t0x[0-9a-f]+\n\.Lcfi_func_end_test_function:\n\t\.size\t__cfi_test_function, \.-__cfi_test_function} { target riscv*-*-* } } } */
