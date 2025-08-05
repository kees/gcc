/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -O2" } */

/* Test that no_sanitize("kcfi") attribute is preserved during inlining */

extern void external_side_effect(int value);

/* Regular function (should get KCFI checks) */
__attribute__((noinline))
void normal_function(void (*callback)(int))
{
    /* This indirect call SHOULD generate KCFI checks */
    callback(300);
    external_side_effect(300);
}

/* Regular function marked with no_sanitize("kcfi") (positive control) */
__attribute__((noinline, no_sanitize("kcfi")))
void sensitive_non_inline_function(void (*callback)(int))
{
    /* This indirect call should NOT generate KCFI checks */
    callback(100);
    external_side_effect(100);
}

/* Function marked with both no_sanitize("kcfi") and always_inline */
__attribute__((always_inline, no_sanitize("kcfi")))
static inline void sensitive_inline_function(void (*callback)(int))
{
    /* This indirect call should NOT generate KCFI checks when inlined */
    callback(42);
    external_side_effect(42);
}

/* Explicit wrapper for testing sensitive_inline_function behavior */
__attribute__((noinline))
void wrap_sensitive_inline(void (*callback)(int))
{
    sensitive_inline_function(callback);
}

/* Function marked with only always_inline (should get KCFI checks) */
__attribute__((always_inline))
static inline void normal_inline_function(void (*callback)(int))
{
    /* This indirect call SHOULD generate KCFI checks when inlined */
    callback(200);
    external_side_effect(200);
}

/* Explicit wrapper for testing normal_inline_function behavior */
__attribute__((noinline))
void wrap_normal_inline(void (*callback)(int))
{
    normal_inline_function(callback);
}

void test_callback(int value)
{
    external_side_effect(value);
}

static void (*volatile function_pointer)(int) = test_callback;

int main(void)
{
    void (*fn_ptr)(int) = function_pointer;

    normal_function(fn_ptr);
    wrap_normal_inline(fn_ptr);
    sensitive_non_inline_function(fn_ptr);
    wrap_sensitive_inline(fn_ptr);

    return 0;
}

/* Verify correct number of KCFI checks: exactly 2 */
/* { dg-final { scan-assembler-times {ud2} 2 { target i?86-*-* x86_64-*-* } } } */
/* { dg-final { scan-assembler-times {brk\s+#33313} 2 { target aarch64*-*-* } } } */

/* Positive controls: these should have KCFI checks */
/* { dg-final { scan-assembler {normal_function:.*ud2.*\.size\s+normal_function} { target i?86-*-* x86_64-*-* } } } */
/* { dg-final { scan-assembler {wrap_normal_inline:.*ud2.*\.size\s+wrap_normal_inline} { target i?86-*-* x86_64-*-* } } } */
/* { dg-final { scan-assembler {normal_function:.*brk\s+#33313.*\.size\s+normal_function} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler {wrap_normal_inline:.*brk\s+#33313.*\.size\s+wrap_normal_inline} { target aarch64*-*-* } } } */

/* Negative controls: these should NOT have KCFI checks */
/* { dg-final { scan-assembler-not {sensitive_non_inline_function:.*ud2.*\.size\s+sensitive_non_inline_function} { target i?86-*-* x86_64-*-* } } } */
/* { dg-final { scan-assembler-not {wrap_sensitive_inline:.*ud2.*\.size\s+wrap_sensitive_inline} { target i?86-*-* x86_64-*-* } } } */
/* { dg-final { scan-assembler-not {sensitive_non_inline_function:.*brk\s+#33313.*\.size\s+sensitive_non_inline_function} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler-not {wrap_sensitive_inline:.*brk\s+#33313.*\.size\s+wrap_sensitive_inline} { target aarch64*-*-* } } } */
