/* Test KCFI check sharing bug - optimizer incorrectly shares KCFI checks between different function types */
/* { dg-do compile } */
/* { dg-options "-fsanitize=kcfi -O2" } */

/* Reproduce the pattern from Linux kernel internal_create_group where:
   - Two different function pointer types (is_visible vs is_bin_visible)
   - Both get loaded into the same register (%rcx)
   - Optimizer creates shared KCFI check with wrong type ID
   - This causes CFI failures in production kernel */

struct kobject { int dummy; };
struct attribute { int dummy; };
struct bin_attribute { int dummy; };

struct attribute_group {
    const char *name;
    int (*is_visible)(struct kobject *, struct attribute *, int);       // Type ID A
    int (*is_bin_visible)(struct kobject *, const struct bin_attribute *, int);  // Type ID B
    struct attribute **attrs;
    const struct bin_attribute **bin_attrs;
};

/* Function that mimics __first_visible from kernel - gets inlined into caller */
static int __first_visible(const struct attribute_group *grp, struct kobject *kobj)
{
    /* Path 1: Call is_visible function pointer */
    if (grp->attrs && grp->attrs[0] && grp->is_visible)
        return grp->is_visible(kobj, grp->attrs[0], 0);

    /* Path 2: Call is_bin_visible function pointer */
    if (grp->bin_attrs && grp->bin_attrs[0] && grp->is_bin_visible)
        return grp->is_bin_visible(kobj, grp->bin_attrs[0], 0);

    return 0;
}

/* Main function that triggers the optimization bug */
int test_kcfi_check_sharing(struct kobject *kobj, const struct attribute_group *grp)
{
    /* This should inline __first_visible and create the problematic pattern where:
       1. Both function pointers get loaded into same register
       2. Optimizer shares KCFI check between them
       3. Uses wrong type ID for one of the calls */
    return __first_visible(grp, kobj);
}

/* Each indirect call should have its own KCFI check with correct type ID

   Should see:
   1. KCFI check for is_visible call with is_visible type ID
   2. KCFI check for is_bin_visible call with is_bin_visible type ID */

/* Verify we have TWO different KCFI check sequences */
/* Each check should have different type ID constants */
/* x86: { dg-final { scan-assembler-times {movl\s+\$-?[0-9]+,\s+%r10d} 2 { target i?86-*-* x86_64-*-* } } } */
/* AArch64: { dg-final { scan-assembler-times {mov\s+w17, #[0-9]+} 2 { target aarch64*-*-* } } } */

/* Verify the checks use DIFFERENT type IDs (not shared) */
/* We should NOT see the same type ID used twice - that would indicate sharing bug */
/* x86: { dg-final { scan-assembler-not {movl\s+\$(-?[0-9]+),\s+%r10d.*movl\s+\$\1,\s+%r10d} { target i?86-*-* x86_64-*-* } } } */
/* AArch64: { dg-final { scan-assembler-not {mov\s+w17, #([0-9]+).*mov\s+w17, #\1} { target aarch64*-*-* } } } */

/* Verify each call follows its own check (not shared) */
/* Should have 2 separate trap instructions */
/* x86: { dg-final { scan-assembler-times {ud2} 2 { target i?86-*-* x86_64-*-* } } } */
/* AArch64: { dg-final { scan-assembler-times {brk\s+#[0-9]+} 2 { target aarch64*-*-* } } } */

/* Verify 2 separate call sites */
/* x86: { dg-final { scan-assembler-times {jmp\s+\*%[a-z0-9]+} 2 { target i?86-*-* x86_64-*-* } } } */
/* AArch64: Allow both blr (regular call) and br (tail call) */
/* AArch64: { dg-final { scan-assembler-times {br	x[0-9]+} 2 { target aarch64*-*-* } } } */
