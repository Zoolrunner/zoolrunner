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
 *   Roland Mainz <roland.mainz@informatik.med.uni-giessen.de>
 *   Leon Sha <leon.sha@sun.com>
 *   Boris Zbarsky <bzbarsky@mit.edu>
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

/* Optional opacity groups; keeps the historical rendering-context ABI intact. */
#ifndef nsIRenderingContextFilter_h___
#define nsIRenderingContextFilter_h___

#include "nsISupports.h"
#include "nsRect.h"

#define NS_IRENDERINGCONTEXTFILTER_IID \
{ 0x52f8583b, 0x3583, 0x4e56, \
  { 0xab, 0x21, 0x6d, 0x02, 0x87, 0x17, 0x3d, 0xaf } }

class nsIRenderingContextFilter : public nsISupports
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IRENDERINGCONTEXTFILTER_IID)

  /* Bounds use the rendering context's current coordinate system. A failed
   * push must leave the context unchanged and must not be followed by a pop. */
  NS_IMETHOD PushFilter(const nsRect& aRect, PRBool aAreaIsOpaque,
                        float aOpacity) = 0;
  NS_IMETHOD PopFilter() = 0;
};

#endif
