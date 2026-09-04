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
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"), in
 * which case the provisions of the GPL or the LGPL are applicable instead of
 * those above.
 *
 * ***** END LICENSE BLOCK ***** */

#import <Cocoa/Cocoa.h>

#include "nsScreenManagerCocoa.h"
#include "nsIScreen.h"

class nsScreenCocoa : public nsIScreen
{
public:
  nsScreenCocoa(NSScreen* aScreen);
  virtual ~nsScreenCocoa();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREEN

private:
  void GetGeckoRect(NSRect aRect, PRInt32* aLeft, PRInt32* aTop,
                    PRInt32* aWidth, PRInt32* aHeight);

  NSScreen* mScreen;
};

NS_IMPL_ISUPPORTS1(nsScreenCocoa, nsIScreen)

nsScreenCocoa::nsScreenCocoa(NSScreen* aScreen)
  : mScreen([aScreen retain])
{
}

nsScreenCocoa::~nsScreenCocoa()
{
  [mScreen release];
}

void
nsScreenCocoa::GetGeckoRect(NSRect aRect, PRInt32* aLeft, PRInt32* aTop,
                            PRInt32* aWidth, PRInt32* aHeight)
{
  NSArray* screens = [NSScreen screens];
  NSRect primaryFrame = [screens count] ? [[screens objectAtIndex:0] frame]
                                        : NSZeroRect;

  *aLeft = (PRInt32)NSMinX(aRect);
  *aTop = (PRInt32)(NSMaxY(primaryFrame) - NSMaxY(aRect));
  *aWidth = (PRInt32)NSWidth(aRect);
  *aHeight = (PRInt32)NSHeight(aRect);
}

NS_IMETHODIMP
nsScreenCocoa::GetRect(PRInt32* aLeft, PRInt32* aTop,
                       PRInt32* aWidth, PRInt32* aHeight)
{
  GetGeckoRect([mScreen frame], aLeft, aTop, aWidth, aHeight);
  return NS_OK;
}

NS_IMETHODIMP
nsScreenCocoa::GetAvailRect(PRInt32* aLeft, PRInt32* aTop,
                            PRInt32* aWidth, PRInt32* aHeight)
{
  GetGeckoRect([mScreen visibleFrame], aLeft, aTop, aWidth, aHeight);
  return NS_OK;
}

NS_IMETHODIMP
nsScreenCocoa::GetPixelDepth(PRInt32* aPixelDepth)
{
  *aPixelDepth = (PRInt32)NSBitsPerPixelFromDepth([mScreen depth]);
  return NS_OK;
}

NS_IMETHODIMP
nsScreenCocoa::GetColorDepth(PRInt32* aColorDepth)
{
  *aColorDepth = (PRInt32)NSBitsPerPixelFromDepth([mScreen depth]);
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(nsScreenManagerCocoa, nsIScreenManager)

nsScreenManagerCocoa::nsScreenManagerCocoa()
{
}

nsScreenManagerCocoa::~nsScreenManagerCocoa()
{
}

static nsIScreen*
CreateScreen(NSScreen* aScreen)
{
  nsIScreen* screen = new nsScreenCocoa(aScreen);
  NS_IF_ADDREF(screen);
  return screen;
}

NS_IMETHODIMP
nsScreenManagerCocoa::ScreenForRect(PRInt32 aLeft, PRInt32 aTop,
                                    PRInt32 aWidth, PRInt32 aHeight,
                                    nsIScreen** aScreen)
{
  NS_ENSURE_ARG_POINTER(aScreen);
  *aScreen = nsnull;

  NSArray* screens = [NSScreen screens];
  if (![screens count])
    return NS_ERROR_FAILURE;

  NSRect primaryFrame = [[screens objectAtIndex:0] frame];
  NSRect requested = NSMakeRect(aLeft,
                                NSMaxY(primaryFrame) - aTop - aHeight,
                                aWidth, aHeight);
  NSScreen* bestScreen = [screens objectAtIndex:0];
  double greatestArea = -1.0;
  PRUint32 screenCount = (PRUint32)[screens count];

  PRUint32 i;
  for (i = 0; i < screenCount; ++i) {
    NSScreen* screen = [screens objectAtIndex:i];
    NSRect intersection = NSIntersectionRect(requested, [screen frame]);
    double area = NSWidth(intersection) * NSHeight(intersection);
    if (area > greatestArea) {
      greatestArea = area;
      bestScreen = screen;
    }
  }

  *aScreen = CreateScreen(bestScreen);
  return *aScreen ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsScreenManagerCocoa::GetPrimaryScreen(nsIScreen** aScreen)
{
  NS_ENSURE_ARG_POINTER(aScreen);
  NSArray* screens = [NSScreen screens];
  *aScreen = [screens count] ? CreateScreen([screens objectAtIndex:0]) : nsnull;
  return *aScreen ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsScreenManagerCocoa::GetNumberOfScreens(PRUint32* aNumberOfScreens)
{
  NS_ENSURE_ARG_POINTER(aNumberOfScreens);
  *aNumberOfScreens = (PRUint32)[[NSScreen screens] count];
  return NS_OK;
}
