/* Test KCFI type ID hashing - verify different signatures generate different __kcfi_typeid_ symbols */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi" } */

/* Test __kcfi_typeid_ symbol generation for address-taken functions.
   Verify precise type discrimination using Itanium C++ ABI mangling. */

/* External function declarations - these will get __kcfi_typeid_ symbols when address-taken */
extern void func_void(void);                    /* FvvE -> 0x126cd6c8 */
extern void func_char(char x);                  /* FvcE -> 0xb2b4b50d */
extern void func_int(int x);                    /* FviE -> 0xae9b16db */
extern void func_long(long x);                  /* FvlE -> 0xc2947a92 */

/* Basic types - verify exact type IDs match with precise patterns */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_void\n\t\.set\t__kcfi_typeid_func_void, 0x126cd6c8} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_char\n\t\.set\t__kcfi_typeid_func_char, 0xb2b4b50d} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_int\n\t\.set\t__kcfi_typeid_func_int, 0xae9b16db} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_long\n\t\.set\t__kcfi_typeid_func_long, 0xc2947a92} } } */

/* Count verification - basic types (void type used by multiple functions) */
/* { dg-final { scan-assembler-times {0x126cd6c8} 2 } } +1 from local_func_void preamble below */
/* { dg-final { scan-assembler-times {0xb2b4b50d} 1 } } */
/* { dg-final { scan-assembler-times {0xae9b16db} 1 } } */
/* { dg-final { scan-assembler-times {0xc2947a92} 1 } } */

/* Pointer parameter types - must all differ */
extern void func_int_ptr(int *x);               /* FvPiE -> 0x24c991ed */
extern void func_char_ptr(char *x);             /* FvPcE -> 0x20aff3bb */
extern void func_void_ptr(void *x);             /* FvPvE -> 0xd481b0e2 */

/* Pointer types - verify they all differ with precise patterns */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_int_ptr\n\t\.set\t__kcfi_typeid_func_int_ptr, 0x24c991ed} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_char_ptr\n\t\.set\t__kcfi_typeid_func_char_ptr, 0x20aff3bb} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_void_ptr\n\t\.set\t__kcfi_typeid_func_void_ptr, 0xd481b0e2} } } */

/* Count verification - pointer types (will appear twice due to array decay earlier) */
/* { dg-final { scan-assembler-times {0x24c991ed} 2 } } */
/* { dg-final { scan-assembler-times {0x20aff3bb} 2 } } */
/* { dg-final { scan-assembler-times {0xd481b0e2} 1 } } */

/* Const qualifier discrimination - const vs non-const must have different type IDs */
extern void func_const_int_ptr(const int *x);   /* FvPKiE -> const int* (must differ from int*) */
extern void func_const_char_ptr(const char *x); /* FvPKcE -> const char* (must differ from char*) */
extern void func_const_void_ptr(const void *x); /* FvPKvE -> const void* (must differ from void*) */

/* Const qualifier types - verify const vs non-const have different type IDs */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_const_int_ptr\n\t\.set\t__kcfi_typeid_func_const_int_ptr, 0x46d77fc6} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_const_char_ptr\n\t\.set\t__kcfi_typeid_func_const_char_ptr, 0x62c8a150} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_const_void_ptr\n\t\.set\t__kcfi_typeid_func_const_void_ptr, 0x469499d9} } } */

/* Count verification - const qualifier types should appear exactly once */
/* { dg-final { scan-assembler-times {0x46d77fc6} 1 } } */
/* { dg-final { scan-assembler-times {0x62c8a150} 1 } } */
/* { dg-final { scan-assembler-times {0x469499d9} 1 } } */

/* Nested pointer types */
extern void func_int_ptr_ptr(int **x);          /* FvPPiE -> 0x53e50f73 */
extern void func_char_ptr_ptr(char **x);        /* FvPPcE -> 0x77d63d95 */

/* Nested pointers with precise patterns */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_int_ptr_ptr\n\t\.set\t__kcfi_typeid_func_int_ptr_ptr, 0x53e50f73} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_char_ptr_ptr\n\t\.set\t__kcfi_typeid_func_char_ptr_ptr, 0x77d63d95} } } */

/* Count verification - nested pointer types should appear exactly once */
/* { dg-final { scan-assembler-times {0x53e50f73} 1 } } */
/* { dg-final { scan-assembler-times {0x77d63d95} 1 } } */

/* Multiple parameter types - order matters */
extern void func_int_char(int x, char y);       /* FvicE -> 0x98838338 */
extern void func_char_int(char x, int y);       /* FvciE -> 0x9aaafd24 */
extern void func_two_int(int x, int y);         /* FviiE -> 0x9c69f19e */

/* Multiple parameter tests with precise patterns */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_int_char\n\t\.set\t__kcfi_typeid_func_int_char, 0x98838338} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_char_int\n\t\.set\t__kcfi_typeid_func_char_int, 0x9aaafd24} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_two_int\n\t\.set\t__kcfi_typeid_func_two_int, 0x9c69f19e} } } */

/* Count verification - multiple parameter types should appear exactly once */
/* { dg-final { scan-assembler-times {0x98838338} 1 } } */
/* { dg-final { scan-assembler-times {0x9aaafd24} 1 } } */
/* { dg-final { scan-assembler-times {0x9c69f19e} 1 } } */

/* Return types */
extern int func_return_int(void);               /* FivE -> 0x426f60ef */
extern char func_return_char(void);             /* FcvE -> 0x3688e5f1 */
extern void* func_return_ptr(void);             /* FPvvE -> 0x924cfbe6 */

/* Return type tests with precise patterns */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_return_int\n\t\.set\t__kcfi_typeid_func_return_int, 0x426f60ef} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_return_char\n\t\.set\t__kcfi_typeid_func_return_char, 0x3688e5f1} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_return_ptr\n\t\.set\t__kcfi_typeid_func_return_ptr, 0x924cfbe6} } } */

/* Count verification - return types should appear exactly once */
/* { dg-final { scan-assembler-times {0x426f60ef} 1 } } */
/* { dg-final { scan-assembler-times {0x3688e5f1} 1 } } */
/* { dg-final { scan-assembler-times {0x924cfbe6} 1 } } */

/* Array parameters - decay to pointers */
extern void func_int_array(int arr[]);          /* FvPiE -> 0x24c991ed (same as int*) */
extern void func_char_array(char arr[]);        /* FvPcE -> 0x20aff3bb (same as char*) */

/* Array decay validation - arrays should have SAME type ID as corresponding pointers */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_int_array\n\t\.set\t__kcfi_typeid_func_int_array, 0x24c991ed} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_char_array\n\t\.set\t__kcfi_typeid_func_char_array, 0x20aff3bb} } } */
/* Counted below. */

/* Function pointer parameters */
extern void func_fptr_void(void (*fp)(void));   /* FvPFvvEE -> 0xbe908da1 */
extern void func_fptr_int(void (*fp)(int));     /* FvPFviEE -> 0xe9a18c50 */
extern void func_fptr_ret_int(int (*fp)(void)); /* FvPFivEE -> 0xdcab0e2c */

/* Function pointer parameter tests with precise patterns */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_fptr_void\n\t\.set\t__kcfi_typeid_func_fptr_void, 0xbe908da1} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_fptr_int\n\t\.set\t__kcfi_typeid_func_fptr_int, 0xe9a18c50} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_fptr_ret_int\n\t\.set\t__kcfi_typeid_func_fptr_ret_int, 0xdcab0e2c} } } */

/* Count verification - function pointer parameter types should appear exactly once */
/* { dg-final { scan-assembler-times {0xbe908da1} 1 } } */
/* { dg-final { scan-assembler-times {0xe9a18c50} 1 } } */
/* { dg-final { scan-assembler-times {0xdcab0e2c} 1 } } */

/* Struct/union/enum parameter types: each struct name must produce different type IDs */
struct test_struct_a { int x; };
struct test_struct_b { int y; };
struct test_struct_c { int z; };
union test_union_a { int i; float f; };
union test_union_b { int j; float g; };
enum test_enum_a { ENUM_A_VAL1, ENUM_A_VAL2 };
enum test_enum_b { ENUM_B_VAL1, ENUM_B_VAL2 };

/* Functions taking struct pointers - must have different type IDs */
extern void func_struct_a_ptr(struct test_struct_a *x);  /* Fv14test_struct_aPiE -> unique */
extern void func_struct_b_ptr(struct test_struct_b *x);  /* Fv14test_struct_bPiE -> unique */
extern void func_struct_c_ptr(struct test_struct_c *x);  /* Fv14test_struct_cPiE -> unique */

/* Struct pointer tests with precise patterns */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_a_ptr\n\t\.set\t__kcfi_typeid_func_struct_a_ptr, 0x71566b2c} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_b_ptr\n\t\.set\t__kcfi_typeid_func_struct_b_ptr, 0x814fc897} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_c_ptr\n\t\.set\t__kcfi_typeid_func_struct_c_ptr, 0xa5523fda} } } */

/* Count verification - struct pointer types should appear exactly once */
/* { dg-final { scan-assembler-times {0x71566b2c} 1 } } */
/* { dg-final { scan-assembler-times {0x814fc897} 1 } } */
/* { dg-final { scan-assembler-times {0xa5523fda} 1 } } */

/* Functions taking const struct pointers - must differ from non-const versions */
extern void func_const_struct_a_ptr(const struct test_struct_a *x);  /* FvPK14test_struct_aE -> unique, different from non-const */
extern void func_const_struct_b_ptr(const struct test_struct_b *x);  /* FvPK14test_struct_bE -> unique, different from non-const */
extern void func_const_struct_c_ptr(const struct test_struct_c *x);  /* FvPK14test_struct_cE -> unique, different from non-const */

/* Const struct pointer tests with precise patterns - must differ from non-const */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_const_struct_a_ptr\n\t\.set\t__kcfi_typeid_func_const_struct_a_ptr, 0x870e939b} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_const_struct_b_ptr\n\t\.set\t__kcfi_typeid_func_const_struct_b_ptr, 0x77153630} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_const_struct_c_ptr\n\t\.set\t__kcfi_typeid_func_const_struct_c_ptr, 0x5b12cb85} } } */

/* Count verification - const struct pointer types should appear exactly once */
/* { dg-final { scan-assembler-times {0x870e939b} 1 } } */
/* { dg-final { scan-assembler-times {0x77153630} 1 } } */
/* { dg-final { scan-assembler-times {0x5b12cb85} 1 } } */

extern void func_union_a_ptr(union test_union_a *x);     /* Fv13test_union_aPiE -> unique */
extern void func_union_b_ptr(union test_union_b *x);     /* Fv13test_union_bPiE -> unique */
extern void func_enum_a_ptr(enum test_enum_a *x);        /* Fv11test_enum_aPiE -> unique */
extern void func_enum_b_ptr(enum test_enum_b *x);        /* Fv11test_enum_bPiE -> unique */

/* Union member access discrimination test - prevents regression of union member bug */
struct tasklet_like_struct {
    int state;
    union {
	/* First union member - should NOT be used for callback calls */
        void (*func)(unsigned long data);
	/* Second union member - should be used for callback calls */
        void (*callback)(struct tasklet_like_struct *t);
    };
    unsigned long data;
};

/* Function with callback signature - this should match when accessed via union->callback */
extern void tasklet_callback_function(struct tasklet_like_struct *t);  /* FvP19tasklet_like_structE -> unique */

/* Function with func signature - this should NOT match callback calls */
extern void tasklet_func_function(unsigned long data);                  /* FvmE -> different from callback */

/* Union member access discrimination tests - MUST use correct union member type */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_tasklet_callback_function\n\t\.set\t__kcfi_typeid_tasklet_callback_function, 0x5c500b1a} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_tasklet_func_function\n\t\.set\t__kcfi_typeid_tasklet_func_function, 0x9e92034f} } } */

/* Union pointer tests with precise patterns */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_union_a_ptr\n\t\.set\t__kcfi_typeid_func_union_a_ptr, 0x01decd2b} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_union_b_ptr\n\t\.set\t__kcfi_typeid_func_union_b_ptr, 0xf1e702c0} } } */

/* Count verification - union pointer types should appear exactly once */
/* { dg-final { scan-assembler-times {0x01decd2b} 1 } } */
/* { dg-final { scan-assembler-times {0xf1e702c0} 1 } } */

/* Enum pointer tests with precise patterns */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_enum_a_ptr\n\t\.set\t__kcfi_typeid_func_enum_a_ptr, 0x8a8b58b6} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_enum_b_ptr\n\t\.set\t__kcfi_typeid_func_enum_b_ptr, 0x3a8eac5d} } } */

/* Count verification - enum pointer types should appear exactly once */
/* { dg-final { scan-assembler-times {0x8a8b58b6} 1 } } */
/* { dg-final { scan-assembler-times {0x3a8eac5d} 1 } } */

/* Count verification - union member discrimination types should appear exactly once */
/* The key test is that callback and func functions have DIFFERENT type IDs, proving union member discrimination works */
/* { dg-final { scan-assembler-times {0x5c500b1a} 1 } } */
/* { dg-final { scan-assembler-times {0x9e92034f} 1 } } */

/* Indirect call through t->callback union must use correct callback type ID (0x5c500b1a).
   The decimal value -1548749594 is the two's complement of 0x5c500b1a used in KCFI checks.  */
/* { dg-final { scan-assembler-times {\tmovl\t\$-1548749594, %r10d} 1 { target x86_64-*-* } } } */
/* { dg-final { scan-assembler-times {\tmov\tw17, #2842\n\tmovk\tw17, #23632, lsl #16} 1 { target aarch64-*-* } } } */

/* Functions returning struct pointers - must have different type IDs */
extern struct test_struct_a* func_ret_struct_a_ptr(void); /* F14test_struct_aPvE -> unique */
extern struct test_struct_b* func_ret_struct_b_ptr(void); /* F14test_struct_bPvE -> unique */
extern struct test_struct_c* func_ret_struct_c_ptr(void); /* F14test_struct_cPvE -> unique */

/* Struct return pointer tests with precise patterns */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_struct_a_ptr\n\t\.set\t__kcfi_typeid_func_ret_struct_a_ptr, 0xd87462f8} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_struct_b_ptr\n\t\.set\t__kcfi_typeid_func_ret_struct_b_ptr, 0xe86dc063} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_struct_c_ptr\n\t\.set\t__kcfi_typeid_func_ret_struct_c_ptr, 0x8c6f6e26} } } */

/* { dg-final { scan-assembler-times {0xd87462f8} 1 } } */
/* { dg-final { scan-assembler-times {0xe86dc063} 1 } } */
/* { dg-final { scan-assembler-times {0x8c6f6e26} 1 } } */

/* Functions taking structs by value - must have different type IDs */
extern void func_struct_a_val(struct test_struct_a x);   /* Fv14test_struct_aE -> unique */
extern void func_struct_b_val(struct test_struct_b x);   /* Fv14test_struct_bE -> unique */
extern void func_struct_c_val(struct test_struct_c x);   /* Fv14test_struct_cE -> unique */

/* Struct by-value parameter tests with precise patterns */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_a_val\n\t\.set\t__kcfi_typeid_func_struct_a_val, 0x065348c6} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_b_val\n\t\.set\t__kcfi_typeid_func_struct_b_val, 0x3655d2ed} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_c_val\n\t\.set\t__kcfi_typeid_func_struct_c_val, 0xd2577418} } } */

/* { dg-final { scan-assembler-times {0x065348c6} 1 } } */
/* { dg-final { scan-assembler-times {0x3655d2ed} 1 } } */
/* { dg-final { scan-assembler-times {0xd2577418} 1 } } */

/* Functions returning structs by value - must have different type IDs */
extern struct test_struct_a func_ret_struct_a_val(void); /* F14test_struct_avE -> unique */
extern struct test_struct_b func_ret_struct_b_val(void); /* F14test_struct_bvE -> unique */
extern struct test_struct_c func_ret_struct_c_val(void); /* F14test_struct_cvE -> unique */

/* Struct return by-value tests with precise patterns */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_struct_a_val\n\t\.set\t__kcfi_typeid_func_ret_struct_a_val, 0x19c15d6a} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_struct_b_val\n\t\.set\t__kcfi_typeid_func_ret_struct_b_val, 0x39c3ce61} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_struct_c_val\n\t\.set\t__kcfi_typeid_func_ret_struct_c_val, 0xe5c71bbc} } } */

/* { dg-final { scan-assembler-times {0x19c15d6a} 1 } } */
/* { dg-final { scan-assembler-times {0x39c3ce61} 1 } } */
/* { dg-final { scan-assembler-times {0xe5c71bbc} 1 } } */

/* Mixed struct parameters - order and type must matter */
extern void func_struct_a_b(struct test_struct_a *a, struct test_struct_b *b); /* unique */
extern void func_struct_b_a(struct test_struct_b *b, struct test_struct_a *a); /* different! */

/* Mixed struct parameter tests - MUST be different (parameter order matters) */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_a_b\n\t\.set\t__kcfi_typeid_func_struct_a_b, 0x807a109b} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_b_a\n\t\.set\t__kcfi_typeid_func_struct_b_a, 0xdcb4f037} } } */

/* { dg-final { scan-assembler-times {0x807a109b} 1 } } */
/* { dg-final { scan-assembler-times {0xdcb4f037} 1 } } */

/* Typedef structs - must be different from named structs */
typedef struct { int value; } typedef_struct_x;
typedef struct { int value; } typedef_struct_y;  /* Same layout but different typedef name */
extern void func_typedef_x_ptr(typedef_struct_x *x);  /* Must be unique */
extern void func_typedef_y_ptr(typedef_struct_y *x);  /* Must be different from typedef_struct_x */

/* Typedef struct tests - MUST be different from each other */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_typedef_x_ptr\n\t\.set\t__kcfi_typeid_func_typedef_x_ptr, 0x5ed7484d} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_typedef_y_ptr\n\t\.set\t__kcfi_typeid_func_typedef_y_ptr, 0x7ad9b2f8} } } */

/* { dg-final { scan-assembler-times {0x5ed7484d} 1 } } */
/* { dg-final { scan-assembler-times {0x7ad9b2f8} 1 } } */

/* Typedef vs open-coded function types - MUST have identical type IDs */
typedef void (*func_ptr_typedef)(int x, char y);
extern void func_with_typedef_param(func_ptr_typedef fp);               /* Should match open-coded */
extern void func_with_opencoded_param(void (*fp)(int x, char y));      /* Should match typedef */

/* Function parameter types - typedef and open-coded should generate SAME type ID */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_with_typedef_param\n\t\.set\t__kcfi_typeid_func_with_typedef_param, 0xf1d25775} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_with_opencoded_param\n\t\.set\t__kcfi_typeid_func_with_opencoded_param, 0xf1d25775} } } */

/* Verify exact count - each typedef/opencoded pair should generate exactly 2 symbols with identical values */
/* { dg-final { scan-assembler-times {0xf1d25775} 2 } } */

/* Typedef vs open-coded function types - MUST have identical type IDs */
typedef int (*ret_func_ptr_typedef)(void);
extern ret_func_ptr_typedef func_ret_typedef_param(void);              /* Should match open-coded */
extern int (*func_ret_opencoded_param(void))(void);                    /* Should match typedef */

/* Return function pointer types - typedef and open-coded should generate SAME type ID */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_typedef_param\n\t\.set\t__kcfi_typeid_func_ret_typedef_param, 0x616a0478} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_ret_opencoded_param\n\t\.set\t__kcfi_typeid_func_ret_opencoded_param, 0x616a0478} } } */

/* Verify exact count - each typedef/opencoded pair should generate exactly 2 symbols with identical values */
/* { dg-final { scan-assembler-times {0x616a0478} 2 } } */

/* Anonymous struct via typedef - should get typedef name as struct name */
typedef struct { int anon_member_1; } anon_typedef_1;
typedef struct { int anon_member_2; } anon_typedef_2;
extern void func_anon_typedef_1(anon_typedef_1 *param);                /* Should use typedef name */
extern void func_anon_typedef_2(anon_typedef_2 *param);                /* Should be different from anon_typedef_1 */

/* Anonymous typedef struct tests - MUST be different from each other */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_anon_typedef_1\n\t\.set\t__kcfi_typeid_func_anon_typedef_1, 0x3b098f27} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_anon_typedef_2\n\t\.set\t__kcfi_typeid_func_anon_typedef_2, 0x2b11c4bc} } } */

/* { dg-final { scan-assembler-times {0x3b098f27} 1 } } */
/* { dg-final { scan-assembler-times {0x2b11c4bc} 1 } } */

/* Local function definitions - these will NOT get __kcfi_typeid_ symbols (only external declarations do) */
void local_func_void(void) { }                  /* FvvE -> 0x126cd6c8 */
void local_func_short(short x) { }              /* FvsE -> 0xd28c44fd */
void local_func_uint(unsigned int x) { }        /* FvjE -> 0x9ea34c70 */
void local_func_float(float x) { }              /* FvfE -> 0xbeac792c */

/* Local function validation - verify local function definitions do NOT get __kcfi_typeid_ symbols */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_void\n} } } */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_short\n} } } */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_uint\n} } } */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_float\n} } } */

/* Local pointer parameter types */
void local_func_double_ptr(double *x) { }       /* FvPdE -> 0x00ad82c4 */
void local_func_float_ptr(float *x) { }         /* FvPfE -> 0xb4a88df2 */

/* Local pointer parameter types - should NOT emit symbols */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_double_ptr\n} } } */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_float_ptr\n} } } */

/* Local nested pointers */
void local_func_void_ptr_ptr(void **x) { }      /* FvPPvE -> 0x63f671c4 */

/* Local nested pointers - should NOT emit symbols */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_void_ptr_ptr\n} } } */

/* Local mixed parameters */
void local_func_ptr_val(int *x, int y) { }      /* FvPiiE -> 0x90e41784 */
void local_func_val_ptr(int x, int *y) { }      /* FviPiE -> 0xd4a79cb6 */

/* Local mixed parameter validation - should NOT emit symbols */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_ptr_val\n} } } */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_val_ptr\n} } } */

/* Local return types */
float local_func_return_float(void) { return 0.0f; }    /* FfvE -> 0xf29546d8 */
double local_func_return_double(void) { return 0.0; }   /* FdvE -> 0x268f8886 */

/* Local return type discrimination - should NOT emit symbols */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_return_float\n} } } */
/* { dg-final { scan-assembler-not {\t\.weak\t__kcfi_typeid_local_func_return_double\n} } } */

struct not_void {
	int nothing;
};

/* Function that takes addresses to make functions visible to KCFI */
void test_address_taken(struct not_void *arg)
{
    /* External functions - taking addresses generates __kcfi_typeid_ symbols */
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

    /* Array parameters - should decay to pointers */
    void (*p16)(int*) = func_int_array;
    void (*p17)(char*) = func_char_array;

    /* Function pointer parameters */
    void (*p18)(void(*)(void)) = func_fptr_void;
    void (*p19)(void(*)(int)) = func_fptr_int;
    void (*p20)(int(*)(void)) = func_fptr_ret_int;

    /* Struct/union/enum function pointers - taking addresses generates __kcfi_typeid_ symbols */
    void (*p_struct_a_ptr)(struct test_struct_a*) = func_struct_a_ptr;
    void (*p_struct_b_ptr)(struct test_struct_b*) = func_struct_b_ptr;
    void (*p_struct_c_ptr)(struct test_struct_c*) = func_struct_c_ptr;

    /* Const struct function pointers - taking addresses generates __kcfi_typeid_ symbols */
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

    /* Typedef vs open-coded function type assignments - should generate identical type IDs */
    void (*p_with_typedef_param)(func_ptr_typedef) = func_with_typedef_param;
    void (*p_with_opencoded_param)(void (*)(int, char)) = func_with_opencoded_param;
    ret_func_ptr_typedef (*p_ret_typedef_param)(void) = func_ret_typedef_param;
    int (*(*p_ret_opencoded_param)(void))(void) = func_ret_opencoded_param;

    /* Anonymous struct typedef assignments - should generate unique type IDs */
    void (*p_anon_typedef_1)(anon_typedef_1 *) = func_anon_typedef_1;
    void (*p_anon_typedef_2)(anon_typedef_2 *) = func_anon_typedef_2;

    /* Union member access discrimination test - take addresses to generate __kcfi_typeid_ symbols */
    void (*p_tasklet_callback)(struct tasklet_like_struct *) = tasklet_callback_function;
    void (*p_tasklet_func)(unsigned long) = tasklet_func_function;

    /* Local functions - taking addresses does NOT generate __kcfi_typeid_ symbols (only external declarations do) */
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

    /* Use pointers to prevent optimization - external functions */
    if (p1) p1();
    if (p2) p2('x');
    if (p3) p3(42);
    if (p4) p4(42L);
    if (p5) p5((int*)0);
    if (p6) p6((char*)0);
    if (p7) p7((void*)0);

    /* Use const qualifier pointers to prevent optimization */
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

    /* Use pointers to prevent optimization - local functions */
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

    struct tasklet_like_struct test_tasklet = { };
    test_tasklet.callback = tasklet_callback_function;  /* Set callback function */

    /* This indirect call through union->callback MUST generate type ID 0x5c500b1a (callback signature) */
    /* NOT type ID 0x9e92034f (func signature from first union member) */
    /* Force indirect call through union member access */
    struct tasklet_like_struct *volatile tasklet_ptr = &test_tasklet;
    if (tasklet_ptr->callback) {
        /* This call should match tasklet_callback_function type ID */
        tasklet_ptr->callback(tasklet_ptr);
    }
}

/* Named struct and its typedef should have IDENTICAL type IDs after canonicalization */
struct named_for_typedef_test { int member; };
typedef struct named_for_typedef_test named_for_typedef_test_t;

extern void func_named_struct_param(struct named_for_typedef_test *param);
extern void func_typedef_struct_param(named_for_typedef_test_t *param);

/* Named struct typedef canonicalization - MUST have identical type IDs */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_named_struct_param\n\t\.set\t__kcfi_typeid_func_named_struct_param, 0xf5a8ad24} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_typedef_struct_param\n\t\.set\t__kcfi_typeid_func_typedef_struct_param, 0xf5a8ad24} } } */

/* Verify exact count - both should generate exactly 2 symbols with identical values */
/* { dg-final { scan-assembler-times {0xf5a8ad24} 2 } } */

void test_named_struct_typedef_canonicalization(struct not_void *arg) {
    /* These should be compatible after canonicalization */
    void (*fp_struct)(struct named_for_typedef_test *) = func_named_struct_param;
    void (*fp_typedef)(struct named_for_typedef_test *) = func_typedef_struct_param;  /* Should work with canonicalization */

    /* Take addresses to generate type IDs */
    if (fp_struct) fp_struct((struct named_for_typedef_test *)0);
    if (fp_typedef) fp_typedef((struct named_for_typedef_test *)0);
}

/* Basic type typedef canonicalization - typedef should canonicalize to underlying basic type */

/* Basic type typedefs commonly used in kernel code */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

/* Functions with basic type typedef vs original type parameters */
extern void func_u8_param(u8 param);
extern void func_unsigned_char_param(unsigned char param);
extern void func_u16_param(u16 param);
extern void func_unsigned_short_param(unsigned short param);
extern void func_u32_param(u32 param);
extern void func_unsigned_int_param(unsigned int param);

void test_basic_typedef_canonicalization(struct not_void *arg) {
    /* These should be compatible after canonicalization */
    void (*fp_u8)(unsigned char) = func_u8_param;                    /* Should work with canonicalization */
    void (*fp_uchar)(unsigned char) = func_unsigned_char_param;      /* Should work normally */
    void (*fp_u16)(unsigned short) = func_u16_param;                 /* Should work with canonicalization */
    void (*fp_ushort)(unsigned short) = func_unsigned_short_param;   /* Should work normally */
    void (*fp_u32)(unsigned int) = func_u32_param;                   /* Should work with canonicalization */
    void (*fp_uint)(unsigned int) = func_unsigned_int_param;         /* Should work normally */

    /* Take addresses to generate type IDs */
    if (fp_u8) fp_u8(0);
    if (fp_uchar) fp_uchar(0);
    if (fp_u16) fp_u16(0);
    if (fp_ushort) fp_ushort(0);
    if (fp_u32) fp_u32(0);
    if (fp_uint) fp_uint(0);
}

/* Basic type typedef canonicalization - MUST have identical type IDs after canonicalization */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u8_param\n\t\.set\t__kcfi_typeid_func_u8_param, 0xd29d8e1e} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_unsigned_char_param\n\t\.set\t__kcfi_typeid_func_unsigned_char_param, 0xd29d8e1e} } } */
/* Count test is below, which includes other tests that use this hash. */

/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u16_param\n\t\.set\t__kcfi_typeid_func_u16_param, 0x9280a74a} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_unsigned_short_param\n\t\.set\t__kcfi_typeid_func_unsigned_short_param, 0x9280a74a} } } */
/* { dg-final { scan-assembler-times {0x9280a74a} 2 } } */

/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u32_param\n\t\.set\t__kcfi_typeid_func_u32_param, 0x9ea34c70} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_unsigned_int_param\n\t\.set\t__kcfi_typeid_func_unsigned_int_param, 0x9ea34c70} } } */
/* Count test is below, which includes other tests that use this hash. */

/* Verify exact count - each typedef/basic type pair should generate exactly 2 symbols with identical values */
/* Note: Counts updated below to include recursive typedef tests */

/* Recursive typedef canonicalization - test multi-level typedef chains */

/* Kernel-style recursive typedef chains that need full canonicalization */
typedef unsigned char __u8_recursive;
typedef __u8_recursive u8_recursive;

typedef unsigned int __u32_recursive;
typedef __u32_recursive u32_recursive;

/* Three-level typedef chains */
typedef unsigned char base_u8_recursive_t;
typedef base_u8_recursive_t mid_u8_recursive_t;
typedef mid_u8_recursive_t top_u8_recursive_t;

/* Struct recursive typedef chains */
struct recursive_struct_test { int value; };
typedef struct recursive_struct_test base_recursive_struct_t;
typedef base_recursive_struct_t top_recursive_struct_t;

/* Functions with recursive typedefs - MUST have same type IDs as canonical forms */
extern void func_u8_recursive_chain(u8_recursive param);          /* u8_recursive -> __u8_recursive -> unsigned char */
extern void func_u8_recursive_mid(__u8_recursive param);          /* __u8_recursive -> unsigned char */
extern void func_u8_recursive_base(unsigned char param);          /* unsigned char (baseline) */

extern void func_u32_recursive_chain(u32_recursive param);        /* u32_recursive -> __u32_recursive -> unsigned int */
extern void func_u32_recursive_mid(__u32_recursive param);        /* __u32_recursive -> unsigned int */
extern void func_u32_recursive_base(unsigned int param);          /* unsigned int (baseline) */

extern void func_three_level_recursive(top_u8_recursive_t param); /* top -> mid -> base -> unsigned char */
extern void func_three_level_mid(mid_u8_recursive_t param);       /* mid -> base -> unsigned char */
extern void func_three_level_base(base_u8_recursive_t param);     /* base -> unsigned char */
extern void func_three_level_final(unsigned char param);          /* unsigned char (baseline) */

extern void func_struct_recursive_chain(top_recursive_struct_t *param);     /* Should resolve to struct name */
extern void func_struct_recursive_mid(base_recursive_struct_t *param);      /* Should resolve to struct name */
extern void func_struct_recursive_original(struct recursive_struct_test *param); /* struct name (baseline) */

void test_recursive_canonicalization(struct not_void *arg) {
    /* Recursive typedef function pointers - should be compatible after full canonicalization */
    void (*fp_u8_chain)(unsigned char) = func_u8_recursive_chain;        /* Should work after 2-level canonicalization */
    void (*fp_u8_mid)(unsigned char) = func_u8_recursive_mid;            /* Should work after 1-level canonicalization */
    void (*fp_u8_base)(unsigned char) = func_u8_recursive_base;          /* Should work normally */

    void (*fp_u32_chain)(unsigned int) = func_u32_recursive_chain;       /* Should work after 2-level canonicalization */
    void (*fp_u32_mid)(unsigned int) = func_u32_recursive_mid;           /* Should work after 1-level canonicalization */
    void (*fp_u32_base)(unsigned int) = func_u32_recursive_base;         /* Should work normally */

    void (*fp_three_chain)(unsigned char) = func_three_level_recursive;  /* Should work after 3-level canonicalization */
    void (*fp_three_mid)(unsigned char) = func_three_level_mid;          /* Should work after 2-level canonicalization */
    void (*fp_three_base)(unsigned char) = func_three_level_base;        /* Should work after 1-level canonicalization */
    void (*fp_three_final)(unsigned char) = func_three_level_final;      /* Should work normally */

    void (*fp_struct_chain)(struct recursive_struct_test *) = func_struct_recursive_chain;  /* Should work after canonicalization */
    void (*fp_struct_mid)(struct recursive_struct_test *) = func_struct_recursive_mid;      /* Should work after canonicalization */
    void (*fp_struct_orig)(struct recursive_struct_test *) = func_struct_recursive_original; /* Should work normally */

    /* Use function pointers to prevent optimization */
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

/* Recursive typedef canonicalization validation - MUST have identical type IDs after full canonicalization */

/* u8 recursive chain - all should resolve to unsigned char */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u8_recursive_chain\n\t\.set\t__kcfi_typeid_func_u8_recursive_chain, 0xd29d8e1e} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u8_recursive_mid\n\t\.set\t__kcfi_typeid_func_u8_recursive_mid, 0xd29d8e1e} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u8_recursive_base\n\t\.set\t__kcfi_typeid_func_u8_recursive_base, 0xd29d8e1e} } } */

/* u32 recursive chain - all should resolve to unsigned int */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u32_recursive_chain\n\t\.set\t__kcfi_typeid_func_u32_recursive_chain, 0x9ea34c70} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u32_recursive_mid\n\t\.set\t__kcfi_typeid_func_u32_recursive_mid, 0x9ea34c70} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_u32_recursive_base\n\t\.set\t__kcfi_typeid_func_u32_recursive_base, 0x9ea34c70} } } */

/* Three-level u8 recursive chain - all should resolve to unsigned char */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_three_level_recursive\n\t\.set\t__kcfi_typeid_func_three_level_recursive, 0xd29d8e1e} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_three_level_mid\n\t\.set\t__kcfi_typeid_func_three_level_mid, 0xd29d8e1e} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_three_level_base\n\t\.set\t__kcfi_typeid_func_three_level_base, 0xd29d8e1e} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_three_level_final\n\t\.set\t__kcfi_typeid_func_three_level_final, 0xd29d8e1e} } } */

/* Struct recursive chain - all should resolve to same struct name */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_recursive_chain\n\t\.set\t__kcfi_typeid_func_struct_recursive_chain, 0x9a4c7f0a} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_recursive_mid\n\t\.set\t__kcfi_typeid_func_struct_recursive_mid, 0x9a4c7f0a} } } */
/* { dg-final { scan-assembler {\t\.weak\t__kcfi_typeid_func_struct_recursive_original\n\t\.set\t__kcfi_typeid_func_struct_recursive_original, 0x9a4c7f0a} } } */

/* Update counts to include recursive typedef tests */
/* Note: u8/unsigned char recursive tests add 7 more occurrences (actual count: 9) */
/* { dg-final { scan-assembler-times {0xd29d8e1e} 9 } } */

/* Note: u32/unsigned int recursive tests add 3 more occurrences (actual count: 6) */
/* { dg-final { scan-assembler-times {0x9ea34c70} 6 } } */

/* Struct recursive: 3 identical type IDs */
/* { dg-final { scan-assembler-times {0x9a4c7f0a} 3 } } */
