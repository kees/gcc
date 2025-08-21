/* Test basic KCFI functionality - preamble generation.  */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi" } */
/* { dg-options "-fsanitize=kcfi -falign-functions=16" { target x86_64-*-* } } */
/* { dg-options "-fsanitize=kcfi -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

/* Extern function declarations - should NOT get KCFI preambles.  */
extern void external_func(void);
extern int external_func_int(int x);

void regular_function(int x) {
    /* This should get KCFI preamble.  */
}

void static_target_function(int x) {
    /* Target function that can be called indirectly.  */
}

static void static_caller(void) {
    /* Static function that makes an indirect call
       Should NOT get KCFI preamble (not address-taken)
       But must generate KCFI check for the indirect call.  */
    void (*local_ptr)(int) = static_target_function;
    local_ptr(42);  /* This should generate KCFI check.  */
}

/* Make external_func address-taken.  */
void (*func_ptr)(int) = regular_function;
void (*ext_ptr)(void) = external_func;

int main() {
    func_ptr(42);
    ext_ptr();        /* Indirect call to external_func.  */
    external_func_int(10);  /* Direct call to external_func_int.  */
    static_caller();  /* Direct call to static function.  */
    return 0;
}

/* Verify KCFI preamble exists for regular_function.  */
/* { dg-final { scan-assembler {__cfi_regular_function:} } } */

/* Verify KCFI preamble symbol comes before main function symbol.  */
/* { dg-final { scan-assembler {__cfi_regular_function:.*regular_function:} } } */

/* Target function should have preamble (address-taken).  */
/* { dg-final { scan-assembler {__cfi_static_target_function:} } } */

/* Static caller should NOT have preamble (it's only called directly,
   not address-taken). */
/* { dg-final { scan-assembler-not {__cfi_static_caller:} } } */

/* x86_64: Verify type ID in preamble (after NOPs, before function label) */
/* { dg-final { scan-assembler {__cfi_regular_function:\n\t+nop\n.*\n\t+movl\t+\$0x[0-9a-f]+, %eax} { target x86_64-*-* } } } */

/* AArch64: Verify type ID word in preamble.  */
/* { dg-final { scan-assembler {__cfi_regular_function:\n\t\.word\t0x[0-9a-f]+} { target aarch64*-*-* } } } */

/* ARM 32-bit: Verify type ID word in preamble.  */
/* { dg-final { scan-assembler {__cfi_regular_function:\n\t\.word\t0x[0-9a-f]+} { target arm32 } } } */

/* RISC-V: Verify type ID word in preamble */
/* { dg-final { scan-assembler {__cfi_regular_function:\n\t\.word\t0x[0-9a-f]+} { target riscv*-*-* } } } */

/* x86_64: Static function should generate complete KCFI check sequence.  */
/* { dg-final { scan-assembler {movl\t\$-?[0-9]+, %r10d\n\taddl\t-4\(%r[a-z0-9]+\), %r10d\n\tje\t(\.Lkcfi_call[0-9]+)\n\.Lkcfi_trap[0-9]+:\n\tud2\n.*\n\1:\n\tcall} { target x86_64-*-* } } } */

/* AArch64: Static function should generate complete KCFI check sequence.  */
/* { dg-final { scan-assembler {ldur\tw16, \[x[0-9]+, #-4\]\n\tmov\tw17, #[0-9]+\n\tmovk\tw17, #[0-9]+, lsl #16\n\tcmp\tw16, w17\n\tb\.eq\t(\.Lkcfi_call[0-9]+)\n\.Lkcfi_trap[0-9]+:\n\tbrk\t#[0-9]+\n\1:\n\tblr} { target aarch64*-*-* } } } */

/* ARM 32-bit: Static function should generate complete KCFI check sequence
   with stack spilling.  */
/* { dg-final { scan-assembler {push\t\{r0, r1\}\n\tldr\tr0, \[r[0-9]+, #-4\]\n\tmovw\tr1, #[0-9]+\n\tmovt\tr1, #[0-9]+\n\tcmp\tr0, r1\n\tpop\t\{r0, r1\}\n\tbeq\t\.Lkcfi_call[0-9]+\n\.Lkcfi_trap[0-9]+:\n\tudf\t#[0-9]+\n\.Lkcfi_call[0-9]+:\n\tblx\tr[0-9]+} { target arm32 } } } */

/* RISC-V: Static function should generate KCFI check for indirect call.  */
/* { dg-final { scan-assembler {lw\tt1, -4\([a-z0-9]+\)\n\tlui\tt2, [0-9]+\n\taddiw\tt2, t2, -?[0-9]+\n\tbeq\tt1, t2, (\.Lkcfi_call[0-9]+)\n\.Lkcfi_trap[0-9]+:\n\tebreak\n\t\.section\t\.kcfi_traps,"ao",@progbits,\.text\n\.Lkcfi_entry[0-9]+:\n\t\.4byte\t\.Lkcfi_trap[0-9]+-\.Lkcfi_entry[0-9]+\n\t\.text\n\1:\n\tjalr} { target riscv*-*-* } } } */

/* Extern functions should NOT get KCFI preambles.  */
/* { dg-final { scan-assembler-not {__cfi_external_func:} } } */
/* { dg-final { scan-assembler-not {__cfi_external_func_int:} } } */

/* Local functions should NOT get __kcfi_typeid_ symbols.  */
/* Only external declarations that are address-taken should get __kcfi_typeid_ */
/* { dg-final { scan-assembler-not {__kcfi_typeid_regular_function} } } */
/* { dg-final { scan-assembler-not {__kcfi_typeid_main} } } */

/* External address-taken functions should get __kcfi_typeid_ symbols.  */
/* { dg-final { scan-assembler {__kcfi_typeid_external_func} } } */

/* External functions that are only called directly should NOT get
   __kcfi_typeid_ symbols.  */
/* { dg-final { scan-assembler-not {__kcfi_typeid_external_func_int} } } */

/* Should have trap section for KCFI checks.  */
/* { dg-final { scan-assembler {\.kcfi_traps} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler {\.kcfi_traps} { target riscv*-*-* } } } */

/* AArch64 should NOT have trap section (uses brk immediate instead).  */
/* { dg-final { scan-assembler-not {\.kcfi_traps} { target aarch64*-*-* } } } */

/* ARM 32-bit should NOT have trap section (uses udf immediate instead).  */
/* { dg-final { scan-assembler-not {\.kcfi_traps} { target arm32 } } } */
