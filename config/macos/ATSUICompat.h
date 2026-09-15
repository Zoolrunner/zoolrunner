/* Compatibility declarations hidden by current SDK headers on LP64. */
#ifndef zoolrunner_ATSUICompat_h
#define zoolrunner_ATSUICompat_h

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

#ifndef FloatToFixed
#define FloatToFixed(value) X2Fix(value)
#define FixedToFloat(value) ((float) Fix2X(value))
#endif
#ifndef FloatToFract
#define FloatToFract(value) X2Frac(value)
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED < 1020
/* The older SDK used these names for the same wildcard values. */
#define kFontNoPlatformCode ((FontPlatformCode) kFontNoPlatform)
#define kFontNoScriptCode ((FontScriptCode) kFontNoScript)
#define kFontNoLanguageCode ((FontLanguageCode) kFontNoLanguage)
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1020
#include <stdlib.h>
#include <stddef.h>
/* ATSUGetGlyphInfo is the original 10.0 shaped-glyph API. Unlike ATSUDirect,
 * it returns a caller-owned array with positions and the actual fallback
 * style for every glyph. Check the returned size before consuming it. */
static ATSUGlyphInfoArray *
zr_ATSUCopyGlyphInfo(ATSUTextLayout layout, UniCharCount length)
{
    ByteCount bytes = 0, capacity;
    ATSUGlyphInfoArray *info;
    OSStatus status = ATSUGetGlyphInfo(layout, 0, length, &bytes, NULL);
    if (bytes < offsetof(ATSUGlyphInfoArray, glyphs))
        return NULL;
    capacity = bytes;
    info = (ATSUGlyphInfoArray *) malloc(capacity);
    if (!info)
        return NULL;
    status = ATSUGetGlyphInfo(layout, 0, length, &bytes, info);
    if (status != noErr || bytes > capacity ||
        info->numGlyphs > (capacity - offsetof(ATSUGlyphInfoArray, glyphs)) /
                          sizeof(ATSUGlyphInfo)) {
        free(info);
        return NULL;
    }
    return info;
}
#endif

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1010
/* The direct FMFont-to-ATSFontRef conversion was added in 10.1. Resolve the
 * font's PostScript name using the two public APIs present in 10.0. */
static ATSFontRef
zr_ATSFontFromATSUFont(ATSUFontID font)
{
    ByteCount length = 0, capacity;
    ItemCount index;
    char *name;
    CFStringRef string;
    ATSFontRef result;
    OSStatus status;
    status = ATSUFindFontName(font, kFontPostscriptName,
                              kFontMacintoshPlatform, kFontRomanScript,
                              kFontNoLanguageCode, 0, NULL, &length, &index);
    if (!length)
        return 0;
    capacity = length;
    name = (char *) malloc(capacity);
    if (!name)
        return 0;
    status = ATSUFindFontName(font, kFontPostscriptName,
                              kFontMacintoshPlatform, kFontRomanScript,
                              kFontNoLanguageCode, capacity, name, &length, &index);
    if (status != noErr || length > capacity) {
        free(name);
        return 0;
    }
    string = CFStringCreateWithBytes(kCFAllocatorDefault,
                                     (const UInt8 *) name, length,
                                     kCFStringEncodingMacRoman, false);
    free(name);
    if (!string)
        return 0;
    result = ATSFontFindFromPostScriptName(string, kATSOptionFlagsDefault);
    CFRelease(string);
    return result;
}
#define FMGetATSFontRefFromFont zr_ATSFontFromATSUFont
#endif

#if defined(__LP64__)
#ifdef __cplusplus
extern "C" {
#endif

extern OSStatus ATSUCreateStyle(ATSUStyle *oStyle);
extern OSStatus ATSUCreateAndCopyStyle(ATSUStyle iStyle, ATSUStyle *oStyle);
extern OSStatus ATSUDisposeStyle(ATSUStyle iStyle);
extern OSStatus ATSUSetAttributes(ATSUStyle iStyle, ItemCount iAttributeCount,
                                  const ATSUAttributeTag iTag[],
                                  const ByteCount iValueSize[],
                                  const ATSUAttributeValuePtr iValue[]);
extern OSStatus ATSUCreateTextLayout(ATSUTextLayout *oTextLayout);
extern OSStatus ATSUCreateTextLayoutWithTextPtr(
    ConstUniCharArrayPtr iText, UniCharArrayOffset iTextOffset,
    UniCharCount iTextLength, UniCharCount iTextTotalLength,
    ItemCount iNumberOfRuns, const UniCharCount iRunLengths[],
    ATSUStyle iStyles[], ATSUTextLayout *oTextLayout);
extern OSStatus ATSUClearLayoutCache(ATSUTextLayout iTextLayout,
                                     UniCharArrayOffset iLineStart);
extern OSStatus ATSUDisposeTextLayout(ATSUTextLayout iTextLayout);
extern OSStatus ATSUSetTextPointerLocation(
    ATSUTextLayout iTextLayout, ConstUniCharArrayPtr iText,
    UniCharArrayOffset iTextOffset, UniCharCount iTextLength,
    UniCharCount iTextTotalLength);
extern OSStatus ATSUSetLayoutControls(
    ATSUTextLayout iTextLayout, ItemCount iAttributeCount,
    const ATSUAttributeTag iTag[], const ByteCount iValueSize[],
    const ATSUAttributeValuePtr iValue[]);
extern OSStatus ATSUSetLineControls(
    ATSUTextLayout iTextLayout, UniCharArrayOffset iLineStart,
    ItemCount iAttributeCount, const ATSUAttributeTag iTag[],
    const ByteCount iValueSize[], const ATSUAttributeValuePtr iValue[]);
extern OSStatus ATSUSetRunStyle(ATSUTextLayout iTextLayout, ATSUStyle iStyle,
                                UniCharArrayOffset iRunStart,
                                UniCharCount iRunLength);
extern OSStatus ATSUSetTransientFontMatching(ATSUTextLayout iTextLayout,
                                             Boolean iTransientFontMatching);
extern OSStatus ATSUFindFontFromName(
    const void *iName, ByteCount iNameLength, FontNameCode iFontNameCode,
    FontPlatformCode iFontNamePlatform, FontScriptCode iFontNameScript,
    FontLanguageCode iFontNameLanguage, ATSUFontID *oFontID);
extern OSStatus ATSUCreateFontFallbacks(ATSUFontFallbacks *oFontFallback);
extern OSStatus ATSUDisposeFontFallbacks(ATSUFontFallbacks iFontFallbacks);
extern OSStatus ATSUSetObjFontFallbacks(
    ATSUFontFallbacks iFontFallbacks, ItemCount iFontFallbacksCount,
    const ATSUFontID iFonts[], ATSUFontFallbackMethod iFontFallbackMethod);
extern OSStatus ATSUDirectGetLayoutDataArrayPtrFromTextLayout(
    ATSUTextLayout iTextLayout, UniCharArrayOffset iLineOffset,
    ATSUDirectDataSelector iDataSelector, void *oLayoutDataArrayPtr[],
    ItemCount *oLayoutDataCount);
extern OSStatus ATSUDirectReleaseLayoutDataArrayPtr(
    ATSULineRef iLineRef, ATSUDirectDataSelector iDataSelector,
    void *iLayoutDataArrayPtr[]);
extern OSStatus ATSUGlyphGetIdealMetrics(
    ATSUStyle iATSUStyle, ItemCount iNumOfGlyphs, GlyphID iGlyphIDs[],
    ByteOffset iInputOffset, ATSGlyphIdealMetrics oIdealMetrics[]);
extern OSStatus ATSUGlyphGetScreenMetrics(
    ATSUStyle iATSUStyle, ItemCount iNumOfGlyphs, GlyphID iGlyphIDs[],
    ByteOffset iInputOffset, Boolean iForcingAntiAlias,
    Boolean iAntiAliasSwitch, ATSGlyphScreenMetrics oScreenMetrics[]);
extern OSStatus ATSUGlyphGetCubicPaths(
    ATSUStyle iATSUStyle, GlyphID iGlyphID, ATSCubicMoveToUPP iMoveToProc,
    ATSCubicLineToUPP iLineToProc, ATSCubicCurveToUPP iCurveToProc,
    ATSCubicClosePathUPP iClosePathProc, void *iCallbackDataPtr,
    OSStatus *oCallbackResult);
extern OSStatus ATSUDrawText(
    ATSUTextLayout iTextLayout, UniCharArrayOffset iLineOffset,
    UniCharCount iLineLength, ATSUTextMeasurement iLocationX,
    ATSUTextMeasurement iLocationY);
extern OSStatus ATSUGetUnjustifiedBounds(
    ATSUTextLayout iTextLayout, UniCharArrayOffset iLineStart,
    UniCharCount iLineLength, ATSUTextMeasurement *oTextBefore,
    ATSUTextMeasurement *oTextAfter, ATSUTextMeasurement *oAscent,
    ATSUTextMeasurement *oDescent);
extern OSStatus ATSUGetGlyphBounds(
    ATSUTextLayout iTextLayout, ATSUTextMeasurement iTextBasePointX,
    ATSUTextMeasurement iTextBasePointY,
    UniCharArrayOffset iBoundsCharStart, UniCharCount iBoundsCharLength,
    UInt16 iTypeOfBounds, ItemCount iMaxNumberOfBounds,
    ATSTrapezoid oGlyphBounds[], ItemCount *oActualNumberOfBounds);
extern OSStatus ATSUMeasureTextImage(
    ATSUTextLayout iTextLayout, UniCharArrayOffset iLineOffset,
    UniCharCount iLineLength, ATSUTextMeasurement iLocationX,
    ATSUTextMeasurement iLocationY, Rect *oTextImageRect);
extern ATSFontRef FMGetATSFontRefFromFont(FMFont iFont);

#ifdef __cplusplus
}
#endif

/* Universal Procedure Pointers are ordinary function pointers on Mach-O. */
#ifndef NewATSCubicMoveToUPP
#define NewATSCubicMoveToUPP(proc) ((ATSCubicMoveToUPP)(proc))
#endif
#ifndef NewATSCubicLineToUPP
#define NewATSCubicLineToUPP(proc) ((ATSCubicLineToUPP)(proc))
#endif
#ifndef NewATSCubicCurveToUPP
#define NewATSCubicCurveToUPP(proc) ((ATSCubicCurveToUPP)(proc))
#endif
#ifndef NewATSCubicClosePathUPP
#define NewATSCubicClosePathUPP(proc) ((ATSCubicClosePathUPP)(proc))
#endif

#endif /* __LP64__ */
#endif /* zoolrunner_ATSUICompat_h */
