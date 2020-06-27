/* { dg-do compile  { target i?86-*-linux* x86_64-*-linux* } } */
/* { dg-options "-O2 -fdump-rtl-loop2_unroll -fno-peel-loops -fdisable-tree-cunroll=foo -fdisable-tree-cunrolli=foo -fenable-rtl-loop2_unroll" } */

unsigned a[100], b[100];
inline void bar()
{
 a[10] = b[10];
}

int foo(void)
{
  int i;
  bar();
  for (i = 0; i < 2; i++)
  {
     a[i]= b[i] + 1;
  }
  return 1;
}

int foo2(void)
{
  int i;
  for (i = 0; i < 2; i++)
  {
     a[i]= b[i] + 1;
  }
  return 1;
}

/* { dg-final { scan-rtl-dump-times "Decided to peel loop completely" 1 "loop2_unroll" } } */
/* { dg-final { cleanup-rtl-dump "loop2_unroll" } } */
/* { dg-excess-errors "extra notes" } */
