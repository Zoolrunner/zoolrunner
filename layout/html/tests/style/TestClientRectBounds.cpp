/* Exact CSSOM View boundary cases independent of font and frame construction. */
#include "nsClientRectBounds.h"
#include <stdio.h>
static unsigned checks,failures;
static void check(const nsClientRectBounds& bounds,double x,double y,double w,double h,const char* name) {
  nsClientRectBounds::Rect r=bounds.Get();
  ++checks;
  if(r.x!=x || r.y!=y || r.width!=w || r.height!=h) {
    ++failures;fprintf(stderr,"FAIL %s: %g,%g %gx%g\n",name,r.x,r.y,r.width,r.height);
  }
}
int main() {
  nsClientRectBounds empty;
  check(empty,0,0,0,0,"no fragments");
  empty.Add(70,90,0,0);
  check(empty,70,90,0,0,"single point retains origin");
  empty.Add(-100,-200,0,0);
  check(empty,70,90,0,0,"all points retain first origin");
  empty.Add(10,20,30,40);
  check(empty,10,20,30,40,"initial points do not enlarge nonempty bounds");
  empty.Add(900,900,0,0);
  check(empty,10,20,30,40,"trailing point does not enlarge bounds");
  empty.Add(-10,-20,20,30);
  check(empty,-10,-20,50,80,"union includes negative coordinates");
  nsClientRectBounds lines;
  lines.Add(5,7,0,12);lines.Add(100,200,50,0);
  check(lines,5,7,0,12,"all fragments have zero width or height");
  lines.Add(20,30,10,10);
  check(lines,5,7,145,193,"nonzero extents included when a fragment has area");
  nsClientRectBounds horizontal;
  horizontal.Add(3,4,12,0);horizontal.Add(20,30,0,5);
  check(horizontal,3,4,12,0,"all-empty horizontal first fragment");
  nsClientRectBounds fractional;
  fractional.Add(.25,.5,1.5,2.25);fractional.Add(-.5,1.25,.75,4.5);
  check(fractional,-.5,.5,2.25,5.25,"fractional coordinates preserved");
  printf("CLIENT-RECT-BOUNDS checks=%u failures=%u\n",checks,failures);
  return failures ? 1 : 0;
}
