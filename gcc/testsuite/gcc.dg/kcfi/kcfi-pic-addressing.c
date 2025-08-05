/* Test KCFI with position-independent code addressing modes */
/* This is a regression test for complex addressing like PLUS(PLUS(...), symbol_ref) */
/* which can occur with PIC and caused change_address_1 RTL errors */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -O2 -fpic" } */

/* Global function pointer table that creates PIC addressing */
struct callbacks {
    int (*handler1)(int);
    void (*handler2)(void);
    int (*handler3)(int, int);
};

static int simple_handler(int x) {
    return x * 2;
}

static void void_handler(void) {
    /* Empty handler */
}

static int complex_handler(int a, int b) {
    return a + b;
}

/* Global structure that will require PIC addressing */
struct callbacks global_callbacks = {
    .handler1 = simple_handler,
    .handler2 = void_handler,
    .handler3 = complex_handler
};

/* Function that uses PIC addressing to access global callbacks */
int test_pic_addressing(int value) {
    /* These indirect calls through global structure create complex addressing
     * like PLUS(PLUS(GOT_base, symbol_offset), struct_offset) which previously
     * caused RTL errors in KCFI instrumentation */

    int result = 0;
    result += global_callbacks.handler1(value);

    global_callbacks.handler2();

    result += global_callbacks.handler3(value, result);

    return result;
}

/* Test with function pointer arrays */
static int (*func_array[])(int) = {
    simple_handler,
    simple_handler,
    simple_handler
};

int test_pic_array(int index, int value) {
    /* Array access with PIC can also create complex addressing */
    return func_array[index % 3](value);
}

/* Test with dynamic PIC addressing */
struct callbacks *get_callbacks(void) {
    return &global_callbacks;
}

int test_dynamic_pic(int value) {
    /* Dynamic access through function call creates very complex addressing */
    struct callbacks *cb = get_callbacks();
    return cb->handler1(value) + cb->handler3(value, value);
}

int main() {
    int result = 0;
    result += test_pic_addressing(10);
    result += test_pic_array(1, 20);
    result += test_dynamic_pic(5);
    return result;
}

/* Verify that all address-taken functions get KCFI preambles */
/* { dg-final { scan-assembler {__cfi_simple_handler:} } } */
/* { dg-final { scan-assembler {__cfi_void_handler:} } } */
/* { dg-final { scan-assembler {__cfi_complex_handler:} } } */

/* x86_64: Verify KCFI checks are generated (should compile without RTL errors, kernel-style encoding) */
/* { dg-final { scan-assembler {movl\t\$-?[0-9]+, %r10d\n\taddl\t-4\(%r[a-z0-9]+\), %r10d} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler {ud2} { target x86_64-*-* } } } */

/* AArch64: Verify KCFI checks */
/* { dg-final { scan-assembler {ldur\tw16, \[x[0-9]+, #-4\]} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler {brk} { target aarch64*-*-* } } } */

/* Should have trap section */
/* { dg-final { scan-assembler {\.kcfi_traps} { target x86_64-*-* } } } */
