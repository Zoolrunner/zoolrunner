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
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

/*
 * CoreFoundation implementation used by 64-bit macOS, where the old Script
 * Manager date and time APIs are unavailable.  This follows the implementation
 * used by later Mozilla releases while retaining the Mozilla 1.8 interface.
 */

#include "nsIServiceManager.h"
#include "nsDateTimeFormatMac.h"
#include <CoreFoundation/CoreFoundation.h>
#include "nsLocaleCID.h"
#include "nsILocaleService.h"
#include "nsUnicharUtils.h"
#include "nsAutoPtr.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDateTimeFormatMac, nsIDateTimeFormat)

nsresult
nsDateTimeFormatMac::Initialize(nsILocale* locale)
{
  nsAutoString localeStr;
  nsAutoString category(NS_LITERAL_STRING("NSILOCALE_TIME"));
  nsresult res = NS_OK;

  if (nsnull == locale) {
    if (!mLocale.IsEmpty() &&
        mLocale.Equals(mAppLocale, nsCaseInsensitiveStringComparator())) {
      return NS_OK;
    }
  } else {
    res = locale->GetCategory(category, localeStr);
    if (NS_SUCCEEDED(res) && !localeStr.IsEmpty() && !mLocale.IsEmpty() &&
        mLocale.Equals(localeStr, nsCaseInsensitiveStringComparator())) {
      return NS_OK;
    }
  }

  nsCOMPtr<nsILocaleService> localeService =
    do_GetService(NS_LOCALESERVICE_CONTRACTID, &res);
  if (NS_SUCCEEDED(res)) {
    nsCOMPtr<nsILocale> appLocale;
    res = localeService->GetApplicationLocale(getter_AddRefs(appLocale));
    if (NS_SUCCEEDED(res)) {
      res = appLocale->GetCategory(category, localeStr);
      if (NS_SUCCEEDED(res) && !localeStr.IsEmpty()) {
        mAppLocale = localeStr;
      }
    }
  }

  if (nsnull == locale) {
    mUseDefaultLocale = true;
  } else {
    mUseDefaultLocale = false;
    res = locale->GetCategory(category, localeStr);
  }

  if (NS_SUCCEEDED(res) && !localeStr.IsEmpty()) {
    mLocale.Assign(localeStr);
  }

  return res;
}

nsresult
nsDateTimeFormatMac::FormatTime(nsILocale* locale,
                                const nsDateFormatSelector dateFormatSelector,
                                const nsTimeFormatSelector timeFormatSelector,
                                const time_t timetTime,
                                nsString& stringOut)
{
  struct tm tmTime;
  struct tm* localTime = localtime_r(&timetTime, &tmTime);
  if (!localTime) {
    stringOut.Truncate();
    return NS_ERROR_FAILURE;
  }
  return FormatTMTime(locale, dateFormatSelector, timeFormatSelector,
                      localTime, stringOut);
}

nsresult
nsDateTimeFormatMac::FormatTMTime(nsILocale* locale,
                                  const nsDateFormatSelector dateFormatSelector,
                                  const nsTimeFormatSelector timeFormatSelector,
                                  const struct tm* tmTime,
                                  nsString& stringOut)
{
  nsresult res = Initialize(locale);
  if (NS_FAILED(res)) {
    return res;
  }

  if (dateFormatSelector == kDateFormatNone &&
      timeFormatSelector == kTimeFormatNone) {
    stringOut.Truncate();
    return NS_OK;
  }

  if (!tmTime) {
    return NS_ERROR_INVALID_ARG;
  }

  CFLocaleRef formatterLocale = nsnull;
  if (!locale) {
    formatterLocale = CFLocaleCopyCurrent();
  } else {
    CFStringRef localeString =
      CFStringCreateWithCharacters(nsnull, mLocale.get(), mLocale.Length());
    if (localeString) {
      formatterLocale = CFLocaleCreate(nsnull, localeString);
      CFRelease(localeString);
    }
  }
  if (!formatterLocale) {
    return NS_ERROR_FAILURE;
  }

  CFDateFormatterStyle dateStyle = kCFDateFormatterNoStyle;
  switch (dateFormatSelector) {
    case kDateFormatLong:
      dateStyle = kCFDateFormatterLongStyle;
      break;
    case kDateFormatShort:
      dateStyle = kCFDateFormatterShortStyle;
      break;
    case kDateFormatYearMonth:
    case kDateFormatWeekday:
    case kDateFormatNone:
      break;
    default:
      CFRelease(formatterLocale);
      return NS_ERROR_INVALID_ARG;
  }

  CFDateFormatterStyle timeStyle = kCFDateFormatterNoStyle;
  switch (timeFormatSelector) {
    case kTimeFormatSeconds:
    case kTimeFormatSecondsForce24Hour:
      timeStyle = kCFDateFormatterMediumStyle;
      break;
    case kTimeFormatNoSeconds:
    case kTimeFormatNoSecondsForce24Hour:
      timeStyle = kCFDateFormatterShortStyle;
      break;
    case kTimeFormatNone:
      break;
    default:
      CFRelease(formatterLocale);
      return NS_ERROR_INVALID_ARG;
  }

  CFDateFormatterRef formatter =
    CFDateFormatterCreate(nsnull, formatterLocale, dateStyle, timeStyle);
  CFRelease(formatterLocale);
  if (!formatter) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (dateFormatSelector == kDateFormatYearMonth ||
      dateFormatSelector == kDateFormatWeekday) {
    CFStringRef prefix = dateFormatSelector == kDateFormatYearMonth
                         ? CFSTR("yyyy/MM ") : CFSTR("EEE ");
    CFStringRef oldFormat = CFDateFormatterGetFormat(formatter);
    CFMutableStringRef newFormat =
      CFStringCreateMutableCopy(nsnull, 0, oldFormat);
    if (!newFormat) {
      CFRelease(formatter);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    CFStringInsert(newFormat, 0, prefix);
    CFDateFormatterSetFormat(formatter, newFormat);
    CFRelease(newFormat);
  }

  if (timeFormatSelector == kTimeFormatSecondsForce24Hour ||
      timeFormatSelector == kTimeFormatNoSecondsForce24Hour) {
    CFStringRef oldFormat = CFDateFormatterGetFormat(formatter);
    CFMutableStringRef newFormat =
      CFStringCreateMutableCopy(nsnull, 0, oldFormat);
    if (!newFormat) {
      CFRelease(formatter);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    CFRange range = CFRangeMake(0, CFStringGetLength(newFormat));
    CFStringFindAndReplace(newFormat, CFSTR("h"), CFSTR("H"), range, 0);
    range.length = CFStringGetLength(newFormat);
    CFStringFindAndReplace(newFormat, CFSTR("a"), CFSTR(""), range, 0);
    CFDateFormatterSetFormat(formatter, newFormat);
    CFRelease(newFormat);
  }

  CFGregorianDate date;
  date.second = tmTime->tm_sec;
  date.minute = (SInt8)tmTime->tm_min;
  date.hour = (SInt8)tmTime->tm_hour;
  date.day = (SInt8)tmTime->tm_mday;
  date.month = (SInt8)(tmTime->tm_mon + 1);
  date.year = tmTime->tm_year + 1900;

  CFTimeZoneRef timeZone = CFTimeZoneCopySystem();
  if (!timeZone) {
    CFRelease(formatter);
    return NS_ERROR_FAILURE;
  }
  CFAbsoluteTime absoluteTime =
    CFGregorianDateGetAbsoluteTime(date, timeZone);
  CFRelease(timeZone);

  CFStringRef formatted =
    CFDateFormatterCreateStringWithAbsoluteTime(nsnull, formatter,
                                                absoluteTime);
  CFRelease(formatter);
  if (!formatted) {
    return NS_ERROR_FAILURE;
  }

  CFIndex length = CFStringGetLength(formatted);
  if (length < 0 || (PRUint64)length > PR_UINT32_MAX) {
    CFRelease(formatted);
    return NS_ERROR_FAILURE;
  }
  nsAutoArrayPtr<UniChar> buffer(new UniChar[length + 1]);
  if (!buffer) {
    CFRelease(formatted);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  CFStringGetCharacters(formatted, CFRangeMake(0, length), buffer.get());
  stringOut.Assign(buffer.get(), (PRUint32)length);
  CFRelease(formatted);
  return NS_OK;
}

nsresult
nsDateTimeFormatMac::FormatPRTime(nsILocale* locale,
                                  const nsDateFormatSelector dateFormatSelector,
                                  const nsTimeFormatSelector timeFormatSelector,
                                  const PRTime prTime,
                                  nsString& stringOut)
{
  PRExplodedTime explodedTime;
  PR_ExplodeTime(prTime, PR_LocalTimeParameters, &explodedTime);
  return FormatPRExplodedTime(locale, dateFormatSelector, timeFormatSelector,
                              &explodedTime, stringOut);
}

nsresult
nsDateTimeFormatMac::FormatPRExplodedTime(nsILocale* locale,
                                          const nsDateFormatSelector dateFormatSelector,
                                          const nsTimeFormatSelector timeFormatSelector,
                                          const PRExplodedTime* explodedTime,
                                          nsString& stringOut)
{
  if (!explodedTime) {
    return NS_ERROR_INVALID_ARG;
  }

  struct tm tmTime;
  memset(&tmTime, 0, sizeof(tmTime));
  tmTime.tm_yday = explodedTime->tm_yday;
  tmTime.tm_wday = explodedTime->tm_wday;
  tmTime.tm_year = explodedTime->tm_year - 1900;
  tmTime.tm_mon = explodedTime->tm_month;
  tmTime.tm_mday = explodedTime->tm_mday;
  tmTime.tm_hour = explodedTime->tm_hour;
  tmTime.tm_min = explodedTime->tm_min;
  tmTime.tm_sec = explodedTime->tm_sec;

  return FormatTMTime(locale, dateFormatSelector, timeFormatSelector,
                      &tmTime, stringOut);
}
