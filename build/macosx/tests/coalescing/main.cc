#include <stdio.h>
#include "shared.h"
extern "C" int* value_a();
extern "C" int* value_b();
int main() {
 int* a=value_a();int* b=value_b();int* m=&shared_value<int>();
 *a=42;
 printf("coalescing: same=%d value=%d\n",a==b && b==m,*m);
 return a==b && b==m && *m==42 ? 0 : 1;
}
