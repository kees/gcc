/* Test KCFI IPA pass robustness with compiler-generated constructs */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -O2" } */

#include <stddef.h>

/* Test various compiler-generated constructs that could confuse IPA pass */

/* 1. static_assert - this was causing the original crash */
typedef struct {
    int field1;
    char field2;
} test_struct_t;

static_assert(offsetof(test_struct_t, field1) == 0, "layout check 1");
static_assert(offsetof(test_struct_t, field2) == 4, "layout check 2");
static_assert(sizeof(test_struct_t) >= 5, "size check");

/* 2. Regular functions that should get KCFI analysis */
void regular_function(void) {
    /* Should get KCFI preamble */
}

static void static_function(void) {
    /* With -O2: correctly identified as not address-taken, no preamble */
}

void address_taken_function(void) {
    /* Should get KCFI preamble (address taken below) */
}

/* 3. Function pointer to create address-taken scenario */
void (*func_ptr)(void) = address_taken_function;

/* 4. More static_asserts mixed with function definitions */
static_assert(sizeof(void*) == 8, "pointer size check");

int main(void) {
    regular_function();    /* Direct call */
    static_function();     /* Direct call to static */
    func_ptr();           /* Indirect call */

    static_assert(sizeof(int) == 4, "int size check");

    return 0;
}

/* Verify KCFI preambles are generated appropriately */
/* { dg-final { scan-assembler "__cfi_regular_function:" } } */
/* { dg-final { scan-assembler "__cfi_address_taken_function:" } } */
/* { dg-final { scan-assembler "__cfi_main:" } } */

/* With -O2: static_function correctly identified as not address-taken */
/* { dg-final { scan-assembler-not "__cfi_static_function:" } } */
