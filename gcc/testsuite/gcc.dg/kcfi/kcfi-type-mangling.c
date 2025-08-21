/* Test KCFI type ID hashing - verify different signatures generate different
   __kcfi_typeid_ symbols. Verifies the mangling strings via the dump file
   output.  */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -fdump-tree-kcfi0-details -fdump-ipa-ipa_kcfi-details" } */
/* { dg-options "-fsanitize=kcfi -fdump-tree-kcfi0-details -fdump-ipa-ipa_kcfi-details -march=armv7-a -mfloat-abi=soft" { target arm32 } } */

#include <stdarg.h>

/* Test __kcfi_typeid_ symbol generation for address-taken functions.
   Verify precise type discrimination using Itanium C++ ABI mangling. */

/* External function declarations - these will get __kcfi_typeid_ symbols
   when address-taken.  */
extern void func_void(void);                    /* _ZTSFvvE -> 0x40e0d3c8 */
extern void func_char(char x);                  /* _ZTSFvcE -> 0x64fce2f1 */
extern void func_int(int x);                    /* _ZTSFviE -> 0x70e35def */
extern void func_long(long x);                  /* _ZTSFvlE -> 0x24efb23e */

/* Basic types - verify exact type IDs match with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_void\n\t\.set\t__kcfi_typeid_func_void, 0x40e0d3c8} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_char\n\t\.set\t__kcfi_typeid_func_char, 0x64fce2f1} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_int\n\t\.set\t__kcfi_typeid_func_int, 0x70e35def} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_long\n\t\.set\t__kcfi_typeid_func_long, 0x24efb23e} } } */

/* Verify basic types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvvE' typeid=0x40e0d3c8} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvcE' typeid=0x64fce2f1} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFviE' typeid=0x70e35def} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvlE' typeid=0x24efb23e} kcfi0 } } */

/* Count verification - basic types (void type used by multiple functions).  */
/* { dg-final { scan-assembler-times {0x40e0d3c8} 4 } }
   +3 from local function preambles + memset test.  */
/* { dg-final { scan-assembler-times {0x64fce2f1} 1 } } */
/* { dg-final { scan-assembler-times {0x70e35def} 1 } } */
/* { dg-final { scan-assembler-times {0x24efb23e} 1 } } */

/* Pointer parameter types - must all differ.  */
extern void func_int_ptr(int *x);               /* _ZTSFvPiE -> 0xb2a15cf9 */
extern void func_char_ptr(char *x);             /* _ZTSFvPcE -> 0x1eaf7e87 */
extern void func_void_ptr(void *x);             /* _ZTSFvPvE -> 0xb2e442e6 */

/* Pointer types - verify they all differ with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_int_ptr\n\t\.set\t__kcfi_typeid_func_int_ptr, 0xb2a15cf9} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_char_ptr\n\t\.set\t__kcfi_typeid_func_char_ptr, 0x1eaf7e87} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_void_ptr\n\t\.set\t__kcfi_typeid_func_void_ptr, 0xb2e442e6} } } */

/* Verify pointer types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPiE' typeid=0xb2a15cf9} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPcE' typeid=0x1eaf7e87} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPvE' typeid=0xb2e442e6} kcfi0 } } */

/* Count verification - pointer types (will appear twice due to array decay
   earlier).  */
/* { dg-final { scan-assembler-times {0xb2a15cf9} 2 } } */
/* { dg-final { scan-assembler-times {0x1eaf7e87} 2 } } */
/* { dg-final { scan-assembler-times {0xb2e442e6} 1 } } */

/* Const qualifier discrimination - const vs non-const must have different
   type IDs.  */
extern void func_const_int_ptr(const int *x);   /* _ZTSFvPKiE -> const int* (must differ from int*) */
extern void func_const_char_ptr(const char *x); /* _ZTSFvPKcE -> const char* (must differ from char*) */
extern void func_const_void_ptr(const void *x); /* _ZTSFvPKvE -> const void* (must differ from void*) */

/* Const qualifier types - verify const vs non-const have different type IDs.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_const_int_ptr\n\t\.set\t__kcfi_typeid_func_const_int_ptr, 0x1dce360a} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_const_char_ptr\n\t\.set\t__kcfi_typeid_func_const_char_ptr, 0x39bf5794} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_const_void_ptr\n\t\.set\t__kcfi_typeid_func_const_void_ptr, 0x0dee7085} } } */

/* Verify const qualifier types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPKiE' typeid=0x1dce360a} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPKcE' typeid=0x39bf5794} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPKvE' typeid=0x0dee7085} kcfi0 } } */

/* Count verification - const qualifier types should appear exactly once.  */
/* { dg-final { scan-assembler-times {0x1dce360a} 1 } } */
/* { dg-final { scan-assembler-times {0x39bf5794} 2 } } +1 from non-variadic simple test.  */
/* { dg-final { scan-assembler-times {0x0dee7085} 1 } } */

/* Nested pointer types.  */
extern void func_int_ptr_ptr(int **x);          /* _ZTSFvPPiE -> 0xf61ef6c7 */
extern void func_char_ptr_ptr(char **x);        /* _ZTSFvPPcE -> 0x8a0f4239 */

/* Nested pointers with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_int_ptr_ptr\n\t\.set\t__kcfi_typeid_func_int_ptr_ptr, 0xf61ef6c7} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_char_ptr_ptr\n\t\.set\t__kcfi_typeid_func_char_ptr_ptr, 0x8a0f4239} } } */

/* Verify nested pointer types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPPiE' typeid=0xf61ef6c7} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPPcE' typeid=0x8a0f4239} kcfi0 } } */

/* Count verification - nested pointer types should appear exactly once.  */
/* { dg-final { scan-assembler-times {0xf61ef6c7} 1 } } */
/* { dg-final { scan-assembler-times {0x8a0f4239} 1 } } */

/* Multiple parameter types - order matters.  */
extern void func_int_char(int x, char y);       /* _ZTSFvicE -> 0x5b983d44 */
extern void func_char_int(char x, int y);       /* _ZTSFvciE -> 0x4dbf9e00 */
extern void func_two_int(int x, int y);         /* _ZTSFviiE -> 0x3fa71bba.  */

/* Multiple parameter tests with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_int_char\n\t\.set\t__kcfi_typeid_func_int_char, 0x5b983d44} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_char_int\n\t\.set\t__kcfi_typeid_func_char_int, 0x4dbf9e00} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_two_int\n\t\.set\t__kcfi_typeid_func_two_int, 0x3fa71bba} } } */

/* Verify multiple parameter types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvicE' typeid=0x5b983d44} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvciE' typeid=0x4dbf9e00} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFviiE' typeid=0x3fa71bba} kcfi0 } } */

/* Count verification - multiple parameter types should appear exactly once.  */
/* { dg-final { scan-assembler-times {0x5b983d44} 1 } } */
/* { dg-final { scan-assembler-times {0x4dbf9e00} 1 } } */
/* { dg-final { scan-assembler-times {0x3fa71bba} 1 } } */

/* Return types.  */
extern int func_return_int(void);               /* _ZTSFivE -> 0xb7f32039 */
extern char func_return_char(void);             /* _ZTSFcvE -> 0x9646527b */
extern void* func_return_ptr(void);             /* _ZTSFPvvE -> 0x81e76bc6 */

/* Return type tests with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_return_int\n\t\.set\t__kcfi_typeid_func_return_int, 0xb7f32039} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_return_char\n\t\.set\t__kcfi_typeid_func_return_char, 0x9646527b} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_return_ptr\n\t\.set\t__kcfi_typeid_func_return_ptr, 0x81e76bc6} } } */

/* Verify return types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFivE' typeid=0xb7f32039} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFcvE' typeid=0x9646527b} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFPvvE' typeid=0x81e76bc6} kcfi0 } } */

/* Count verification - return types should appear exactly once.  */
/* { dg-final { scan-assembler-times {0xb7f32039} 1 } } */
/* { dg-final { scan-assembler-times {0x9646527b} 1 } } */
/* { dg-final { scan-assembler-times {0x81e76bc6} 1 } } */

/* Array parameters - decay to pointers.  */
extern void func_int_array(int arr[]);          /* _ZTSFvPiE -> 0xb2a15cf9 (same as int*) */
extern void func_char_array(char arr[]);        /* _ZTSFvPcE -> 0x1eaf7e87 (same as char*) */

/* Array decay validation - arrays should have SAME type ID as corresponding pointers.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_int_array\n\t\.set\t__kcfi_typeid_func_int_array, 0xb2a15cf9} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_char_array\n\t\.set\t__kcfi_typeid_func_char_array, 0x1eaf7e87} } } */
/* Counted below. */

/* Function pointer parameters.  */
extern void func_fptr_void(void (*fp)(void));   /* _ZTSFvPFvvEE -> 0xc88f6251 */
extern void func_fptr_int(void (*fp)(int));     /* _ZTSFvPFviEE -> 0xc4bf13bc */
extern void func_fptr_ret_int(int (*fp)(void)); /* _ZTSFvPFivEE -> 0xf728b0c2 */

/* Function pointer parameter tests with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_fptr_void\n\t\.set\t__kcfi_typeid_func_fptr_void, 0xc88f6251} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_fptr_int\n\t\.set\t__kcfi_typeid_func_fptr_int, 0xc4bf13bc} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_fptr_ret_int\n\t\.set\t__kcfi_typeid_func_fptr_ret_int, 0xf728b0c2} } } */

/* Verify function pointer parameter types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPFvvEE' typeid=0xc88f6251} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPFviEE' typeid=0xc4bf13bc} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPFivEE' typeid=0xf728b0c2} kcfi0 } } */

/* Count verification - function pointer parameter types should appear exactly once.  */
/* { dg-final { scan-assembler-times {0xc88f6251} 1 } } */
/* { dg-final { scan-assembler-times {0xc4bf13bc} 1 } } */
/* { dg-final { scan-assembler-times {0xf728b0c2} 1 } } */

/* Variadic functions - must include 'z' marker for ellipsis parameter.  */
extern void func_variadic_simple(const char *fmt, ...);         /* _ZTSFvPKczE -> uses z for variadic.  */
extern void func_variadic_mixed(int x, const char *fmt, ...);   /* _ZTSFviPKczE -> int + const char* + variadic.  */
extern void func_variadic_multi(int x, char y, const char *fmt, ...); /* _ZTSFvicPKczE -> multiple params + variadic.  */

/* Audit log pattern - matches Linux kernel audit_log function signature.  */
struct audit_context { int dummy; };
extern void audit_log_pattern(struct audit_context *ctx,
                              unsigned int gfp_mask, int type,
                              const char *fmt, ...); /* _ZTSFvP13audit_contextjiPKczE */

/* va_start regression test.  */
void test_va_start_regression(float dummy, const char *fmt, ...);

/* Variadic function tests - must differ from non-variadic equivalents.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_variadic_simple\n\t\.set\t__kcfi_typeid_func_variadic_simple, 0xc948a054} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_variadic_mixed\n\t\.set\t__kcfi_typeid_func_variadic_mixed, 0x00fbb853} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_variadic_multi\n\t\.set\t__kcfi_typeid_func_variadic_multi, 0xe22e4c64} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_audit_log_pattern\n\t\.set\t__kcfi_typeid_audit_log_pattern, 0xa610bd06} } } */

/* Verify variadic function mangling includes 'z' marker.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPKczE' typeid=0xc948a054} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFviPKczE' typeid=0x00fbb853} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvicPKczE' typeid=0xe22e4c64} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvP13audit_contextjiPKczE' typeid=0xa610bd06} kcfi0 } } */

/* Count verification - variadic function types should appear exactly once.  */
/* { dg-final { scan-assembler-times {0xc948a054} 1 } } */
/* { dg-final { scan-assembler-times {0x00fbb853} 1 } } */
/* { dg-final { scan-assembler-times {0xe22e4c64} 1 } } */
/* { dg-final { scan-assembler-times {0xa610bd06} 1 } } */

/* Non-variadic equivalents - must differ from variadic versions.  */
extern void func_non_variadic_simple(const char *fmt);          /* _ZTSFvPKcE -> no z marker.  */
extern void func_non_variadic_mixed(int x, const char *fmt);    /* _ZTSFviPKcE -> no z marker.  */

/* Non-variadic function tests - must have different type IDs from variadic versions.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_non_variadic_simple\n\t\.set\t__kcfi_typeid_func_non_variadic_simple, 0x39bf5794} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_non_variadic_mixed\n\t\.set\t__kcfi_typeid_func_non_variadic_mixed, 0xddf27ea9} } } */

/* Verify non-variadic function mangling lacks 'z' marker.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPKcE' typeid=0x39bf5794} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFviPKcE' typeid=0xddf27ea9} kcfi0 } } */

/* Count verification - non-variadic function types should appear exactly once.  */
/* { dg-final { scan-assembler-times {0x39bf5794} 2 } } +1 from earlier const char* test.  */
/* { dg-final { scan-assembler-times {0xddf27ea9} 1 } } */

/* Struct/union/enum parameter types: each struct name must produce different type IDs.  */
struct test_struct_a { int x; };
struct test_struct_b { int y; };
struct test_struct_c { int z; };
union test_union_a { int i; float f; };
union test_union_b { int j; float g; };
enum test_enum_a { ENUM_A_VAL1, ENUM_A_VAL2 };
enum test_enum_b { ENUM_B_VAL1, ENUM_B_VAL2 };

/* Functions taking struct pointers - must have different type IDs.  */
extern void func_struct_a_ptr(struct test_struct_a *x);  /* _ZTSFv14test_struct_aPiE -> unique.  */
extern void func_struct_b_ptr(struct test_struct_b *x);  /* _ZTSFv14test_struct_bPiE -> unique.  */
extern void func_struct_c_ptr(struct test_struct_c *x);  /* _ZTSFv14test_struct_cPiE -> unique.  */

/* Struct pointer tests with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_a_ptr\n\t\.set\t__kcfi_typeid_func_struct_a_ptr, 0x784c51f8} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_b_ptr\n\t\.set\t__kcfi_typeid_func_struct_b_ptr, 0x8845af63} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_c_ptr\n\t\.set\t__kcfi_typeid_func_struct_c_ptr, 0x2c475d26} } } */

/* Verify struct pointer types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvP13test_struct_aE' typeid=0x784c51f8} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvP13test_struct_bE' typeid=0x8845af63} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvP13test_struct_cE' typeid=0x2c475d26} kcfi0 } } */

/* Count verification - struct pointer types should appear exactly once.  */
/* { dg-final { scan-assembler-times {0x784c51f8} 1 } } */
/* { dg-final { scan-assembler-times {0x8845af63} 1 } } */
/* { dg-final { scan-assembler-times {0x2c475d26} 1 } } */

/* Functions taking const struct pointers - must differ from
   non-const versions.  */
extern void func_const_struct_a_ptr(const struct test_struct_a *x);  /* _ZTSFvPK14test_struct_aE -> unique, different from non-const.  */
extern void func_const_struct_b_ptr(const struct test_struct_b *x);  /* _ZTSFvPK14test_struct_bE -> unique, different from non-const.  */
extern void func_const_struct_c_ptr(const struct test_struct_c *x);  /* _ZTSFvPK14test_struct_cE -> unique, different from non-const.  */

/* Const struct pointer tests with precise patterns - must differ
   from non-const.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_const_struct_a_ptr\n\t\.set\t__kcfi_typeid_func_const_struct_a_ptr, 0xe57ff62f} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_const_struct_b_ptr\n\t\.set\t__kcfi_typeid_func_const_struct_b_ptr, 0xd58698c4} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_const_struct_c_ptr\n\t\.set\t__kcfi_typeid_func_const_struct_c_ptr, 0xa98414e9} } } */

/* Verify const struct pointer types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPK13test_struct_aE' typeid=0xe57ff62f} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPK13test_struct_bE' typeid=0xd58698c4} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvPK13test_struct_cE' typeid=0xa98414e9} kcfi0 } } */

/* Count verification - const struct pointer types should appear exactly once.  */
/* { dg-final { scan-assembler-times {0xe57ff62f} 1 } } */
/* { dg-final { scan-assembler-times {0xd58698c4} 1 } } */
/* { dg-final { scan-assembler-times {0xa98414e9} 1 } } */

extern void func_union_a_ptr(union test_union_a *x);     /* _ZTSFv13test_union_aPiE -> unique.  */
extern void func_union_b_ptr(union test_union_b *x);     /* _ZTSFv13test_union_bPiE -> unique.  */
extern void func_enum_a_ptr(enum test_enum_a *x);        /* _ZTSFv11test_enum_aPiE -> unique.  */
extern void func_enum_b_ptr(enum test_enum_b *x);        /* _ZTSFv11test_enum_bPiE -> unique.  */

/* Union member access discrimination test - prevents regression of
   union member bug.  */
struct tasklet_like_struct {
    int state;
    union {
	/* First union member - should NOT be used for callback calls.  */
        void (*func)(unsigned long data);
	/* Second union member - should be used for callback calls.  */
        void (*callback)(struct tasklet_like_struct *t);
    };
    unsigned long data;
};

/* Function with callback signature - this should match when accessed via
   union->callback.  */
extern void tasklet_callback_function(struct tasklet_like_struct *t);  /* _ZTSFvP19tasklet_like_structE -> unique.  */

/* Function with func signature - this should NOT match callback calls.  */
extern void tasklet_func_function(unsigned long data);                  /* _ZTSFvmE -> different from callback.  */

/* Union member access discrimination tests.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_tasklet_callback_function\n\t\.set\t__kcfi_typeid_tasklet_callback_function, 0x84fa4a3e} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_tasklet_func_function\n\t\.set\t__kcfi_typeid_tasklet_func_function, 0x80ee047b} } } */

/* Verify union member discrimination tests.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvP19tasklet_like_structE' typeid=0x84fa4a3e} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvmE' typeid=0x80ee047b} kcfi0 } } */

/* Union pointer tests with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_union_a_ptr\n\t\.set\t__kcfi_typeid_func_union_a_ptr, 0xfeec6097} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_union_b_ptr\n\t\.set\t__kcfi_typeid_func_union_b_ptr, 0xeef3032c} } } */

/* Verify union pointer types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvP12test_union_aE' typeid=0xfeec6097} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvP12test_union_bE' typeid=0xeef3032c} kcfi0 } } */

/* Count verification - union pointer types should appear exactly once.  */
/* { dg-final { scan-assembler-times {0xfeec6097} 1 } } */
/* { dg-final { scan-assembler-times {0xeef3032c} 1 } } */

/* Enum pointer tests with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_enum_a_ptr\n\t\.set\t__kcfi_typeid_func_enum_a_ptr, 0xd2bdb84a} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_enum_b_ptr\n\t\.set\t__kcfi_typeid_func_enum_b_ptr, 0xf2c02941} } } */

/* Verify enum pointer types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvP11test_enum_aE' typeid=0xd2bdb84a} kcfi0 } } */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvP11test_enum_bE' typeid=0xf2c02941} kcfi0 } } */

/* Count verification - enum pointer types should appear exactly once.  */
/* { dg-final { scan-assembler-times {0xd2bdb84a} 1 } } */
/* { dg-final { scan-assembler-times {0xf2c02941} 1 } } */

/* Count verification - union member discrimination types should appear exactly once.  */
/* The key test is that callback and func functions have DIFFERENT type IDs, proving union member discrimination works.  */
/* { dg-final { scan-assembler-times {0x84fa4a3e} 1 } } */
/* { dg-final { scan-assembler-times {0x80ee047b} 1 } } */

/* Indirect call through t->callback union must use correct callback
   type ID (0x84fa4a3e). The decimal value 2063971778 corresponds to
   0x84fa4a3e used in KCFI checks.  */
/* { dg-final { scan-assembler-times {\tmovl\t\$2063971778, %r10d} 1 { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-times {\tmov\tw17, #19006\n\tmovk\tw17, #34042, lsl #16} 1 { target aarch64-*-* } } } */
/* { dg-final { scan-assembler-times {\tpush\t\{r0, r1\}\n\tldr\tr0, \[r[0-9]+, #-4\]\n\tmovw\tr1, #19006\n\tmovt\tr1, #34042} 1 { target arm32 } } } */
/* { dg-final { scan-assembler-times {\tlui\tt2, 544677\n\taddiw\tt2, t2, -1474} 1 { target riscv*-*-* } } } */

/* Functions returning struct pointers - must have different type IDs.  */
extern struct test_struct_a* func_ret_struct_a_ptr(void); /* _ZTSF14test_struct_aPvE -> unique.  */
extern struct test_struct_b* func_ret_struct_b_ptr(void); /* _ZTSF14test_struct_bPvE -> unique.  */
extern struct test_struct_c* func_ret_struct_c_ptr(void); /* _ZTSF14test_struct_cPvE -> unique.  */

/* Struct return pointer tests with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_struct_a_ptr\n\t\.set\t__kcfi_typeid_func_ret_struct_a_ptr, 0x25780668} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_struct_b_ptr\n\t\.set\t__kcfi_typeid_func_ret_struct_b_ptr, 0xb1377aa5} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_struct_c_ptr\n\t\.set\t__kcfi_typeid_func_ret_struct_c_ptr, 0x0dc41dee} } } */

/* Verify struct return pointer types.  */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFP13test_struct_avE' typeid=0x25780668} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFP13test_struct_bvE' typeid=0xb1377aa5} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFP13test_struct_cvE' typeid=0x0dc41dee} kcfi0 } } */

/* { dg-final { scan-assembler-times {0x25780668} 1 } } */
/* { dg-final { scan-assembler-times {0xb1377aa5} 1 } } */
/* { dg-final { scan-assembler-times {0x0dc41dee} 1 } } */

/* Functions taking structs by value - must have different type IDs.  */
extern void func_struct_a_val(struct test_struct_a x);   /* _ZTSFv14test_struct_aE -> unique.  */
extern void func_struct_b_val(struct test_struct_b x);   /* _ZTSFv14test_struct_bE -> unique.  */
extern void func_struct_c_val(struct test_struct_c x);   /* _ZTSFv14test_struct_cE -> unique.  */

/* Struct by-value parameter tests with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_a_val\n\t\.set\t__kcfi_typeid_func_struct_a_val, 0xe0fb126a} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_b_val\n\t\.set\t__kcfi_typeid_func_struct_b_val, 0x00fd8361} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_c_val\n\t\.set\t__kcfi_typeid_func_struct_c_val, 0xad00d0bc} } } */

/* Verify struct by-value parameter types.  */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFv13test_struct_aE' typeid=0xe0fb126a} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFv13test_struct_bE' typeid=0x00fd8361} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFv13test_struct_cE' typeid=0xad00d0bc} kcfi0 } } */

/* { dg-final { scan-assembler-times {0xe0fb126a} 1 } } */
/* { dg-final { scan-assembler-times {0x00fd8361} 1 } } */
/* { dg-final { scan-assembler-times {0xad00d0bc} 1 } } */

/* Functions returning structs by value - must have different type IDs.  */
extern struct test_struct_a func_ret_struct_a_val(void); /* _ZTSF14test_struct_avE -> unique.  */
extern struct test_struct_b func_ret_struct_b_val(void); /* _ZTSF14test_struct_bvE -> unique.  */
extern struct test_struct_c func_ret_struct_c_val(void); /* _ZTSF14test_struct_cvE -> unique.  */

/* Struct return by-value tests with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_struct_a_val\n\t\.set\t__kcfi_typeid_func_ret_struct_a_val, 0x0405e05a} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_struct_b_val\n\t\.set\t__kcfi_typeid_func_ret_struct_b_val, 0x6c60f9bb} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_struct_c_val\n\t\.set\t__kcfi_typeid_func_ret_struct_c_val, 0xd8ef4934} } } */

/* Verify struct return by-value types - using correct P prefix for
   function pointer.  */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSF13test_struct_avE' typeid=0x0405e05a} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSF13test_struct_bvE' typeid=0x6c60f9bb} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSF13test_struct_cvE' typeid=0xd8ef4934} kcfi0 } } */

/* { dg-final { scan-assembler-times {0x0405e05a} 1 } } */
/* { dg-final { scan-assembler-times {0x6c60f9bb} 1 } } */
/* { dg-final { scan-assembler-times {0xd8ef4934} 1 } } */

/* Mixed struct parameters - order and type must matter.  */
extern void func_struct_a_b(struct test_struct_a *a, struct test_struct_b *b); /* unique.  */
extern void func_struct_b_a(struct test_struct_b *b, struct test_struct_a *a); /* different! */

/* Mixed struct parameter tests - MUST be different (parameter order matters).  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_a_b\n\t\.set\t__kcfi_typeid_func_struct_a_b, 0xf4af6e27} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_b_a\n\t\.set\t__kcfi_typeid_func_struct_b_a, 0x16bb1ad3} } } */

/* Verify mixed struct parameter types.  */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFvP13test_struct_aP13test_struct_bE' typeid=0xf4af6e27} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFvP13test_struct_bP13test_struct_aE' typeid=0x16bb1ad3} kcfi0 } } */

/* { dg-final { scan-assembler-times {0xf4af6e27} 1 } } */
/* { dg-final { scan-assembler-times {0x16bb1ad3} 1 } } */

/* Typedef structs - must be different from named structs.  */
typedef struct { int value; } typedef_struct_x;
typedef struct { int value; } typedef_struct_y;  /* Same layout but different typedef name.  */
extern void func_typedef_x_ptr(typedef_struct_x *x);  /* Must be unique.  */
extern void func_typedef_y_ptr(typedef_struct_y *x);  /* Must be different from typedef_struct_x.  */

/* Typedef struct tests - MUST be different from each other.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_typedef_x_ptr\n\t\.set\t__kcfi_typeid_func_typedef_x_ptr, 0x746f7969} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_typedef_y_ptr\n\t\.set\t__kcfi_typeid_func_typedef_y_ptr, 0xa071fd44} } } */

/* Verify typedef struct pointer types.  */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFvP16typedef_struct_xE' typeid=0x746f7969} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFvP16typedef_struct_yE' typeid=0xa071fd44} kcfi0 } } */

/* { dg-final { scan-assembler-times {0x746f7969} 1 } } */
/* { dg-final { scan-assembler-times {0xa071fd44} 1 } } */

/* Typedef vs open-coded function types - MUST have identical type IDs.  */
typedef void (*func_ptr_typedef)(int x, char y);
extern void func_with_typedef_param(func_ptr_typedef fp);               /* Should match open-coded.  */
extern void func_with_opencoded_param(void (*fp)(int x, char y));      /* Should match typedef.  */

/* Function parameter types - typedef and open-coded should generate SAME type ID */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_with_typedef_param\n\t\.set\t__kcfi_typeid_func_with_typedef_param, 0xdc5c6da9} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_with_opencoded_param\n\t\.set\t__kcfi_typeid_func_with_opencoded_param, 0xdc5c6da9} } } */

/* Verify function pointer parameter types.  */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFvPFvicEE' typeid=0xdc5c6da9} kcfi0 } } */

/* Verify exact count - each typedef/opencoded pair should generate exactly 2 symbols with identical values.  */
/* { dg-final { scan-assembler-times {0xdc5c6da9} 2 } } */

/* Typedef vs open-coded function types - MUST have identical type IDs.  */
typedef int (*ret_func_ptr_typedef)(void);
extern ret_func_ptr_typedef func_ret_typedef_param(void);              /* Should match open-coded.  */
extern int (*func_ret_opencoded_param(void))(void);                    /* Should match typedef.  */

/* Return function pointer types - typedef and open-coded should
   generate SAME type ID.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_typedef_param\n\t\.set\t__kcfi_typeid_func_ret_typedef_param, 0xdfeb316a} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_opencoded_param\n\t\.set\t__kcfi_typeid_func_ret_opencoded_param, 0xdfeb316a} } } */

/* Verify return function pointer types.  */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFPFivEvE' typeid=0xdfeb316a} kcfi0 } } */

/* Verify exact count - each typedef/opencoded pair should generate exactly
   2 symbols with identical values.  */
/* { dg-final { scan-assembler-times {0xdfeb316a} 2 } } */

/* Anonymous struct via typedef - should get typedef name as struct name.  */
typedef struct { int anon_member_1; } anon_typedef_1;
typedef struct { int anon_member_2; } anon_typedef_2;
extern void func_anon_typedef_1(anon_typedef_1 *param);                /* Should use typedef name.  */
extern void func_anon_typedef_2(anon_typedef_2 *param);                /* Should be different from anon_typedef_1 */

/* Anonymous typedef struct tests - MUST be different from each other.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_anon_typedef_1\n\t\.set\t__kcfi_typeid_func_anon_typedef_1, 0x55475a23} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_anon_typedef_2\n\t\.set\t__kcfi_typeid_func_anon_typedef_2, 0x454f8fb8} } } */

/* Verify anonymous typedef struct types.  */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFvP14anon_typedef_1E' typeid=0x55475a23} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFvP14anon_typedef_2E' typeid=0x454f8fb8} kcfi0 } } */

/* { dg-final { scan-assembler-times {0x55475a23} 1 } } */
/* { dg-final { scan-assembler-times {0x454f8fb8} 1 } } */

/* Local function definitions - these will NOT get __kcfi_typeid_ symbols (only external declarations do) */
void local_func_void(void) { }                  /* _ZTSFvvE -> 0x40e0d3c8 */
void local_func_short(short x) { }              /* _ZTSFvsE -> 0x84d472e1 */
void local_func_uint(unsigned int x) { }        /* _ZTSFvjE -> 0x60eb9384 */
void local_func_float(float x) { }              /* _ZTSFvfE -> 0x210943d8 */

/* Local function validation - verify local function definitions do NOT get
   __kcfi_typeid_ symbols.  */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_void\n} } } */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_short\n} } } */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_uint\n} } } */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_float\n} } } */

/* Local pointer parameter types.  */
void local_func_double_ptr(double *x) { }       /* _ZTSFvPdE -> 0x1ec0c7a8 */
void local_func_float_ptr(float *x) { }         /* _ZTSFvPfE -> 0xd2bbd2d6 */

/* Local pointer parameter types - should NOT emit symbols.  */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_double_ptr\n} } } */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_float_ptr\n} } } */

/* Local nested pointers.  */
void local_func_void_ptr_ptr(void **x) { }      /* _ZTSFvPPvE -> 0xa64349b0 */

/* Local nested pointers - should NOT emit symbols.  */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_void_ptr_ptr\n} } } */

/* Local mixed parameters.  */
void local_func_ptr_val(int *x, int y) { }      /* _ZTSFvPiiE -> 0xf072c2e8 */
void local_func_val_ptr(int x, int *y) { }      /* _ZTSFviPiE -> 0x0d1f87aa */

/* Local mixed parameter validation - should NOT emit symbols.  */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_ptr_val\n} } } */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_val_ptr\n} } } */

/* Local return types.  */
float local_func_return_float(void) { return 0.0f; }    /* _ZTSFfvE -> 0xee5e2118 */
double local_func_return_double(void) { return 0.0; }   /* _ZTSFdvE -> 0x59256b1e */

/* Local return type discrimination - should NOT emit symbols.  */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_return_float\n} } } */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_return_double\n} } } */

/* Verify local function mangle strings appear in KCFI dump (even though no symbols are emitted) */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFvsE' typeid=0x84d472e1} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFvfE' typeid=0x210943d8} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFvPdE' typeid=0x1ec0c7a8} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFvPfE' typeid=0xd2bbd2d6} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFvPPvE' typeid=0xa64349b0} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFvPiiE' typeid=0xf072c2e8} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFviPiE' typeid=0x0d1f87aa} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFfvE' typeid=0xee5e2118} kcfi0 } } */
/* { dg-final { scan-tree-dump {KCFI type ID: mangled='_ZTSFdvE' typeid=0x59256b1e} kcfi0 } } */

struct not_void {
	int nothing;
};

/* Function that takes addresses to make functions visible to KCFI */
void test_address_taken(struct not_void *arg)
{
    /* External functions - taking addresses generates __kcfi_typeid_ symbols.  */
    void (*p1)(void) = func_void;
    void (*p2)(char) = func_char;
    void (*p3)(int) = func_int;
    void (*p4)(long) = func_long;

    void (*p5)(int*) = func_int_ptr;
    void (*p6)(char*) = func_char_ptr;
    void (*p7)(void*) = func_void_ptr;

    void (*p_const_int_ptr)(const int*) = func_const_int_ptr;
    void (*p_const_char_ptr)(const char*) = func_const_char_ptr;
    void (*p_const_void_ptr)(const void*) = func_const_void_ptr;

    void (*p8)(int**) = func_int_ptr_ptr;
    void (*p9)(char**) = func_char_ptr_ptr;

    void (*p10)(int, char) = func_int_char;
    void (*p11)(char, int) = func_char_int;
    void (*p12)(int, int) = func_two_int;

    int (*p13)(void) = func_return_int;
    char (*p14)(void) = func_return_char;
    void* (*p15)(void) = func_return_ptr;

    /* Array parameters - should decay to pointers.  */
    void (*p16)(int*) = func_int_array;
    void (*p17)(char*) = func_char_array;

    /* Function pointer parameters.  */
    void (*p18)(void(*)(void)) = func_fptr_void;
    void (*p19)(void(*)(int)) = func_fptr_int;
    void (*p20)(int(*)(void)) = func_fptr_ret_int;

    /* Struct/union/enum function pointers.  */
    void (*p_struct_a_ptr)(struct test_struct_a*) = func_struct_a_ptr;
    void (*p_struct_b_ptr)(struct test_struct_b*) = func_struct_b_ptr;
    void (*p_struct_c_ptr)(struct test_struct_c*) = func_struct_c_ptr;

    /* Const struct function pointers.  */
    void (*p_const_struct_a_ptr)(const struct test_struct_a*) = func_const_struct_a_ptr;
    void (*p_const_struct_b_ptr)(const struct test_struct_b*) = func_const_struct_b_ptr;
    void (*p_const_struct_c_ptr)(const struct test_struct_c*) = func_const_struct_c_ptr;
    void (*p_union_a_ptr)(union test_union_a*) = func_union_a_ptr;
    void (*p_union_b_ptr)(union test_union_b*) = func_union_b_ptr;
    void (*p_enum_a_ptr)(enum test_enum_a*) = func_enum_a_ptr;
    void (*p_enum_b_ptr)(enum test_enum_b*) = func_enum_b_ptr;

    struct test_struct_a* (*p_ret_struct_a_ptr)(void) = func_ret_struct_a_ptr;
    struct test_struct_b* (*p_ret_struct_b_ptr)(void) = func_ret_struct_b_ptr;
    struct test_struct_c* (*p_ret_struct_c_ptr)(void) = func_ret_struct_c_ptr;

    void (*p_struct_a_val)(struct test_struct_a) = func_struct_a_val;
    void (*p_struct_b_val)(struct test_struct_b) = func_struct_b_val;
    void (*p_struct_c_val)(struct test_struct_c) = func_struct_c_val;

    struct test_struct_a (*p_ret_struct_a_val)(void) = func_ret_struct_a_val;
    struct test_struct_b (*p_ret_struct_b_val)(void) = func_ret_struct_b_val;
    struct test_struct_c (*p_ret_struct_c_val)(void) = func_ret_struct_c_val;

    void (*p_struct_a_b)(struct test_struct_a*, struct test_struct_b*) = func_struct_a_b;
    void (*p_struct_b_a)(struct test_struct_b*, struct test_struct_a*) = func_struct_b_a;

    void (*p_typedef_x_ptr)(typedef_struct_x*) = func_typedef_x_ptr;
    void (*p_typedef_y_ptr)(typedef_struct_y*) = func_typedef_y_ptr;

    /* Typedef vs open-coded function type assignments should generate
       identical type IDs.  */
    void (*p_with_typedef_param)(func_ptr_typedef) = func_with_typedef_param;
    void (*p_with_opencoded_param)(void (*)(int, char)) = func_with_opencoded_param;
    ret_func_ptr_typedef (*p_ret_typedef_param)(void) = func_ret_typedef_param;
    int (*(*p_ret_opencoded_param)(void))(void) = func_ret_opencoded_param;

    /* Anonymous struct typedef assignments - should generate unique type IDs.  */
    void (*p_anon_typedef_1)(anon_typedef_1 *) = func_anon_typedef_1;
    void (*p_anon_typedef_2)(anon_typedef_2 *) = func_anon_typedef_2;

    /* Union member access discrimination test.  */
    void (*p_tasklet_callback)(struct tasklet_like_struct *) = tasklet_callback_function;
    void (*p_tasklet_func)(unsigned long) = tasklet_func_function;

    /* Local functions - taking addresses does NOT generate __kcfi_typeid_
       symbols (only external declarations do).  */
    void (*p21)(void) = local_func_void;
    void (*p22)(short) = local_func_short;
    void (*p23)(unsigned int) = local_func_uint;
    void (*p24)(float) = local_func_float;

    void (*p25)(double*) = local_func_double_ptr;
    void (*p26)(float*) = local_func_float_ptr;

    void (*p27)(void**) = local_func_void_ptr_ptr;

    void (*p28)(int*, int) = local_func_ptr_val;
    void (*p29)(int, int*) = local_func_val_ptr;

    float (*p30)(void) = local_func_return_float;
    double (*p31)(void) = local_func_return_double;

    /* Use pointers to prevent optimization - external functions.  */
    if (p1) p1();
    if (p2) p2('x');
    if (p3) p3(42);
    if (p4) p4(42L);
    if (p5) p5((int*)0);
    if (p6) p6((char*)0);
    if (p7) p7((void*)0);

    /* Use const qualifier pointers to prevent optimization.  */
    if (p_const_int_ptr) p_const_int_ptr((const int*)0);
    if (p_const_char_ptr) p_const_char_ptr((const char*)0);
    if (p_const_void_ptr) p_const_void_ptr((const void*)0);
    if (p8) p8((int**)0);
    if (p9) p9((char**)0);
    if (p10) p10(1, 'x');
    if (p11) p11('x', 1);
    if (p12) p12(1, 2);
    if (p13) p13();
    if (p14) p14();
    if (p15) p15();
    if (p16) p16((int*)0);
    if (p17) p17((char*)0);
    if (p18) p18((void(*)(void))0);
    if (p19) p19((void(*)(int))0);
    if (p20) p20((int(*)(void))0);

    /* Use pointers to prevent optimization - local functions.  */
    if (p21) p21();
    if (p22) p22(1);
    if (p23) p23(1U);
    if (p24) p24(1.0f);
    if (p25) p25((double*)0);
    if (p26) p26((float*)0);
    if (p27) p27((void**)0);
    if (p28) p28((int*)0, 1);
    if (p29) p29(1, (int*)0);
    if (p30) p30();
    if (p31) p31();

    /* Use struct/union/enum function pointers to generate KCFI type IDs.  */
    if (p_struct_a_ptr) p_struct_a_ptr((struct test_struct_a*)0);
    if (p_struct_b_ptr) p_struct_b_ptr((struct test_struct_b*)0);
    if (p_struct_c_ptr) p_struct_c_ptr((struct test_struct_c*)0);
    if (p_const_struct_a_ptr) p_const_struct_a_ptr((const struct test_struct_a*)0);
    if (p_const_struct_b_ptr) p_const_struct_b_ptr((const struct test_struct_b*)0);
    if (p_const_struct_c_ptr) p_const_struct_c_ptr((const struct test_struct_c*)0);
    if (p_union_a_ptr) p_union_a_ptr((union test_union_a*)0);
    if (p_union_b_ptr) p_union_b_ptr((union test_union_b*)0);
    if (p_enum_a_ptr) p_enum_a_ptr((enum test_enum_a*)0);
    if (p_enum_b_ptr) p_enum_b_ptr((enum test_enum_b*)0);

    /* Use struct return type function pointers to generate type IDs.  */
    if (p_ret_struct_a_ptr) p_ret_struct_a_ptr();
    if (p_ret_struct_b_ptr) p_ret_struct_b_ptr();
    if (p_ret_struct_c_ptr) p_ret_struct_c_ptr();

    /* Use struct by-value parameter function pointers to generate type IDs.  */
    struct test_struct_a dummy_a = {};
    struct test_struct_b dummy_b = {};
    struct test_struct_c dummy_c = {};
    if (p_struct_a_val) p_struct_a_val(dummy_a);
    if (p_struct_b_val) p_struct_b_val(dummy_b);
    if (p_struct_c_val) p_struct_c_val(dummy_c);

    /* Use struct return by-value function pointers to generate type IDs.  */
    if (p_ret_struct_a_val) p_ret_struct_a_val();
    if (p_ret_struct_b_val) p_ret_struct_b_val();
    if (p_ret_struct_c_val) p_ret_struct_c_val();

    /* Use multi-parameter struct function pointers to generate type IDs.  */
    if (p_struct_a_b) p_struct_a_b((struct test_struct_a*)0, (struct test_struct_b*)0);
    if (p_struct_b_a) p_struct_b_a((struct test_struct_b*)0, (struct test_struct_a*)0);

    /* Use typedef struct function pointers to generate type IDs.  */
    if (p_typedef_x_ptr) p_typedef_x_ptr((typedef_struct_x*)0);
    if (p_typedef_y_ptr) p_typedef_y_ptr((typedef_struct_y*)0);

    /* Use typedef vs open-coded function pointers to generate type IDs.  */
    if (p_with_typedef_param) p_with_typedef_param((func_ptr_typedef)0);
    if (p_with_opencoded_param) p_with_opencoded_param((void (*)(int, char))0);
    if (p_ret_typedef_param) p_ret_typedef_param();
    if (p_ret_opencoded_param) p_ret_opencoded_param();

    /* Use anonymous typedef function pointers to generate type IDs.  */
    if (p_anon_typedef_1) p_anon_typedef_1((anon_typedef_1*)0);
    if (p_anon_typedef_2) p_anon_typedef_2((anon_typedef_2*)0);

    /* Use tasklet func function pointer to generate type ID. */
    if (p_tasklet_func) p_tasklet_func(0);

    struct tasklet_like_struct test_tasklet = { };
    test_tasklet.callback = tasklet_callback_function;

    /* This indirect call through union->callback MUST generate type ID
       0x84fa4a3e (callback signature). NOT type ID 0x80ee047b (func signature
       from first union member).  */
    struct tasklet_like_struct *volatile tasklet_ptr = &test_tasklet;
    if (tasklet_ptr->callback) {
        /* This call should match tasklet_callback_function type ID */
        tasklet_ptr->callback(tasklet_ptr);
    }
}

/* Named struct and its typedef should have IDENTICAL type IDs after canonicalization.  */
struct named_for_typedef_test { int member; };
typedef struct named_for_typedef_test named_for_typedef_test_t;

extern void func_named_struct_param(struct named_for_typedef_test *param);
extern void func_typedef_struct_param(named_for_typedef_test_t *param);

/* Named struct typedef canonicalization - MUST have identical type IDs.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_named_struct_param\n\t\.set\t__kcfi_typeid_func_named_struct_param, 0x9316d030} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_typedef_struct_param\n\t\.set\t__kcfi_typeid_func_typedef_struct_param, 0x9316d030} } } */

/* Verify named struct typedef canonicalization types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvP22named_for_typedef_testE' typeid=0x9316d030} kcfi0 } } */

/* Verify exact count - both should generate exactly 2 symbols with identical values.  */
/* { dg-final { scan-assembler-times {0x9316d030} 2 } } */

void test_named_struct_typedef_canonicalization(struct not_void *arg) {
    /* These should be compatible after canonicalization.  */
    void (*fp_struct)(struct named_for_typedef_test *) = func_named_struct_param;
    void (*fp_typedef)(struct named_for_typedef_test *) = func_typedef_struct_param;

    /* Take addresses to generate type IDs.  */
    if (fp_struct) fp_struct((struct named_for_typedef_test *)0);
    if (fp_typedef) fp_typedef((struct named_for_typedef_test *)0);
}

/* Basic type typedef canonicalization - typedef should canonicalize to
   underlying basic type.  */

/* Basic type typedefs commonly used in kernel code.  */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

/* Functions with basic type typedef vs original type parameters.  */
extern void func_u8_param(u8 param);
extern void func_unsigned_char_param(unsigned char param);
extern void func_u16_param(u16 param);
extern void func_unsigned_short_param(unsigned short param);
extern void func_u32_param(u32 param);
extern void func_unsigned_int_param(unsigned int param);

void test_basic_typedef_canonicalization(struct not_void *arg) {
    /* These should be compatible after canonicalization.  */
    void (*fp_u8)(unsigned char) = func_u8_param;                    /* Should work with canonicalization.  */
    void (*fp_uchar)(unsigned char) = func_unsigned_char_param;      /* Should work normally.  */
    void (*fp_u16)(unsigned short) = func_u16_param;                 /* Should work with canonicalization.  */
    void (*fp_ushort)(unsigned short) = func_unsigned_short_param;   /* Should work normally.  */
    void (*fp_u32)(unsigned int) = func_u32_param;                   /* Should work with canonicalization.  */
    void (*fp_uint)(unsigned int) = func_unsigned_int_param;         /* Should work normally.  */

    /* Take addresses to generate type IDs.  */
    if (fp_u8) fp_u8(0);
    if (fp_uchar) fp_uchar(0);
    if (fp_u16) fp_u16(0);
    if (fp_ushort) fp_ushort(0);
    if (fp_u32) fp_u32(0);
    if (fp_uint) fp_uint(0);
}

/* Basic type typedef canonicalization - MUST have identical type IDs
   after canonicalization.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u8_param\n\t\.set\t__kcfi_typeid_func_u8_param, 0x14e69eb2} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_unsigned_char_param\n\t\.set\t__kcfi_typeid_func_unsigned_char_param, 0x14e69eb2} } } */

/* Verify basic type canonicalization (u8/unsigned char) */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvhE' typeid=0x14e69eb2} kcfi0 } } */

/* Count test is below, which includes other tests that use this hash. */

/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u16_param\n\t\.set\t__kcfi_typeid_func_u16_param, 0x74dca876} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_unsigned_short_param\n\t\.set\t__kcfi_typeid_func_unsigned_short_param, 0x74dca876} } } */

/* Verify basic type canonicalization (u16/unsigned short) */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvtE' typeid=0x74dca876} kcfi0 } } */

/* { dg-final { scan-assembler-times {0x74dca876} 2 } } */

/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u32_param\n\t\.set\t__kcfi_typeid_func_u32_param, 0x60eb9384} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_unsigned_int_param\n\t\.set\t__kcfi_typeid_func_unsigned_int_param, 0x60eb9384} } } */

/* Verify basic type canonicalization (u32/unsigned int) */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvjE' typeid=0x60eb9384} kcfi0 } } */

/* Count test is below, which includes other tests that use this hash. */

/* Verify exact count - each typedef/basic type pair should generate exactly 2 symbols with identical values.  */
/* Note: Counts updated below to include recursive typedef tests.  */

/* Recursive typedef canonicalization - test multi-level typedef chains.  */

/* Kernel-style recursive typedef chains that need full canonicalization.  */
typedef unsigned char __u8_recursive;
typedef __u8_recursive u8_recursive;

typedef unsigned int __u32_recursive;
typedef __u32_recursive u32_recursive;

/* Three-level typedef chains.  */
typedef unsigned char base_u8_recursive_t;
typedef base_u8_recursive_t mid_u8_recursive_t;
typedef mid_u8_recursive_t top_u8_recursive_t;

/* Struct recursive typedef chains.  */
struct recursive_struct_test { int value; };
typedef struct recursive_struct_test base_recursive_struct_t;
typedef base_recursive_struct_t top_recursive_struct_t;

/* Functions with recursive typedefs - MUST have same type IDs as canonical forms.  */
extern void func_u8_recursive_chain(u8_recursive param);          /* u8_recursive -> __u8_recursive -> unsigned char.  */
extern void func_u8_recursive_mid(__u8_recursive param);          /* __u8_recursive -> unsigned char.  */
extern void func_u8_recursive_base(unsigned char param);          /* unsigned char (baseline) */

extern void func_u32_recursive_chain(u32_recursive param);        /* u32_recursive -> __u32_recursive -> unsigned int.  */
extern void func_u32_recursive_mid(__u32_recursive param);        /* __u32_recursive -> unsigned int.  */
extern void func_u32_recursive_base(unsigned int param);          /* unsigned int (baseline) */

extern void func_three_level_recursive(top_u8_recursive_t param); /* top -> mid -> base -> unsigned char.  */
extern void func_three_level_mid(mid_u8_recursive_t param);       /* mid -> base -> unsigned char.  */
extern void func_three_level_base(base_u8_recursive_t param);     /* base -> unsigned char.  */
extern void func_three_level_final(unsigned char param);          /* unsigned char (baseline) */

extern void func_struct_recursive_chain(top_recursive_struct_t *param);     /* Should resolve to struct name.  */
extern void func_struct_recursive_mid(base_recursive_struct_t *param);      /* Should resolve to struct name.  */
extern void func_struct_recursive_original(struct recursive_struct_test *param); /* struct name (baseline) */

void test_recursive_canonicalization(struct not_void *arg) {
    /* Recursive typedef function pointers - should be compatible after
       full canonicalization.  */
    void (*fp_u8_chain)(unsigned char) = func_u8_recursive_chain;        /* Should work after 2-level canonicalization.  */
    void (*fp_u8_mid)(unsigned char) = func_u8_recursive_mid;            /* Should work after 1-level canonicalization.  */
    void (*fp_u8_base)(unsigned char) = func_u8_recursive_base;          /* Should work normally.  */

    void (*fp_u32_chain)(unsigned int) = func_u32_recursive_chain;       /* Should work after 2-level canonicalization.  */
    void (*fp_u32_mid)(unsigned int) = func_u32_recursive_mid;           /* Should work after 1-level canonicalization.  */
    void (*fp_u32_base)(unsigned int) = func_u32_recursive_base;         /* Should work normally.  */

    void (*fp_three_chain)(unsigned char) = func_three_level_recursive;  /* Should work after 3-level canonicalization.  */
    void (*fp_three_mid)(unsigned char) = func_three_level_mid;          /* Should work after 2-level canonicalization.  */
    void (*fp_three_base)(unsigned char) = func_three_level_base;        /* Should work after 1-level canonicalization.  */
    void (*fp_three_final)(unsigned char) = func_three_level_final;      /* Should work normally.  */

    void (*fp_struct_chain)(struct recursive_struct_test *) = func_struct_recursive_chain;  /* Should work after canonicalization.  */
    void (*fp_struct_mid)(struct recursive_struct_test *) = func_struct_recursive_mid;      /* Should work after canonicalization.  */
    void (*fp_struct_orig)(struct recursive_struct_test *) = func_struct_recursive_original; /* Should work normally.  */

    /* Use function pointers to prevent optimization.  */
    if (fp_u8_chain) fp_u8_chain(0);
    if (fp_u8_mid) fp_u8_mid(0);
    if (fp_u8_base) fp_u8_base(0);
    if (fp_u32_chain) fp_u32_chain(0);
    if (fp_u32_mid) fp_u32_mid(0);
    if (fp_u32_base) fp_u32_base(0);
    if (fp_three_chain) fp_three_chain(0);
    if (fp_three_mid) fp_three_mid(0);
    if (fp_three_base) fp_three_base(0);
    if (fp_three_final) fp_three_final(0);
    if (fp_struct_chain) fp_struct_chain((struct recursive_struct_test *)0);
    if (fp_struct_mid) fp_struct_mid((struct recursive_struct_test *)0);
    if (fp_struct_orig) fp_struct_orig((struct recursive_struct_test *)0);
}

/* Recursive typedef canonicalization validation - MUST have identical type
   IDs after full canonicalization.  */

/* u8 recursive chain - all should resolve to unsigned char.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u8_recursive_chain\n\t\.set\t__kcfi_typeid_func_u8_recursive_chain, 0x14e69eb2} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u8_recursive_mid\n\t\.set\t__kcfi_typeid_func_u8_recursive_mid, 0x14e69eb2} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u8_recursive_base\n\t\.set\t__kcfi_typeid_func_u8_recursive_base, 0x14e69eb2} } } */

/* u32 recursive chain - all should resolve to unsigned int.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u32_recursive_chain\n\t\.set\t__kcfi_typeid_func_u32_recursive_chain, 0x60eb9384} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u32_recursive_mid\n\t\.set\t__kcfi_typeid_func_u32_recursive_mid, 0x60eb9384} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u32_recursive_base\n\t\.set\t__kcfi_typeid_func_u32_recursive_base, 0x60eb9384} } } */

/* Three-level u8 recursive chain - all should resolve to unsigned char.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_three_level_recursive\n\t\.set\t__kcfi_typeid_func_three_level_recursive, 0x14e69eb2} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_three_level_mid\n\t\.set\t__kcfi_typeid_func_three_level_mid, 0x14e69eb2} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_three_level_base\n\t\.set\t__kcfi_typeid_func_three_level_base, 0x14e69eb2} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_three_level_final\n\t\.set\t__kcfi_typeid_func_three_level_final, 0x14e69eb2} } } */

/* Struct recursive chain - all should resolve to same struct name.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_recursive_chain\n\t\.set\t__kcfi_typeid_func_struct_recursive_chain, 0xf63dce36} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_recursive_mid\n\t\.set\t__kcfi_typeid_func_struct_recursive_mid, 0xf63dce36} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_recursive_original\n\t\.set\t__kcfi_typeid_func_struct_recursive_original, 0xf63dce36} } } */

/* Update counts to include recursive typedef tests.  */
/* Note: u8/unsigned char recursive tests add 7 more occurrences (actual count: 9) */
/* { dg-final { scan-assembler-times {0x14e69eb2} 9 } } */

/* Note: u32/unsigned int recursive tests add 3 more occurrences (actual count: 6) */
/* { dg-final { scan-assembler-times {0x60eb9384} 6 } } */

/* Verify struct recursive typedef canonicalization types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFvP21recursive_struct_testE' typeid=0xf63dce36} kcfi0 } } */

/* Struct recursive: 3 identical type IDs.  */
/* { dg-final { scan-assembler-times {0xf63dce36} 3 } } */

/* VLA (Variable Length Array) mangling tests.  */

/* Basic VLA cases - all should decay to simple pointer types.  */
extern void func_vla_1d(int n, int arr[n]);           /* _ZTSFviPiE -> 0x0d1f87aa */
extern void func_vla_empty(int n, int arr[]);         /* _ZTSFviPiE -> 0x0d1f87aa */
extern void func_vla_ptr(int n, int *arr);            /* _ZTSFviPiE -> 0x0d1f87aa */

/* VLA 1D tests with precise patterns - all should be identical.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_vla_1d\n\t\.set\t__kcfi_typeid_func_vla_1d, 0x0d1f87aa} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_vla_empty\n\t\.set\t__kcfi_typeid_func_vla_empty, 0x0d1f87aa} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_vla_ptr\n\t\.set\t__kcfi_typeid_func_vla_ptr, 0x0d1f87aa} } } */

/* Verify VLA 1D types.  */
/* { dg-final { scan-tree-dump {mangled='_ZTSFviPiE' typeid=0x0d1f87aa} kcfi0 } } */

/* Count verification - VLA 1D types should appear exactly 3 times in assembly.  */
/* { dg-final { scan-assembler-times {0x0d1f87aa} 4 } } +1 from local function preamble.  */

/* 2D arrays with known dimension - VLA in first dimension, fixed in second.  */
extern void func_vla_2d_first(int n, int arr[n][10]);      /* _ZTSFviPA10_iE -> 0x2cd9653d */
extern void func_vla_2d_empty(int n, int arr[][10]);       /* _ZTSFviPA10_iE -> 0x2cd9653d */
extern void func_vla_2d_ptr(int n, int (*arr)[10]);        /* _ZTSFviPA10_iE -> 0x2cd9653d */

/* 2D VLA with fixed dimension tests with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_vla_2d_first\n\t\.set\t__kcfi_typeid_func_vla_2d_first, 0x2cd9653d} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_vla_2d_empty\n\t\.set\t__kcfi_typeid_func_vla_2d_empty, 0x2cd9653d} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_vla_2d_ptr\n\t\.set\t__kcfi_typeid_func_vla_2d_ptr, 0x2cd9653d} } } */

/* Verify 2D VLA with fixed dimension types.  */
/* { dg-final { scan-ipa-dump {mangled='_ZTSFviPA10_iE' typeid=0x2cd9653d} ipa_kcfi } } */

/* Count verification - 2D VLA with fixed dimension should appear exactly 3 times.  */
/* { dg-final { scan-assembler-times {0x2cd9653d} 3 } } */

/* 2D VLA cases - both dimensions variable (Itanium ABI: variable dimension = empty) */
extern void func_vla_2d_both(int rows, int cols, int arr[rows][cols]); /* _ZTSFviiPA_iE -> 0xc63cc57b */
extern void func_vla_2d_second(int rows, int cols, int arr[][cols]);   /* _ZTSFviiPA_iE -> 0xc63cc57b */
extern void func_vla_2d_star(int rows, int cols, int arr[*][cols]);    /* _ZTSFviiPA_iE -> 0xc63cc57b */

/* 2D VLA with both dimensions variable tests with precise patterns.  */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_vla_2d_both\n\t\.set\t__kcfi_typeid_func_vla_2d_both, 0xc63cc57b} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_vla_2d_second\n\t\.set\t__kcfi_typeid_func_vla_2d_second, 0xc63cc57b} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_vla_2d_star\n\t\.set\t__kcfi_typeid_func_vla_2d_star, 0xc63cc57b} } } */

/* Verify 2D VLA with both dimensions variable types.  */
/* { dg-final { scan-ipa-dump {mangled='_ZTSFviiPA_iE' typeid=0xc63cc57b} ipa_kcfi } } */

/* Count verification - 2D VLA with both variable dimensions should appear exactly 3 times in assembly.  */
/* { dg-final { scan-assembler-times {0xc63cc57b} 3 } } */

/* VLA test function to force mangling.  */
void test_vla_mangling_verification(void) {
    void (*fp_vla_1d)(int, int*) = func_vla_1d;
    void (*fp_vla_empty)(int, int*) = func_vla_empty;
    void (*fp_vla_ptr)(int, int*) = func_vla_ptr;
    void (*fp_vla_2d_first)(int, int(*)[10]) = func_vla_2d_first;
    void (*fp_vla_2d_empty)(int, int(*)[10]) = func_vla_2d_empty;
    void (*fp_vla_2d_ptr)(int, int(*)[10]) = func_vla_2d_ptr;

    /* 2D VLA functions - take addresses to generate __kcfi_typeid_ symbols.  */
    volatile void *vla_p1 = func_vla_2d_both;
    volatile void *vla_p2 = func_vla_2d_second;
    volatile void *vla_p3 = func_vla_2d_star;
    (void)vla_p1; (void)vla_p2; (void)vla_p3;

    /* Variadic functions - take addresses and call through typed pointers to generate __kcfi_typeid_ symbols.  */
    void (*fp_variadic_simple)(const char *, ...) = func_variadic_simple;
    void (*fp_variadic_mixed)(int, const char *, ...) = func_variadic_mixed;
    void (*fp_variadic_multi)(int, char, const char *, ...) = func_variadic_multi;
    void (*fp_audit_pattern)(struct audit_context *, unsigned int, int, const char *, ...) = audit_log_pattern;
    void (*fp_non_variadic_simple)(const char *) = func_non_variadic_simple;
    void (*fp_non_variadic_mixed)(int, const char *) = func_non_variadic_mixed;

    /* Call through function pointers to trigger KCFI analysis.  */
    if (fp_variadic_simple) fp_variadic_simple("test");
    if (fp_variadic_mixed) fp_variadic_mixed(1, "test");
    if (fp_variadic_multi) fp_variadic_multi(1, 'x', "test");
    if (fp_audit_pattern) fp_audit_pattern((struct audit_context *)0, 0, 1, "test");
    if (fp_non_variadic_simple) fp_non_variadic_simple("test");
    if (fp_non_variadic_mixed) fp_non_variadic_mixed(1, "test");

    /* va_start regression test - ensures builtin functions are skipped in KCFI processing.  */
    test_va_start_regression(0.0f, "format", 42, 'x', "string");

    /* Keep volatile assignments for backward compatibility.  */
    volatile void *variadic_p1 = func_variadic_simple;
    volatile void *variadic_p2 = func_variadic_mixed;
    volatile void *variadic_p3 = func_variadic_multi;
    volatile void *audit_pattern_p = audit_log_pattern;
    volatile void *non_variadic_p1 = func_non_variadic_simple;
    volatile void *non_variadic_p2 = func_non_variadic_mixed;
    (void)variadic_p1; (void)variadic_p2; (void)variadic_p3;
    (void)audit_pattern_p;
    (void)non_variadic_p1; (void)non_variadic_p2;
}

/* va_start regression test implementation - triggers __builtin_va_start usage.  */
void test_va_start_regression(float dummy, const char *fmt, ...) {
    va_list args;
    /* This previously caused crash due to __builtin_va_start processing.  */
    va_start(args, fmt);
    /* Simple va_list usage to ensure the builtin call is generated.  */
    (void)args;
    va_end(args);
}

/* Library builtin test - __builtin_memset resolves to memset and should
   get KCFI type ID.  */

/* memset signature: void *memset(void *s, int c, size_t n)
   - 64-bit targets: size_t is 'unsigned long' (m) -> _ZTSFPvPvimE -> 0x1d8c7ada
   - 32-bit ARM: size_t is 'unsigned int' (j) -> _ZTSFPvPvijE -> 0xdd98e20d */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_memset\n\t\.set\t__kcfi_typeid_memset, 0x1d8c7ada} { target { ! arm*-*-* } } } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_memset\n\t\.set\t__kcfi_typeid_memset, 0xdd98e20d} { target arm32 } } } */
/* { dg-final { scan-ipa-dump {mangled='_ZTSFPvPvimE' typeid=0x1d8c7ada} ipa_kcfi { target { ! arm*-*-* } } } } */
/* { dg-final { scan-ipa-dump {mangled='_ZTSFPvPvijE' typeid=0xdd98e20d} ipa_kcfi { target arm32 } } } */
/* { dg-final { scan-assembler-times {0x1d8c7ada} 1 { target { ! arm*-*-* } } } } */
/* { dg-final { scan-assembler-times {0xdd98e20d} 1 { target arm32 } } } */

void test_builtin_memset_indirect(void) {
    char buffer[64];
    /* Force indirect call through function pointer to test KCFI validation.
       __builtin_memset resolves to regular memset which should get a type ID. */
    void *(*memset_ptr)(void *, int, __SIZE_TYPE__) = __builtin_memset;
    volatile void *result = memset_ptr(buffer, 0, sizeof(buffer));
    (void)result;  /* Prevent optimization.  */
}
