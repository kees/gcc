/* Test KCFI call-site offset validation across architectures.  */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi" } */
/* { dg-options "-fsanitize=kcfi -falign-functions=16" { target x86_64-*-* } } */
/* { dg-options "-fsanitize=kcfi -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

void target_func_a(void) { }
void target_func_b(int x) { }
void target_func_c(int x, int y) { }

int main() {
    void (*ptr_a)(void) = target_func_a;
    void (*ptr_b)(int) = target_func_b;
    void (*ptr_c)(int, int) = target_func_c;

    /* Multiple indirect calls.  */
    ptr_a();
    ptr_b(1);
    ptr_c(1, 2);

    return 0;
}

/* Should have KCFI preambles for all functions.  */
/* { dg-final { scan-assembler "__cfi_target_func_a:" } } */
/* { dg-final { scan-assembler "__cfi_target_func_b:" } } */
/* { dg-final { scan-assembler "__cfi_target_func_c:" } } */

/* x86_64: All call sites should use -4 offset for KCFI type ID loads, even
   with -falign-functions=16 (we're not using patchable entries here).  */
/* { dg-final { scan-assembler {movl\t\$-?[0-9]+, %r10d\n\taddl\t-4\(%r[a-z0-9]+\), %r10d} { target x86_64-*-* } } } */

/* AArch64: All call sites should use -4 offset.  */
/* { dg-final { scan-assembler {ldur\tw16, \[x[0-9]+, #-4\]} { target aarch64*-*-* } } } */

/* ARM 32-bit: All call sites should use -4 offset with stack spilling.  */
/* { dg-final { scan-assembler {ldr\tr0, \[r[0-9]+, #-4\]} { target arm32 } } } */

/* RISC-V: All call sites should use -4 offset.  */
/* { dg-final { scan-assembler {lw\tt1, -4\(} { target riscv*-*-* } } } */

/* Should have trap section.  */
/* { dg-final { scan-assembler {\.kcfi_traps} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler {\.kcfi_traps} { target riscv*-*-* } } } */

/* AArch64 should NOT have trap section (uses brk immediate instead) */
/* { dg-final { scan-assembler-not {\.kcfi_traps} { target aarch64*-*-* } } } */

/* ARM 32-bit should NOT have trap section (uses udf immediate instead) */
/* { dg-final { scan-assembler-not {\.kcfi_traps} { target arm32 } } } */
