/* Compile with the target compiler; do not execute on the build host. */
#include <stddef.h>
#include "prtypes.h"
#include "jsautocfg.h"

#define CHECK(name, condition) typedef char name[(condition) ? 1 : -1]
CHECK(js_word, JS_BYTES_PER_WORD == sizeof(void *));
CHECK(js_long, JS_BYTES_PER_LONG == sizeof(long));
CHECK(js_int, JS_BYTES_PER_INT == sizeof(int));
CHECK(js_double, JS_BYTES_PER_DOUBLE == sizeof(double));
CHECK(nspr_word, PR_BYTES_PER_WORD == sizeof(void *));
CHECK(nspr_long, PR_BYTES_PER_LONG == sizeof(long));
struct align_double { char byte; double value; };
struct align_pointer { char byte; void *value; };
CHECK(js_double_alignment, JS_ALIGN_OF_DOUBLE == offsetof(struct align_double, value));
CHECK(js_pointer_alignment, JS_ALIGN_OF_POINTER == offsetof(struct align_pointer, value));
CHECK(nspr_double_alignment, PR_ALIGN_OF_DOUBLE == offsetof(struct align_double, value));
#if defined(__LITTLE_ENDIAN__)
#ifndef IS_LITTLE_ENDIAN
#error "Target byte order disagrees with generated configuration"
#endif
#ifdef IS_BIG_ENDIAN
#error "Conflicting target byte order definitions"
#endif
#else
#ifndef IS_BIG_ENDIAN
#error "Target byte order disagrees with generated configuration"
#endif
#ifdef IS_LITTLE_ENDIAN
#error "Conflicting target byte order definitions"
#endif
#endif
