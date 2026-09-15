/* Declarations for dynamically resolved Core Foundation APIs with early SDKs. */
#ifndef zoolrunner_CoreFoundationCompat_h
#define zoolrunner_CoreFoundationCompat_h

#include <CoreFoundation/CoreFoundation.h>
#include <AvailabilityMacros.h>

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1020
/* Preserve the integer enum ABI of CFStringNormalize when compiling its
 * existing dynamic lookup against headers predating that function. */
typedef enum {
    kCFStringNormalizationFormD = 0,
    kCFStringNormalizationFormKD,
    kCFStringNormalizationFormC,
    kCFStringNormalizationFormKC
} CFStringNormalizationForm;
#endif

#endif
