/* Compatibility declarations hidden by current SDK headers on LP64. */
#ifndef zoolrunner_ATSUICompat_h
#define zoolrunner_ATSUICompat_h

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

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
extern OSStatus ATSUSetRunStyle(ATSUTextLayout iTextLayout, ATSUStyle iStyle,
                                UniCharArrayOffset iRunStart,
                                UniCharCount iRunLength);
extern OSStatus ATSUSetTransientFontMatching(ATSUTextLayout iTextLayout,
                                             Boolean iTransientFontMatching);
extern OSStatus ATSUFindFontFromName(
    const void *iName, ByteCount iNameLength, FontNameCode iFontNameCode,
    FontPlatformCode iFontNamePlatform, FontScriptCode iFontNameScript,
    FontLanguageCode iFontNameLanguage, ATSUFontID *oFontID);
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
