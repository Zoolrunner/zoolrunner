/* Narrow LP64 replacements for Carbon routines omitted by modern SDKs. */
#ifndef zoolrunner_macos_CarbonCompat_h
#define zoolrunner_macos_CarbonCompat_h

#if defined(__LP64__)
#include <string.h>
#define BlockMove(source, destination, size) \
    memmove((destination), (source), (size_t)(size))
#define BlockMoveData(source, destination, size) \
    memmove((destination), (source), (size_t)(size))
#endif

#endif
