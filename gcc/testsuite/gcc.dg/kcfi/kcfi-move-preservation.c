/* Test that KCFI preserves function pointer moves at -O2 optimization.
   This test ensures that the combine pass doesn't incorrectly optimize away
   the move instruction needed to transfer function pointers from argument
   registers to the target registers used by KCFI patterns.  */

/* { dg-do compile } */
/* { dg-options "-O2 -fsanitize=kcfi -std=gnu11" } */
/* { dg-options "-O2 -fsanitize=kcfi -std=gnu11 -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

static int called_count = 0;

/* Function taking one argument, returning void.  */
static __attribute__((noinline)) void increment_void(int *counter)
{
    (*counter)++;
}

/* Function taking one argument, returning int.  */
static __attribute__((noinline)) int increment_int(int *counter)
{
    (*counter)++;
    return *counter;
}

/* Don't allow the compiler to inline the calls.  */
static __attribute__((noinline)) void indirect_call(void (*func)(int *))
{
    func(&called_count);
}

int main(void)
{
    /* This should work - matching prototype.  */
    indirect_call(increment_void);

    /* This should trap - mismatched prototype.  */
    indirect_call((void *)increment_int);

    return 0;
}

/* Verify complete KCFI check sequence with preserved move instruction. At
   -O2, the combine pass previously optimized away the move from %rdi to %rax,
   breaking KCFI. Verify the full sequence is preserved. */

/* x86_64: Complete KCFI sequence with move preservation and indirect jump.  */
/* { dg-final { scan-assembler {(indirect_call):.*\n.*movq\s+%rdi,\s+(%rax)\n.*movl\s+\$[0-9]+,\s+%r10d\n\taddl\s+-4\(\2\),\s+%r10d\n\tje\s+\.Lkcfi_call[0-9]+\n\.Lkcfi_trap[0-9]+:\n\tud2.*\.Lkcfi_call[0-9]+:\n\tjmp\s+\*\2.*\.size\s+\1,\s+\.-\1} { target x86_64-*-* } } } */

/* AArch64: Complete KCFI sequence with move preservation and indirect branch.  */
/* { dg-final { scan-assembler {(indirect_call):.*\n.*mov\s+(x[0-9]+),\s+x0\n.*ldur\s+w16,\s+\[\2,\s+#-4\]\n\tmov\s+w17,\s+#[0-9]+\n\tmovk\s+w17,\s+#[0-9]+,\s+lsl\s+#16\n\tcmp\s+w16,\s+w17\n\tb\.eq\s+\.Lkcfi_call[0-9]+\n\.Lkcfi_trap[0-9]+:\n\tbrk\s+#[0-9]+.*\.Lkcfi_call[0-9]+:\n\tbr\s+\2.*\.size\s+\1,\s+\.-\1} { target aarch64*-*-* } } } */

/* ARM32: Complete KCFI sequence with move preservation and indirect branch.  */
/* { dg-final { scan-assembler {(indirect_call):.*\n.*mov\s+(r[0-9]+),\s+r0\n.*push\s+\{r0,\s+r1\}\n\tldr\s+r0,\s+\[\2,\s+#-4\]\n\tmovw\s+r1,\s+#[0-9]+\n\tmovt\s+r1,\s+#[0-9]+\n\tcmp\s+r0,\s+r1\n\tpop\s+\{r0,\s+r1\}\n\tbeq\s+\.Lkcfi_call[0-9]+\n\.Lkcfi_trap[0-9]+:\n\tudf\s+#[0-9]+.*\.Lkcfi_call[0-9]+:\n\tbx\s+\2.*\.size\s+\1,\s+\.-\1} { target arm32 } } } */

/* RISC-V: Complete KCFI sequence with move preservation and indirect jump.  */
/* { dg-final { scan-assembler {(indirect_call):.*mv\s+(a[0-9]+),a0.*lw\s+t1,\s+-4\(\2\).*lui\s+t2,\s+[0-9]+.*addiw\s+t2,\s+t2,\s+-?[0-9]+.*beq\s+t1,\s+t2,\s+\.Lkcfi_call[0-9]+.*ebreak.*jalr\s+zero,\s+\2,\s+0.*\.size\s+\1,\s+\.-\1} { target riscv64-*-* } } } */
