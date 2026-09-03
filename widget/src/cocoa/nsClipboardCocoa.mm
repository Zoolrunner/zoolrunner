/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * LP64 Cocoa pasteboard implementation.  The historical implementation in
 * widget/src/mac/nsClipboard.cpp remains in use by classic 32-bit targets.
 */

#import <Cocoa/Cocoa.h>

#include "nsClipboard.h"
#include "nsCOMPtr.h"
#include "nsITransferable.h"
#include "nsISupportsPrimitives.h"
#include "nsMemory.h"
#include "nsPrimitiveHelpers.h"
#include "nsString.h"
#include "nsXPIDLString.h"

static NSString *
PasteboardTypeForFlavor(const char *aFlavor)
{
  if (!strcmp(aFlavor, kUnicodeMime) || !strcmp(aFlavor, kTextMime))
    return NSStringPboardType;
  if (!strcmp(aFlavor, kHTMLMime))
    return NSHTMLPboardType;
  if (!strcmp(aFlavor, kURLDataMime))
    return @"CorePasteboardFlavorType 0x75726C20";
  if (!strcmp(aFlavor, kURLDescriptionMime))
    return @"CorePasteboardFlavorType 0x75726C64";
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
         !strcmp(aFlavor, kURLDataMime) ||
         !strcmp(aFlavor, kURLDescriptionMime);
}

nsClipboard::nsClipboard()
  : nsBaseClipboard(), mChangeCount(0)
{
}

nsClipboard::~nsClipboard()
{
}

NS_IMETHODIMP
nsClipboard::SetNativeClipboardData(PRInt32 aWhichClipboard)
{
  if (aWhichClipboard != kGlobalClipboard || !mTransferable)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupportsArray> flavors;
  nsresult rv = mTransferable->FlavorsTransferableCanExport(getter_AddRefs(flavors));
  if (NS_FAILED(rv))
    return rv;

  NSMutableArray *types = [NSMutableArray array];
  NSMutableDictionary *values = [NSMutableDictionary dictionary];
  PRUint32 count = 0;
  flavors->Count(&count);
  for (PRUint32 i = 0; i < count; ++i) {
    nsCOMPtr<nsISupports> flavorSupports;
    flavors->GetElementAt(i, getter_AddRefs(flavorSupports));
    nsCOMPtr<nsISupportsCString> flavorObject(do_QueryInterface(flavorSupports));
    if (!flavorObject)
      continue;

    nsXPIDLCString flavor;
    flavorObject->ToString(getter_Copies(flavor));
    NSString *type = PasteboardTypeForFlavor(flavor);
    if (!type)
      continue;

    nsCOMPtr<nsISupports> dataObject;
    PRUint32 dataLength = 0;
    rv = mTransferable->GetTransferData(flavor, getter_AddRefs(dataObject),
                                        &dataLength);
    if (NS_FAILED(rv) || !dataObject)
      continue;

    void *data = nsnull;
    nsPrimitiveHelpers::CreateDataFromPrimitive(flavor, dataObject, &data,
                                                dataLength);
    if (!data)
      continue;

    id value = nil;
    if (FlavorUsesUnicodeString(flavor)) {
      value = [NSString stringWithCharacters:(const unichar *)data
                                      length:dataLength / sizeof(PRUnichar)];
    } else {
      value = [NSData dataWithBytes:data length:dataLength];
    }
    nsMemory::Free(data);

    if (value) {
      [types addObject:type];
      [values setObject:value forKey:type];
    }
  }

  if (![types count])
    return NS_ERROR_FAILURE;

  mIgnoreEmptyNotification = PR_TRUE;
  NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
  [pasteboard declareTypes:types owner:nil];
  for (NSString *type in types) {
    id value = [values objectForKey:type];
    if ([value isKindOfClass:[NSString class]])
      [pasteboard setString:value forType:type];
    else
      [pasteboard setData:value forType:type];
  }
  mChangeCount = [pasteboard changeCount];
  mIgnoreEmptyNotification = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsClipboard::GetNativeClipboardData(nsITransferable *aTransferable,
                                    PRInt32 aWhichClipboard)
{
  if (aWhichClipboard != kGlobalClipboard || !aTransferable)
    return NS_ERROR_FAILURE;

  NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
  if (mChangeCount == [pasteboard changeCount] && mTransferable) {
    nsCOMPtr<nsISupportsArray> flavors;
    if (NS_SUCCEEDED(aTransferable->FlavorsTransferableCanImport(
                       getter_AddRefs(flavors)))) {
      PRUint32 count = 0;
      flavors->Count(&count);
      for (PRUint32 i = 0; i < count; ++i) {
        nsCOMPtr<nsISupports> flavorSupports;
        flavors->GetElementAt(i, getter_AddRefs(flavorSupports));
        nsCOMPtr<nsISupportsCString> flavorObject(do_QueryInterface(flavorSupports));
        if (!flavorObject)
          continue;
        nsXPIDLCString flavor;
        flavorObject->ToString(getter_Copies(flavor));
        nsCOMPtr<nsISupports> data;
        PRUint32 length = 0;
        if (NS_SUCCEEDED(mTransferable->GetTransferData(
                           flavor, getter_AddRefs(data), &length))) {
          aTransferable->SetTransferData(flavor, data, length);
          return NS_OK;
        }
      }
    }
  }

  nsCOMPtr<nsISupportsArray> flavors;
  nsresult rv = aTransferable->FlavorsTransferableCanImport(getter_AddRefs(flavors));
  if (NS_FAILED(rv))
    return rv;

  PRUint32 count = 0;
  flavors->Count(&count);
  for (PRUint32 i = 0; i < count; ++i) {
    nsCOMPtr<nsISupports> flavorSupports;
    flavors->GetElementAt(i, getter_AddRefs(flavorSupports));
    nsCOMPtr<nsISupportsCString> flavorObject(do_QueryInterface(flavorSupports));
    if (!flavorObject)
      continue;

    nsXPIDLCString flavor;
    flavorObject->ToString(getter_Copies(flavor));
    NSString *type = PasteboardTypeForFlavor(flavor);
    if (!type || ![pasteboard availableTypeFromArray:
                    [NSArray arrayWithObject:type]])
      continue;

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
      nsPrimitiveHelpers::CreatePrimitiveForData(kUnicodeMime, characters,
                                                  byteLength,
                                                  getter_AddRefs(primitive));
      nsMemory::Free(characters);
      if (primitive) {
        aTransferable->SetTransferData(flavor, primitive, byteLength);
        return NS_OK;
      }
    } else {
      NSData *value = [pasteboard dataForType:type];
      if (!value)
        continue;
      NSUInteger nativeLength = [value length];
      if (nativeLength > PR_UINT32_MAX)
        continue;
      PRUint32 byteLength = (PRUint32)nativeLength;
      nsPrimitiveHelpers::CreatePrimitiveForData(flavor,
                                                  (void *)[value bytes],
                                                  byteLength,
                                                  getter_AddRefs(primitive));
      if (primitive) {
        aTransferable->SetTransferData(flavor, primitive, byteLength);
        return NS_OK;
      }
    }
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsClipboard::HasDataMatchingFlavors(nsISupportsArray *aFlavorList,
                                    PRInt32 aWhichClipboard,
                                    PRBool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_FALSE;
  if (aWhichClipboard != kGlobalClipboard || !aFlavorList)
    return NS_OK;

  NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
  PRUint32 count = 0;
  aFlavorList->Count(&count);
  for (PRUint32 i = 0; i < count; ++i) {
    nsCOMPtr<nsISupports> flavorSupports;
    aFlavorList->GetElementAt(i, getter_AddRefs(flavorSupports));
    nsCOMPtr<nsISupportsCString> flavorObject(do_QueryInterface(flavorSupports));
    if (!flavorObject)
      continue;
    nsXPIDLCString flavor;
    flavorObject->ToString(getter_Copies(flavor));
    NSString *type = PasteboardTypeForFlavor(flavor);
    if (type && [pasteboard availableTypeFromArray:
                  [NSArray arrayWithObject:type]]) {
      *aResult = PR_TRUE;
      break;
    }
  }
  return NS_OK;
}
