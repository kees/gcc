/* Test KCFI cold function and cold partition behavior.  */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -O2" } */
/* { dg-options "-fsanitize=kcfi -O2 -march=armv7-a -mfloat-abi=soft" { target arm32 } } */
/* { dg-additional-options "-freorder-blocks-and-partition" { target freorder } } */

void regular_function(void) {
    /* Regular function should get preamble.  */
}

/* Cold-attributed function should STILL get preamble (it's a regular
   function, just marked cold).  */
__attribute__((cold))
void cold_attributed_function(void) {
    /* This function has cold attribute but should still get KCFI preamble.  */
}

/* Hot-attributed function should get preamble.  */
__attribute__((hot))
void hot_attributed_function(void) {
    /* This function is explicitly hot and should get KCFI preamble.  */
}

/* Global to prevent optimization from eliminating cold paths.  */
extern void abort(void);

/* Additional function to test that normal functions still get preambles.  */
__attribute__((noinline))
int another_regular_function(int x) {
    return x + 42;
}

/* Function designed to generate cold partitions under optimization.  */
__attribute__((noinline))
void function_with_cold_partition(int condition) {
    /* Hot path - very likely to execute.  */
    if (__builtin_expect(condition == 42, 1)) {
        /* Simple hot path that optimizer will keep inline.  */
        return;
    }

    /* Cold paths that actually do something to prevent elimination.  */
    if (__builtin_expect(condition < 0, 0)) {
        /* Error path 1 - call abort to prevent elimination.  */
        abort();
    }

    if (__builtin_expect(condition > 1000000, 0)) {
        /* Error path 2 - call abort to prevent elimination.  */
        abort();
    }

    if (__builtin_expect(condition == 999999, 0)) {
        /* Error path 3 - more substantial cold code.  */
        volatile int sum = 0;
        for (volatile int i = 0; i < 100; i++) {
            sum += i * condition;
        }
        if (sum > 0)
            abort();
    }

    /* More cold paths - switch with many unlikely cases.  */
    switch (condition) {
        case 1000001: case 1000002: case 1000003: case 1000004: case 1000005:
        case 1000006: case 1000007: case 1000008: case 1000009: case 1000010:
            /* Each case does some work before abort.  */
            volatile int work = condition * 2;
            if (work > 0) abort();
            break;
        default:
            if (condition != 42) {
                /* Fallback cold path - substantial work.  */
                volatile int result = 0;
                for (volatile int j = 0; j < condition % 50; j++) {
                    result += j;
                }
                if (result >= 0) abort();
            }
    }
}

/* Test function pointers to ensure address-taken detection works.  */
void test_function_pointers(void) {
    void (*regular_ptr)(void) = regular_function;
    void (*cold_ptr)(void) = cold_attributed_function;
    void (*hot_ptr)(void) = hot_attributed_function;

    regular_ptr();
    cold_ptr();
    hot_ptr();
}

int main() {
    regular_function();
    cold_attributed_function();
    hot_attributed_function();
    function_with_cold_partition(42); /* Normal case - stay in hot path.  */
    another_regular_function(5);
    test_function_pointers();
    return 0;
}

/* Regular function should have preamble.  */
/* { dg-final { scan-assembler "__cfi_regular_function:" } } */

/* Cold-attributed function should STILL have preamble (it's a legitimate function) */
/* { dg-final { scan-assembler "__cfi_cold_attributed_function:" } } */

/* Hot-attributed function should have preamble.  */
/* { dg-final { scan-assembler "__cfi_hot_attributed_function:" } } */

/* Function that generates cold partitions should have preamble for main entry.  */
/* { dg-final { scan-assembler "__cfi_function_with_cold_partition:" } } */

/* Address-taken functions should have preambles.  */
/* { dg-final { scan-assembler "__cfi_test_function_pointers:" } } */

/* The function should generate a .cold partition (only on targets that support freorder) */
/* { dg-final { scan-assembler "function_with_cold_partition\\.cold:" { target freorder } } } */

/* The .cold partition should NOT get a __cfi_ preamble since it's never
   reached via indirect calls.  */
/* { dg-final { scan-assembler-not "__cfi_function_with_cold_partition\\.cold:" { target freorder } } } */

/* Additional regular function should get preamble.  */
/* { dg-final { scan-assembler "__cfi_another_regular_function:" } } */

/* Test coverage summary:
   1. Cold-attributed function (__attribute__((cold))): SHOULD get preamble
   2. Cold partition (-freorder-blocks-and-partition): should NOT get preamble
   3. IPA split .part function (split_part=true): Logic in place, would skip if triggered

   Note: IPA function splitting (creating .part functions with split_part=true) requires
   specific optimization conditions that are difficult to trigger reliably in tests.
   The KCFI logic correctly handles this case using the split_part flag check.
*/
