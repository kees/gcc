/* Test KCFI protection when indirect calls get converted to tail calls.  */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -O2" } */
/* { dg-options "-fsanitize=kcfi -O2 -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

typedef int (*func_ptr_t)(int);
typedef void (*void_func_ptr_t)(void);

struct function_table {
    func_ptr_t process;
    void_func_ptr_t cleanup;
};

/* Target functions.  */
int process_data(int x) { return x * 2; }
void cleanup_data(void) {}

/* Initialize function table.  */
volatile struct function_table vtable = {
    .process = &process_data,
    .cleanup = &cleanup_data
};

/* Indirect call through struct member that should become tail call.  */
int test_struct_indirect_call(int x) {
    /* This is an indirect call that should be converted to tail call:
       Without -fno-optimize-sibling-calls should become "jmp *vtable+0(%rip)"
       With -fno-optimize-sibling-calls should become "call *vtable+0(%rip)"  */
    return vtable.process(x);
}

/* Indirect call through function pointer parameter.  */
int test_param_indirect_call(func_ptr_t handler, int x) {
    /* This is an indirect call that should be converted to tail call:
       Without -fno-optimize-sibling-calls should become "jmp *%rdi"
       With -fno-optimize-sibling-calls should be "call *%rdi"  */
    return handler(x);
}

/* Void indirect call through struct member.  */
void test_void_indirect_call(void) {
    /* This is an indirect call that should be converted to tail call:
     * Without -fno-optimize-sibling-calls: should become "jmp *vtable+8(%rip)"
     * With -fno-optimize-sibling-calls: should be "call *vtable+8(%rip)"  */
    vtable.cleanup();
}

/* Non-tail call for comparison (should always be call).  */
int test_non_tail_indirect_call(func_ptr_t handler, int x) {
    /* This should never become a tail call - always "call *%rdi"  */
    int result = handler(x);
    return result + 1;  /* Prevents tail call optimization.  */
}

/* Should have KCFI preambles for all functions.  */
/* { dg-final { scan-assembler-times "__cfi_process_data:" 1 } } */
/* { dg-final { scan-assembler-times "__cfi_cleanup_data:" 1 } } */
/* { dg-final { scan-assembler-times "__cfi_test_struct_indirect_call:" 1 } } */
/* { dg-final { scan-assembler-times "__cfi_test_param_indirect_call:" 1 } } */
/* { dg-final { scan-assembler-times "__cfi_test_void_indirect_call:" 1 } } */
/* { dg-final { scan-assembler-times "__cfi_test_non_tail_indirect_call:" 1 } } */

/* Should have exactly 4 KCFI checks for indirect calls as
   (load type ID + compare).  */
/* { dg-final { scan-assembler-times {movl\t\$-?[0-9]+, %r10d} 4 { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-times {addl\t-4\(%r[a-z0-9]+\), %r10d} 4 { target x86_64-*-* } } } */

/* Should have exactly 4 trap sections and 4 trap instructions.  */
/* { dg-final { scan-assembler-times "\\.kcfi_traps" 4 { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-times "ud2" 4 { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-times "\\.kcfi_traps" 4 { target riscv*-*-* } } } */
/* { dg-final { scan-assembler-times "ebreak" 4 { target riscv*-*-* } } } */

/* Should NOT have unprotected direct jumps to vtable.  */
/* { dg-final { scan-assembler-not {jmp\t\*vtable\(%rip\)} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-not {jmp\t\*vtable\+8\(%rip\)} { target x86_64-*-* } } } */

/* Should have exactly 3 protected tail calls (jmp through register after
   KCFI check).  */
/* { dg-final { scan-assembler-times {jmp\t\*%[a-z0-9]+} 3 { target x86_64-*-* } } } */

/* Should have exactly 1 regular call (non-tail call case).  */
/* { dg-final { scan-assembler-times {call\t\*%[a-z0-9]+} 1 { target x86_64-*-* } } } */

/* RISC-V: Should have exactly 4 KCFI checks for indirect calls
   (comparison instruction).  */
/* { dg-final { scan-assembler-times {beq\tt1, t2, \.Lkcfi_call[0-9]+} 4 { target riscv*-*-* } } } */

/* RISC-V: Should have exactly 4 KCFI checks for indirect calls as
   (load type ID + compare).  */
/* { dg-final { scan-assembler-times {lw\tt1, -4\([a-z0-9]+\)} 4 { target riscv*-*-* } } } */
/* { dg-final { scan-assembler-times {lui\tt2, [0-9]+} 4 { target riscv*-*-* } } } */

/* RISC-V: Should have exactly 3 protected tail calls (jr after
   KCFI check - no return address save).  */
/* { dg-final { scan-assembler-times {jalr\t(x0|zero), [a-z0-9]+, 0} 3 { target riscv*-*-* } } } */

/* RISC-V: Should have exactly 1 regular call (non-tail call case - saves
   return address).  */
/* { dg-final { scan-assembler-times {jalr\t(x1|ra), [a-z0-9]+, 0} 1 { target riscv*-*-* } } } */

/* Type ID loading should use lui + addiw pattern for 32-bit constants.  */
/* { dg-final { scan-assembler {lui\tt2, [0-9]+} { target riscv*-*-* } } } */
/* { dg-final { scan-assembler {addiw\tt2, t2, -?[0-9]+} { target riscv*-*-* } } } */

/* Should have exactly 4 KCFI checks for indirect calls (load type ID from
   -4 offset + compare).  */
/* { dg-final { scan-assembler-times {ldur\tw16, \[x[0-9]+, #-4\]} 4 { target aarch64-*-* } } } */
/* { dg-final { scan-assembler-times {cmp\tw16, w17} 4 { target aarch64-*-* } } } */

/* Should have exactly 4 trap instructions.  */
/* { dg-final { scan-assembler-times {brk\t#[0-9]+} 4 { target aarch64-*-* } } } */

/* Should have exactly 3 protected tail calls (br through register after
   KCFI check).  */
/* { dg-final { scan-assembler-times {br\tx[0-9]+} 3 { target aarch64-*-* } } } */

/* Should have exactly 1 regular call (non-tail call case).  */
/* { dg-final { scan-assembler-times {blr\tx[0-9]+} 1 { target aarch64-*-* } } } */

/* Type ID loading should use mov + movk pattern for 32-bit constants.  */
/* { dg-final { scan-assembler {mov\tw17, #[0-9]+} { target aarch64-*-* } } } */
/* { dg-final { scan-assembler {movk\tw17, #[0-9]+, lsl #16} { target aarch64-*-* } } } */

/* Should have exactly 4 KCFI checks for indirect calls (load type ID from
   -4 offset + compare).  */
/* { dg-final { scan-assembler-times {ldr\tr0, \[r[0-9]+, #-4\]} 4 { target arm32 } } } */
/* { dg-final { scan-assembler-times {cmp\tr0, r1} 4 { target arm32 } } } */

/* Should have exactly 4 trap instructions.  */
/* { dg-final { scan-assembler-times {udf\t#[0-9]+} 4 { target arm32 } } } */

/* Should have exactly 3 protected tail calls (bx through register after
   KCFI check).  */
/* { dg-final { scan-assembler-times {bx\tr[0-9]+} 3 { target arm32 } } } */

/* Should have exactly 1 regular call (non-tail call case).  */
/* { dg-final { scan-assembler-times {blx\tr[0-9]+} 1 { target arm32 } } } */

/* Type ID loading should use movw + movt pattern for 32-bit constants
   into r1.  */
/* { dg-final { scan-assembler {movw\tr1, #[0-9]+} { target arm32 } } } */
/* { dg-final { scan-assembler {movt\tr1, #[0-9]+} { target arm32 } } } */
