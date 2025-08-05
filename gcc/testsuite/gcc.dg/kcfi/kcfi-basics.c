/* Test basic KCFI functionality - preamble generation */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi" } */

/* Extern function declarations - should NOT get KCFI preambles */
extern void external_func(void);
extern int external_func_int(int x);

void regular_function(int x) {
    /* This should get KCFI preamble */
}

void static_target_function(int x) {
    /* Target function that can be called indirectly */
}

static void static_caller(void) {
    /* Static function that makes an indirect call */
    /* Should NOT get KCFI preamble (not address-taken) */
    /* But SHOULD generate KCFI check for the indirect call */
    void (*local_ptr)(int) = static_target_function;
    local_ptr(42);  /* This should generate KCFI check */
}

void (*func_ptr)(int) = regular_function;
void (*ext_ptr)(void) = external_func;  /* Make external_func address-taken */

int main() {
    func_ptr(42);
    ext_ptr();        /* Indirect call to external_func */
    external_func_int(10);  /* Direct call to external_func_int */
    static_caller();  /* Direct call to static function */
    return 0;
}

/* Verify KCFI preamble exists for regular_function */
/* { dg-final { scan-assembler {__cfi_regular_function:} } } */

/* Target function should have preamble (address-taken) */
/* { dg-final { scan-assembler {__cfi_static_target_function:} } } */

/* Static caller should NOT have preamble (it's only called directly, not address-taken) */
/* { dg-final { scan-assembler-not {__cfi_static_caller:} } } */

/* x86_64: Verify type ID in preamble (after NOPs, before function label) */
/* { dg-final { scan-assembler {__cfi_regular_function:\n\t+nop\n.*\n\t+movl\t+\$0x[0-9a-f]+, %eax} { target x86_64-*-* } } } */

/* x86_64: Static function should generate KCFI check for indirect call */
/* { dg-final { scan-assembler {movl\t\$-?[0-9]+, %r10d\n\taddl\t-4\(%r[a-z0-9]+\), %r10d} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler {ud2} { target x86_64-*-* } } } */

/* AArch64: Verify type ID word in preamble */
/* { dg-final { scan-assembler {__cfi_regular_function:\n\t\.word 0x[0-9a-f]+} { target aarch64*-*-* } } } */

/* AArch64: Static function should generate KCFI check for indirect call */
/* { dg-final { scan-assembler {ldur\tw16, \[x[0-9]+, #-4\]} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler {brk} { target aarch64*-*-* } } } */

/* Extern functions should NOT get KCFI preambles */
/* { dg-final { scan-assembler-not {__cfi_external_func:} } } */
/* { dg-final { scan-assembler-not {__cfi_external_func_int:} } } */

/* Local functions should NOT get __kcfi_typeid_ symbols */
/* Only external declarations that are address-taken should get __kcfi_typeid_ */
/* { dg-final { scan-assembler-not {__kcfi_typeid_regular_function} } } */
/* { dg-final { scan-assembler-not {__kcfi_typeid_main} } } */

/* External address-taken functions should get __kcfi_typeid_ symbols */
/* { dg-final { scan-assembler {__kcfi_typeid_external_func} } } */

/* External functions that are only called directly should NOT get __kcfi_typeid_ symbols */
/* { dg-final { scan-assembler-not {__kcfi_typeid_external_func_int} } } */

/* Should have trap section for KCFI checks */
/* { dg-final { scan-assembler {\.kcfi_traps} { target x86_64-*-* } } } */
