/* Test KCFI with large patchable function entries */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -fpatchable-function-entry=11,11" } */

void test_function(void) {
    /* 11 prefix NOPs, 0 entry NOPs - maximum prefix case */
}

int main() {
    void (*func_ptr)(void) = test_function;
    func_ptr(); /* Call site should use -(11+4) = -15 offset */
    return 0;
}

/* Should have KCFI preamble */
/* { dg-final { scan-assembler "__cfi_test_function:" } } */

/* Should have patchable function entry section */
/* { dg-final { scan-assembler "__patchable_function_entries" } } */

/* x86_64: Should have exactly 11 prefix NOPs between .LPFE and .type */
/* { dg-final { scan-assembler {\.LPFE[0-9]+:\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*nop\n\t*\.type} { target x86_64-*-* } } } */

/* x86_64: Should have 0 entry NOPs - function starts immediately with pushq (no __kcfi_typeid for definitions) */
/* { dg-final { scan-assembler {test_function:\n\.LFB[0-9]+:\n\t*\.cfi_startproc\n\t*pushq\t*%rbp} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-not {\t*\.weak\t*__kcfi_typeid_test_function\n} { target x86_64-*-* } } } */

/* x86_64: KCFI should have 0 NOPs - goes directly to movl (offsetToAlignment(11+5,16)=0) */
/* { dg-final { scan-assembler {__cfi_test_function:\n\t*movl} { target x86_64-*-* } } } */

/* x86_64: Validate KCFI type ID is present */
/* { dg-final { scan-assembler {movl\t\$0x[0-9a-f]+, %eax} { target x86_64-*-* } } } */

/* x86_64: Call site should use -15 offset (kernel-style encoding) */
/* { dg-final { scan-assembler {movl\t\$-?[0-9]+, %r10d\n\taddl\t-15\(%r[a-z0-9]+\), %r10d} { target x86_64-*-* } } } */

/* AArch64: Call site should use -15 offset */
/* { dg-final { scan-assembler {ldur\tw16, \[x[0-9]+, #-15\]} { target aarch64*-*-* } } } */
