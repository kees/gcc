/* { dg-do compile { target *-*-linux* *-*-gnu* } } */
/* { dg-require-effective-target default_pie } */
/* { dg-options "-O2" } */
int foo (void);

int
main (void)
{
	return foo ();
}

/* { dg-final { scan-assembler "foo@PLT" } } */
