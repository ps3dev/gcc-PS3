/* { dg-do compile { target { { powerpc*-*-* } && { ! powerpc*-apple-darwin* } } } } */
/* { dg-require-effective-target powerpc_fprs } */
/* { dg-options "-O2 -mrecip -ffast-math -mcpu=power7" } */
/* { dg-final { scan-assembler-times "xsrsqrtedp" 1 } } */
/* { dg-final { scan-assembler-times "xsmsub.dp\|fmsub\ " 1 } } */
/* { dg-final { scan-assembler-times "xsmuldp" 4 } } */
/* { dg-final { scan-assembler-times "xsnmsub.dp\|fnmsub\ " 2 } } */
/* { dg-final { scan-assembler-times "frsqrtes" 1 } } */
/* { dg-final { scan-assembler-times "fmsubs" 1 } } */
/* { dg-final { scan-assembler-times "fmuls" 4 } } */
/* { dg-final { scan-assembler-times "fnmsubs" 2 } } */

double
rsqrt_d (double a)
{
  return 1.0 / __builtin_sqrt (a);
}

float
rsqrt_f (float a)
{
  return 1.0f / __builtin_sqrtf (a);
}
