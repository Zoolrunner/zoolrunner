/* GCC's assertion helper, compiled for early Darwin's printf ABI. */
#include <stdio.h>
#include <stdlib.h>

__attribute__((visibility("hidden"))) void
__eprintf(const char *format, const char *expression,
          unsigned int line, const char *filename)
{
    fprintf(stderr, format, expression, line, filename);
    fflush(stderr);
    abort();
}
