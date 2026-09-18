/* Host numeric comparison for the selected bundled kernels; not an OS runtime test. */
#include "jsmathfd.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>
struct operation { const char *name; double (*actual)(double); double (*reference)(double); };
static struct operation operations[] = {
 {"expm1",js_math_expm1,expm1}, {"log1p",js_math_log1p,log1p},
 {"cbrt",js_math_cbrt,cbrt}, {"asinh",js_math_asinh,asinh},
 {"tanh",js_math_tanh,tanh}, {"acosh",js_math_acosh,acosh},
 {"atanh",js_math_atanh,atanh}, {"cosh",js_math_cosh,cosh},
 {"sinh",js_math_sinh,sinh}, {"log10",js_math_log10,log10}
};
static int failures;
static unsigned long checks;
static void check(double x) {
 unsigned i;
 for(i=0;i<sizeof(operations)/sizeof(operations[0]);++i) {
  double a=operations[i].actual(x), b=operations[i].reference(x);
  int ok;
  ++checks;
  if(isnan(b)) ok=isnan(a);
  else if(b==0 || isinf(b)) ok=a==b && !!signbit(a)==!!signbit(b);
  else ok=isfinite(a) && fabs(a-b)<=fmax(DBL_MIN*DBL_EPSILON, fabs(b)*DBL_EPSILON*4);
  if(!ok) { if(failures<20) printf("FAIL %s x=%a actual=%a reference=%a\n",operations[i].name,x,a,b); ++failures; }
 }
}
int main(void) {
 uint64_t state=UINT64_C(0x123456789abcdef), bits;
 double x;
 unsigned i;
 const double edges[]={0.,-0.,INFINITY,-INFINITY,NAN,DBL_MIN,DBL_MIN*DBL_EPSILON,DBL_MAX,1.,-1.,0.5,-0.5,709.,710.,710.4758600739439,711.,-56.};
 for(i=0;i<sizeof(edges)/sizeof(edges[0]);++i) {check(edges[i]);check(nextafter(edges[i],INFINITY));check(nextafter(edges[i],-INFINITY));}
 for(i=0;i<200000;++i) {
  state^=state<<13;state^=state>>7;state^=state<<17;
  bits=state;memcpy(&x,&bits,sizeof x);check(x);
 }
 printf("MATH-PORT checks=%lu failures=%d\n",checks,failures);
 return failures?1:0;
}
