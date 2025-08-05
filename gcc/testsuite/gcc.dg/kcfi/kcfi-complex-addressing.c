/* Test KCFI with complex addressing modes (structure members, array elements) */
/* This is a regression test for the change_address_1 RTL error that occurred */
/* when target_addr was PLUS(reg, offset) instead of a simple register */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -O2" } */

struct function_table {
    int (*callback1)(int);
    int (*callback2)(int, int);
    void (*callback3)(void);
    int data;
};

static int handler1(int x) {
    return x * 2;
}

static int handler2(int x, int y) {
    return x + y;
}

static void handler3(void) {
    /* Empty handler */
}

/* Test indirect calls through structure members - this creates PLUS(reg, offset) addressing */
int test_struct_members(struct function_table *table) {
    int result = 0;

    /* These indirect calls will generate complex addressing modes:
     * call *(%rdi)          - callback1 at offset 0
     * call *8(%rdi)         - callback2 at offset 8
     * call *16(%rdi)        - callback3 at offset 16
     * Our KCFI instrumentation must handle PLUS(reg, struct_offset) + kcfi_offset */

    result += table->callback1(10);        /* MEM[PLUS(reg, 0)] -> PLUS(reg, -4) for KCFI */
    result += table->callback2(5, 7);      /* MEM[PLUS(reg, 8)] -> PLUS(reg, 4) for KCFI */
    table->callback3();                    /* MEM[PLUS(reg, 16)] -> PLUS(reg, 12) for KCFI */

    return result;
}

/* Test indirect calls through array elements - another source of complex addressing */
typedef int (*func_array_t)(int);

int test_array_elements(func_array_t functions[], int index) {
    /* This creates addressing like MEM[PLUS(PLUS(reg, index*8), 0)]
     * which should be simplified to MEM[PLUS(reg, index*8)] */
    return functions[index](42);  /* Complex array indexing */
}

/* Test with global structure */
static struct function_table global_table = {
    .callback1 = handler1,
    .callback2 = handler2,
    .callback3 = handler3,
    .data = 100
};

int test_global_struct(void) {
    /* Access through global structure - may generate different addressing patterns */
    return global_table.callback1(20) + global_table.callback2(3, 4);
}

/* Test nested structure access */
struct nested_table {
    struct function_table inner;
    int extra_data;
};

int test_nested_struct(struct nested_table *nested) {
    /* Even more complex addressing: nested structure member access */
    return nested->inner.callback1(15);  /* MEM[PLUS(reg, inner_offset + callback1_offset)] */
}

int main() {
    struct function_table local_table = {
        .callback1 = handler1,
        .callback2 = handler2,
        .callback3 = handler3,
        .data = 50
    };

    func_array_t func_array[] = { handler1, handler1, handler1 };

    int result = 0;
    result += test_struct_members(&local_table);
    result += test_array_elements(func_array, 1);
    result += test_global_struct();

    struct nested_table nested = { .inner = local_table, .extra_data = 200 };
    result += test_nested_struct(&nested);

    return result;
}

/* Verify that all address-taken functions get KCFI preambles */
/* { dg-final { scan-assembler {__cfi_handler1:} } } */
/* { dg-final { scan-assembler {__cfi_handler2:} } } */
/* { dg-final { scan-assembler {__cfi_handler3:} } } */

/* x86_64: Verify KCFI checks are generated for indirect calls through complex addressing */
/* The key test: these should compile without change_address_1 errors (kernel-style encoding) */
/* { dg-final { scan-assembler {movl\t\$-?[0-9]+, %r10d\n\taddl\t-4\(%r[a-z0-9]+\), %r10d} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler {ud2} { target x86_64-*-* } } } */

/* AArch64: Verify KCFI checks for complex addressing */
/* { dg-final { scan-assembler {ldur\tw16, \[x[0-9]+, #-4\]} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler {brk} { target aarch64*-*-* } } } */

/* Should have trap section (x86 only - AArch64 uses brk immediate) */
/* { dg-final { scan-assembler {\.kcfi_traps} { target x86_64-*-* } } } */
