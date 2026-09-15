/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Patrick C. Beard <beard@netscape.com>
 *   Simon Fraser     <sfraser@netscape.com>
 *   Conrad Carlen    <ccarlen@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsDeviceContextSpecX.h"

#include "prmem.h"
#include "plstr.h"
#include "nsCRT.h"

#include "nsIServiceManager.h"
#include "nsIPrintOptions.h"
#include "nsIPrintSettingsX.h"

/** -------------------------------------------------------
 *  Construct the nsDeviceContextSpecX
 *  @update   dc 12/02/98
 */
nsDeviceContextSpecX::nsDeviceContextSpecX()
: mPrintSession(0)
, mPageFormat(kPMNoPageFormat)
, mPrintSettings(kPMNoPrintSettings)
#ifndef __LP64__
, mSavedPort(0)
#endif
, mBeganPrinting(PR_FALSE)
{
}

/** -------------------------------------------------------
 *  Destroy the nsDeviceContextSpecX
 *  @update   dc 12/02/98
 */
nsDeviceContextSpecX::~nsDeviceContextSpecX()
{
  ClosePrintManager();
}

NS_IMPL_ISUPPORTS2(nsDeviceContextSpecX, nsIDeviceContextSpec, nsIPrintingContext)

/** -------------------------------------------------------
 *  Initialize the nsDeviceContextSpecMac
 *  @update   dc 12/02/98
 */
NS_IMETHODIMP nsDeviceContextSpecX::Init(nsIPrintSettings* aPS, PRBool	aIsPrintPreview)
{
  nsresult rv;
    
  nsCOMPtr<nsIPrintSettingsX> printSettingsX(do_QueryInterface(aPS));
  if (!printSettingsX)
    return NS_ERROR_NO_INTERFACE;
  
  rv = printSettingsX->GetNativePrintSession(&mPrintSession);
  if (NS_FAILED(rv))
    return rv;  
  rv = printSettingsX->GetPMPageFormat(&mPageFormat);
  if (NS_FAILED(rv))
    return rv;
  rv = printSettingsX->GetPMPrintSettings(&mPrintSettings);
  if (NS_FAILED(rv))
    return rv;

  return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::PrintManagerOpen(PRBool* aIsOpen)
{
    *aIsOpen = mBeganPrinting;
    return NS_OK;
}

/** -------------------------------------------------------
 * Closes the printmanager if it is open.
 * @update   dc 12/03/98
 */
NS_IMETHODIMP nsDeviceContextSpecX::ClosePrintManager()
{
	return NS_OK;
}  

NS_IMETHODIMP nsDeviceContextSpecX::BeginDocument(PRUnichar*  aTitle, 
                                                  PRUnichar*  aPrintToFileName,
                                                  PRInt32     aStartPage, 
                                                  PRInt32     aEndPage)
{
    if (aTitle) {
      CFStringRef cfString = ::CFStringCreateWithCharacters(NULL, aTitle, nsCRT::strlen(aTitle));
      if (cfString) {
#ifdef __LP64__
        ::PMPrintSettingsSetJobName(mPrintSettings, cfString);
#else
        ::PMSetJobNameCFString(mPrintSettings, cfString);
#endif
        ::CFRelease(cfString);
      }
    }

    OSStatus status;
    status = ::PMSetFirstPage(mPrintSettings, aStartPage, false);
    NS_ASSERTION(status == noErr, "PMSetFirstPage failed");
    status = ::PMSetLastPage(mPrintSettings, aEndPage, false);
    NS_ASSERTION(status == noErr, "PMSetLastPage failed");

#if defined(__LP64__) || defined(MOZ_ENABLE_CAIRO_GFX)
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1040
    status = ::PMSessionBeginCGDocumentNoDialog(mPrintSession, mPrintSettings,
                                                mPageFormat);
#else
    const void *contextType = kPMGraphicsContextCoreGraphics;
    CFArrayRef contextTypes = ::CFArrayCreate(NULL, &contextType, 1,
                                              &kCFTypeArrayCallBacks);
    if (!contextTypes)
      return NS_ERROR_OUT_OF_MEMORY;
    status = ::PMSessionSetDocumentFormatGeneration(mPrintSession,
                 kPMDocumentFormatPDF, contextTypes, NULL);
    ::CFRelease(contextTypes);
    if (status == noErr)
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1020
      status = ::PMSessionBeginDocument(mPrintSession, mPrintSettings,
                                         mPageFormat);
#else
      status = ::PMSessionBeginDocumentNoDialog(mPrintSession, mPrintSettings,
                                                 mPageFormat);
#endif
#endif
#else
    status = ::PMSessionBeginDocument(mPrintSession, mPrintSettings, mPageFormat);
#endif
    if (status != noErr) return NS_ERROR_ABORT;

    mBeganPrinting = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::EndDocument()
{
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1020 && (defined(__LP64__) || defined(MOZ_ENABLE_CAIRO_GFX))
    OSStatus status = ::PMSessionEndDocumentNoDialog(mPrintSession);
#else
    OSStatus status = ::PMSessionEndDocument(mPrintSession);
#endif
    mBeganPrinting = PR_FALSE;
    return status == noErr ? NS_OK : NS_ERROR_ABORT;
}

NS_IMETHODIMP nsDeviceContextSpecX::AbortDocument()
{
    return EndDocument();
}

NS_IMETHODIMP nsDeviceContextSpecX::BeginPage()
{
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1020 && (defined(__LP64__) || defined(MOZ_ENABLE_CAIRO_GFX))
    OSStatus status = ::PMSessionBeginPageNoDialog(mPrintSession, mPageFormat, NULL);
#else
    OSStatus status = ::PMSessionBeginPage(mPrintSession, mPageFormat, NULL);
#endif
    if (status != noErr) return NS_ERROR_ABORT;

#if !defined(__LP64__) && !defined(MOZ_ENABLE_CAIRO_GFX)
    ::GetPort(&mSavedPort);
    void *graphicsContext;
    status = ::PMSessionGetGraphicsContext(mPrintSession, kPMGraphicsContextQuickdraw, &graphicsContext);
    if (status != noErr)
      return NS_ERROR_ABORT;
    ::SetPort((CGrafPtr)graphicsContext);
#endif
    return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::EndPage()
{
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1020 && (defined(__LP64__) || defined(MOZ_ENABLE_CAIRO_GFX))
    OSStatus status = ::PMSessionEndPageNoDialog(mPrintSession);
#else
    OSStatus status = ::PMSessionEndPage(mPrintSession);
#endif
#if !defined(__LP64__) && !defined(MOZ_ENABLE_CAIRO_GFX)
    if (mSavedPort)
    {
        ::SetPort(mSavedPort);
        mSavedPort = 0;
    }
#endif
    if (status != noErr)
      return NS_ERROR_ABORT;
    return NS_OK;
}

#if defined(__LP64__) || defined(MOZ_ENABLE_CAIRO_GFX)
CGContextRef nsDeviceContextSpecX::GetCGContext()
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1040
    CGContextRef context = NULL;
    if (::PMSessionGetCGGraphicsContext(mPrintSession, &context) != noErr)
      return NULL;
    return context;
#else
    void *context = NULL;
    if (::PMSessionGetGraphicsContext(mPrintSession,
           kPMGraphicsContextCoreGraphics, &context) != noErr)
      return NULL;
    return static_cast<CGContextRef>(context);
#endif
}
#endif

NS_IMETHODIMP nsDeviceContextSpecX::GetPrinterResolution(double* aResolution)
{
    PMPrinter printer;
    OSStatus status = ::PMSessionGetCurrentPrinter(mPrintSession, &printer);
    if (status != noErr)
      return NS_ERROR_FAILURE;
      
    PMResolution defaultResolution;
#ifdef __LP64__
    status = ::PMPrinterGetOutputResolution(printer, mPrintSettings,
                                            &defaultResolution);
#else
    status = ::PMPrinterGetPrinterResolution(printer, kPMDefaultResolution, &defaultResolution);
#endif
    if (status != noErr)
      return NS_ERROR_FAILURE;
    
    *aResolution = defaultResolution.hRes;
    return NS_OK;
}

NS_IMETHODIMP nsDeviceContextSpecX::GetPageRect(double* aTop, double* aLeft, double* aBottom, double* aRight)
{
    PMRect pageRect;
    ::PMGetAdjustedPageRect(mPageFormat, &pageRect);
    *aTop = pageRect.top, *aLeft = pageRect.left;
    *aBottom = pageRect.bottom, *aRight = pageRect.right;
    return NS_OK;
}
