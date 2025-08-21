/* Test that no_sanitize("kcfi") attribute is preserved during inlining.  */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -O2" } */
/* { dg-options "-fsanitize=kcfi -O2 -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

extern void external_side_effect(int value);

/* Regular function (should get KCFI checks) */
__attribute__((noinline))
void normal_function(void (*callback)(int))
{
    /* This indirect call must generate KCFI checks.  */
    callback(300);
    external_side_effect(300);
}

/* Regular function marked with no_sanitize("kcfi") (positive control) */
__attribute__((noinline, no_sanitize("kcfi")))
void sensitive_non_inline_function(void (*callback)(int))
{
    /* This indirect call should NOT generate KCFI checks.  */
    callback(100);
    external_side_effect(100);
}

/* Function marked with both no_sanitize("kcfi") and always_inline.  */
__attribute__((always_inline, no_sanitize("kcfi")))
static inline void sensitive_inline_function(void (*callback)(int))
{
    /* This indirect call should NOT generate KCFI checks when inlined.  */
    callback(42);
    external_side_effect(42);
}

/* Explicit wrapper for testing sensitive_inline_function behavior.  */
__attribute__((noinline))
void wrap_sensitive_inline(void (*callback)(int))
{
    sensitive_inline_function(callback);
}

/* Function marked with only always_inline (should get KCFI checks) */
__attribute__((always_inline))
static inline void normal_inline_function(void (*callback)(int))
{
    /* This indirect call must generate KCFI checks when inlined.  */
    callback(200);
    external_side_effect(200);
}

/* Explicit wrapper for testing normal_inline_function behavior.  */
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
/* { dg-final { scan-assembler-times {ud2} 2 { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-times {brk\s+#[0-9]+} 2 { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler-times {udf\s+#[0-9]+} 2 { target arm32 } } } */
/* { dg-final { scan-assembler-times {ebreak} 2 { target riscv*-*-* } } } */

/* Positive controls: these should have KCFI checks.  */
/* { dg-final { scan-assembler {normal_function:.*ud2.*\.size\s+normal_function} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler {wrap_normal_inline:.*ud2.*\.size\s+wrap_normal_inline} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler {normal_function:.*brk\s+#[0-9]+.*\.size\s+normal_function} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler {wrap_normal_inline:.*brk\s+#[0-9]+.*\.size\s+wrap_normal_inline} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler {normal_function:.*udf\t#[0-9]+.*\.size\s+normal_function} { target arm32 } } } */
/* { dg-final { scan-assembler {wrap_normal_inline:.*udf\t#[0-9]+.*\.size\s+wrap_normal_inline} { target arm32 } } } */
/* { dg-final { scan-assembler {normal_function:.*ebreak.*\.size\s+normal_function} { target riscv*-*-* } } } */
/* { dg-final { scan-assembler {wrap_normal_inline:.*ebreak.*\.size\s+wrap_normal_inline} { target riscv*-*-* } } } */

/* Negative controls: these should NOT have KCFI checks.  */
/* { dg-final { scan-assembler-not {sensitive_non_inline_function:.*ud2.*\.size\s+sensitive_non_inline_function} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-not {wrap_sensitive_inline:.*ud2.*\.size\s+wrap_sensitive_inline} { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-not {sensitive_non_inline_function:.*brk\s+#[0-9]+.*\.size\s+sensitive_non_inline_function} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler-not {wrap_sensitive_inline:.*brk\s+#[0-9]+.*\.size\s+wrap_sensitive_inline} { target aarch64*-*-* } } } */
/* { dg-final { scan-assembler-not {sensitive_non_inline_function:[^\n]*udf\t#[0-9]+[^\n]*\.size\tsensitive_non_inline_function} { target arm32 } } } */
/* { dg-final { scan-assembler-not {wrap_sensitive_inline:[^\n]*udf\t#[0-9]+[^\n]*\.size\twrap_sensitive_inline} { target arm32 } } } */
/* { dg-final { scan-assembler-not {sensitive_non_inline_function:.*ebreak.*\.size\s+sensitive_non_inline_function} { target riscv*-*-* } } } */
/* { dg-final { scan-assembler-not {wrap_sensitive_inline:.*ebreak.*\.size\s+wrap_sensitive_inline} { target riscv*-*-* } } } */
