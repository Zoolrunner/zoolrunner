/* Core Graphics declarations for the pre-Intel SDKs. */
#ifndef zoolrunner_QuartzCompat_h
#define zoolrunner_QuartzCompat_h

#include <ApplicationServices/ApplicationServices.h>
#include <AvailabilityMacros.h>

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1040
/* Panther takes only alpha information, with implicit big-endian pixels.
 * This matches native 32-bit Cairo pixels on PowerPC. Do not substitute this
 * for the explicit byte-order flags on little-endian targets. */
#if !defined(__ppc__) && !defined(__POWERPC__)
#error Pre-Tiger Core Graphics requires the PowerPC pixel layout
#endif
typedef CGImageAlphaInfo CGBitmapInfo;
#define kCGBitmapAlphaInfoMask 0x1f
#define kCGBitmapByteOrderMask 0x7000
#define kCGBitmapByteOrder32Host 0
#define CGBitmapContextGetBitmapInfo CGBitmapContextGetAlphaInfo
#endif

#endif
