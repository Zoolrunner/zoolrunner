/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Cocoa drag service for LP64 macOS.  This is deliberately separate from
 * the Carbon Drag Manager implementation used by historical Mac targets.
 */

#import <Cocoa/Cocoa.h>

#include "nsDragService.h"
#include "nsCOMPtr.h"
#include "nsITransferable.h"
#include "nsISupportsPrimitives.h"
#include "nsILocalFile.h"
#include "nsMemory.h"
#include "nsNetUtil.h"
#include "nsPrimitiveHelpers.h"
#include "nsString.h"
#include "nsXPIDLString.h"

extern NSPasteboard *gCocoaDragPasteboard;
extern NSView *gCocoaLastDragView;
extern NSEvent *gCocoaLastDragEvent;

static NSString * const kCocoaWildcardPboardType = @"ZoolRunnerWildcard";
static NSString * const kCocoaURLPboardType =
  @"CorePasteboardFlavorType 0x75726C20";
static NSString * const kCocoaURLDescriptionPboardType =
  @"CorePasteboardFlavorType 0x75726C64";

static NSString *
PasteboardTypeForFlavor(const char *aFlavor)
{
  if (!strcmp(aFlavor, kUnicodeMime) || !strcmp(aFlavor, kTextMime))
    return NSStringPboardType;
  if (!strcmp(aFlavor, kHTMLMime))
    return NSHTMLPboardType;
  if (!strcmp(aFlavor, kURLMime) || !strcmp(aFlavor, kURLDataMime))
    return kCocoaURLPboardType;
  if (!strcmp(aFlavor, kURLDescriptionMime))
    return kCocoaURLDescriptionPboardType;
  if (!strcmp(aFlavor, kFileMime))
    return NSFilenamesPboardType;
  if (!strcmp(aFlavor, kPNGImageMime))
    return @"public.png";
  if (!strcmp(aFlavor, kJPEGImageMime))
    return @"public.jpeg";
  if (!strcmp(aFlavor, kGIFImageMime))
    return @"com.compuserve.gif";
  return [NSString stringWithUTF8String:aFlavor];
}

static PRBool
FlavorUsesUnicodeString(const char *aFlavor)
{
  return !strcmp(aFlavor, kUnicodeMime) || !strcmp(aFlavor, kHTMLMime) ||
         !strcmp(aFlavor, kURLMime) || !strcmp(aFlavor, kURLDataMime) ||
         !strcmp(aFlavor, kURLDescriptionMime);
}

static nsresult
PutTransferableOnDragPasteboard(nsITransferable *aTransferable)
{
  nsCOMPtr<nsISupportsArray> flavors;
  nsresult rv = aTransferable->FlavorsTransferableCanExport(
    getter_AddRefs(flavors));
  NS_ENSURE_SUCCESS(rv, rv);

  NSMutableArray *types = [NSMutableArray array];
  NSMutableDictionary *values = [NSMutableDictionary dictionary];
  PRUint32 count = 0;
  flavors->Count(&count);
  for (PRUint32 i = 0; i < count; ++i) {
    nsCOMPtr<nsISupports> flavorSupports;
    flavors->GetElementAt(i, getter_AddRefs(flavorSupports));
    nsCOMPtr<nsISupportsCString> flavorObject(
      do_QueryInterface(flavorSupports));
    if (!flavorObject)
      continue;

    nsXPIDLCString flavor;
    flavorObject->ToString(getter_Copies(flavor));
    NSString *type = PasteboardTypeForFlavor(flavor);
    if (!type)
      continue;

    nsCOMPtr<nsISupports> dataObject;
    PRUint32 dataLength = 0;
    if (NS_FAILED(aTransferable->GetTransferData(
                    flavor, getter_AddRefs(dataObject), &dataLength)) ||
        !dataObject)
      continue;

    id value = nil;
    if (!strcmp(flavor, kFileMime)) {
      nsCOMPtr<nsILocalFile> file(do_QueryInterface(dataObject));
      if (file) {
        nsAutoString path;
        if (NS_SUCCEEDED(file->GetPath(path))) {
          NSString *nativePath = [NSString
            stringWithCharacters:(const unichar *)path.get()
                          length:path.Length()];
          value = [NSArray arrayWithObject:nativePath];
        }
      }
    } else {
      void *data = nsnull;
      nsPrimitiveHelpers::CreateDataFromPrimitive(flavor, dataObject, &data,
                                                  dataLength);
      if (!data)
        continue;
      if (FlavorUsesUnicodeString(flavor)) {
        value = [NSString stringWithCharacters:(const unichar *)data
                                        length:dataLength / sizeof(PRUnichar)];
      } else {
        value = [NSData dataWithBytes:data length:dataLength];
      }
      nsMemory::Free(data);
    }

    if (value) {
      if (![types containsObject:type])
        [types addObject:type];
      [values setObject:value forKey:type];
    }
  }

  if (![types count])
    return NS_ERROR_FAILURE;
  [types addObject:kCocoaWildcardPboardType];

  NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:NSDragPboard];
  [pasteboard declareTypes:types owner:nil];
  NSUInteger typeCount = [types count] - 1;
  for (NSUInteger i = 0; i < typeCount; ++i) {
    NSString *type = [types objectAtIndex:i];
    id value = [values objectForKey:type];
    if ([value isKindOfClass:[NSString class]])
      [pasteboard setString:value forType:type];
    else if ([value isKindOfClass:[NSArray class]])
      [pasteboard setPropertyList:value forType:type];
    else
      [pasteboard setData:value forType:type];
  }
  return NS_OK;
}

NS_IMPL_ADDREF_INHERITED(nsDragService, nsBaseDragService)
NS_IMPL_RELEASE_INHERITED(nsDragService, nsBaseDragService)
NS_IMPL_QUERY_INTERFACE4(nsDragService, nsIDragService,
                         nsIDragService_1_8_BRANCH, nsIDragSession,
                         nsIDragSessionMac)

nsDragService::nsDragService()
  : mDataItems(nsnull), mNativeDragView(nsnull), mNativeDragEvent(nsnull)
{
}

nsDragService::~nsDragService()
{
  NS_IF_RELEASE(mDataItems);
  [(id)mNativeDragView release];
  [(id)mNativeDragEvent release];
}

NS_IMETHODIMP
nsDragService::InvokeDragSession(nsIDOMNode *aDOMNode,
                                 nsISupportsArray *aTransferableArray,
                                 nsIScriptableRegion *aDragRegion,
                                 PRUint32 aActionType)
{
  nsresult rv = nsBaseDragService::InvokeDragSession(
    aDOMNode, aTransferableArray, aDragRegion, aActionType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!aTransferableArray || !gCocoaLastDragView || !gCocoaLastDragEvent)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupports> supports;
  aTransferableArray->GetElementAt(0, getter_AddRefs(supports));
  nsCOMPtr<nsITransferable> transferable(do_QueryInterface(supports));
  if (!transferable)
    return NS_ERROR_FAILURE;
  rv = PutTransferableOnDragPasteboard(transferable);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_RELEASE(mDataItems);
  mDataItems = aTransferableArray;
  NS_ADDREF(mDataItems);
  mNativeDragView = [gCocoaLastDragView retain];
  mNativeDragEvent = [gCocoaLastDragEvent retain];

  NSImage *image = [[[NSImage alloc] initWithSize:NSMakeSize(20, 20)] autorelease];
  [image lockFocus];
  [[NSColor colorWithCalibratedWhite:0.3 alpha:0.55] set];
  [NSBezierPath fillRect:NSMakeRect(0, 0, 20, 20)];
  [image unlockFocus];

  nsBaseDragService::StartDragSession();
  NSPoint location = [gCocoaLastDragView convertPoint:
    [gCocoaLastDragEvent locationInWindow] fromView:nil];
  [gCocoaLastDragView dragImage:image
                             at:location
                         offset:NSMakeSize(0, 0)
                          event:gCocoaLastDragEvent
                     pasteboard:[NSPasteboard pasteboardWithName:NSDragPboard]
                         source:gCocoaLastDragView
                      slideBack:YES];
  if (mDoingDrag)
    nsBaseDragService::EndDragSession();

  [(id)mNativeDragView release];
  [(id)mNativeDragEvent release];
  mNativeDragView = nsnull;
  mNativeDragEvent = nsnull;
  NS_IF_RELEASE(mDataItems);
  mDataItems = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsDragService::GetData(nsITransferable *aTransferable, PRUint32 aItemIndex)
{
  NS_ENSURE_ARG_POINTER(aTransferable);

  nsCOMPtr<nsISupportsArray> flavors;
  nsresult rv = aTransferable->FlavorsTransferableCanImport(
    getter_AddRefs(flavors));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 flavorCount = 0;
  flavors->Count(&flavorCount);
  if (mDataItems) {
    nsCOMPtr<nsISupports> supports;
    mDataItems->GetElementAt(aItemIndex, getter_AddRefs(supports));
    nsCOMPtr<nsITransferable> source(do_QueryInterface(supports));
    if (source) {
      for (PRUint32 i = 0; i < flavorCount; ++i) {
        nsCOMPtr<nsISupports> flavorSupports;
        flavors->GetElementAt(i, getter_AddRefs(flavorSupports));
        nsCOMPtr<nsISupportsCString> flavorObject(
          do_QueryInterface(flavorSupports));
        if (!flavorObject)
          continue;
        nsXPIDLCString flavor;
        flavorObject->ToString(getter_Copies(flavor));
        nsCOMPtr<nsISupports> data;
        PRUint32 length = 0;
        if (NS_SUCCEEDED(source->GetTransferData(
                           flavor, getter_AddRefs(data), &length))) {
          aTransferable->SetTransferData(flavor, data, length);
          return NS_OK;
        }
      }
    }
  }

  NSPasteboard *pasteboard = gCocoaDragPasteboard;
  if (!pasteboard)
    pasteboard = [NSPasteboard pasteboardWithName:NSDragPboard];
  for (PRUint32 i = 0; i < flavorCount; ++i) {
    nsCOMPtr<nsISupports> flavorSupports;
    flavors->GetElementAt(i, getter_AddRefs(flavorSupports));
    nsCOMPtr<nsISupportsCString> flavorObject(
      do_QueryInterface(flavorSupports));
    if (!flavorObject)
      continue;
    nsXPIDLCString flavor;
    flavorObject->ToString(getter_Copies(flavor));
    NSString *type = PasteboardTypeForFlavor(flavor);
    if (!type || ![pasteboard availableTypeFromArray:
                    [NSArray arrayWithObject:type]])
      continue;

    if (!strcmp(flavor, kFileMime)) {
      NSArray *files = [pasteboard propertyListForType:NSFilenamesPboardType];
      if (aItemIndex >= [files count])
        continue;
      NSString *path = [files objectAtIndex:aItemIndex];
      NSUInteger length = [path length];
      if (length > PR_UINT32_MAX)
        continue;
      PRUnichar *characters = (PRUnichar *)nsMemory::Alloc(
        (length + 1) * sizeof(PRUnichar));
      if (!characters)
        return NS_ERROR_OUT_OF_MEMORY;
      [path getCharacters:(unichar *)characters];
      characters[length] = 0;
      nsCOMPtr<nsILocalFile> file;
      rv = NS_NewLocalFile(nsDependentString(characters, (PRUint32)length),
                           PR_TRUE,
                           getter_AddRefs(file));
      nsMemory::Free(characters);
      if (NS_SUCCEEDED(rv) && file) {
        nsCOMPtr<nsISupports> data(do_QueryInterface(file));
        aTransferable->SetTransferData(flavor, data, 0);
        return NS_OK;
      }
      continue;
    }

    nsCOMPtr<nsISupports> primitive;
    if (FlavorUsesUnicodeString(flavor) || !strcmp(flavor, kTextMime)) {
      NSString *value = [pasteboard stringForType:type];
      if (!value)
        continue;
      NSUInteger length = [value length];
      PRUnichar *characters = (PRUnichar *)nsMemory::Alloc(
        length * sizeof(PRUnichar));
      if (!characters && length)
        return NS_ERROR_OUT_OF_MEMORY;
      [value getCharacters:(unichar *)characters];
      PRUint32 byteLength = (PRUint32)(length * sizeof(PRUnichar));
      const char *primitiveFlavor = !strcmp(flavor, kTextMime) ?
                                    kUnicodeMime : flavor.get();
      nsPrimitiveHelpers::CreatePrimitiveForData(
        primitiveFlavor, characters, byteLength, getter_AddRefs(primitive));
      nsMemory::Free(characters);
      if (primitive) {
        aTransferable->SetTransferData(flavor, primitive, byteLength);
        return NS_OK;
      }
    } else {
      NSData *value = [pasteboard dataForType:type];
      if (!value || [value length] > PR_UINT32_MAX)
        continue;
      PRUint32 length = (PRUint32)[value length];
      nsPrimitiveHelpers::CreatePrimitiveForData(
        flavor, (void *)[value bytes], length, getter_AddRefs(primitive));
      if (primitive) {
        aTransferable->SetTransferData(flavor, primitive, length);
        return NS_OK;
      }
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDragService::IsDataFlavorSupported(const char *aDataFlavor, PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_FALSE;
  if (mDataItems) {
    PRUint32 count = 0;
    mDataItems->Count(&count);
    for (PRUint32 i = 0; i < count; ++i) {
      nsCOMPtr<nsISupports> supports;
      mDataItems->GetElementAt(i, getter_AddRefs(supports));
      nsCOMPtr<nsITransferable> item(do_QueryInterface(supports));
      if (!item)
        continue;
      nsCOMPtr<nsISupportsArray> flavors;
      if (NS_FAILED(item->FlavorsTransferableCanExport(getter_AddRefs(flavors))))
        continue;
      PRUint32 flavorCount = 0;
      flavors->Count(&flavorCount);
      for (PRUint32 j = 0; j < flavorCount; ++j) {
        nsCOMPtr<nsISupports> flavorSupports;
        flavors->GetElementAt(j, getter_AddRefs(flavorSupports));
        nsCOMPtr<nsISupportsCString> flavorObject(
          do_QueryInterface(flavorSupports));
        nsXPIDLCString flavor;
        if (flavorObject)
          flavorObject->ToString(getter_Copies(flavor));
        if (flavor && !strcmp(flavor, aDataFlavor)) {
          *aResult = PR_TRUE;
          return NS_OK;
        }
      }
    }
  }
  NSPasteboard *pasteboard = gCocoaDragPasteboard;
  if (!pasteboard)
    pasteboard = [NSPasteboard pasteboardWithName:NSDragPboard];
  NSString *type = PasteboardTypeForFlavor(aDataFlavor);
  *aResult = type && [pasteboard availableTypeFromArray:
                       [NSArray arrayWithObject:type]] != nil;
  return NS_OK;
}

NS_IMETHODIMP
nsDragService::GetNumDropItems(PRUint32 *aNumItems)
{
  NS_ENSURE_ARG_POINTER(aNumItems);
  *aNumItems = 0;
  if (mDataItems)
    return mDataItems->Count(aNumItems);
  NSPasteboard *pasteboard = gCocoaDragPasteboard;
  if (!pasteboard)
    return NS_OK;
  NSArray *files = [pasteboard propertyListForType:NSFilenamesPboardType];
  if (files)
    *aNumItems = (PRUint32)[files count];
  else if ([[pasteboard types] count])
    *aNumItems = 1;
  return NS_OK;
}

NS_IMETHODIMP
nsDragService::SetDragAction(PRUint32 aAction)
{
  return nsBaseDragService::SetDragAction(aAction);
}

NS_IMETHODIMP
nsDragService::SetDragReference(DragReference aDragRef)
{
  // Carbon DragReferences have no role in the Cocoa destination protocol.
  return NS_OK;
}
