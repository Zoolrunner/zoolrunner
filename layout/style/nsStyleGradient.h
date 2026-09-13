/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This file is available under the MPL 1.1/GPL 2.0/LGPL 2.1 tri-license.
 * See https://www.mozilla.org/MPL/1.1/ for the MPL terms.
 */
#ifndef nsStyleGradient_h_
#define nsStyleGradient_h_

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsColor.h"
#include "nsSize.h"
#include "nsStyleCoord.h"
#include "nsString.h"
#include "nsTArray.h"
class imgIContainer;

struct nsStyleGradientStop {
  nscolor mColor;
  nsStyleCoord mPosition; // null means an omitted stop position
};

// Computed stops are immutable; the single raster cache is an implementation
// detail and may be replaced when this gradient is painted at another size.
class nsStyleGradient : public nsISupports {
public:
  NS_DECL_ISUPPORTS
  nsStyleGradient();
  double mAngle; // radians, clockwise from upward
  PRInt32 mDirection; // top=1, bottom=2, left=4, right=8; zero uses mAngle
  nsTArray<nsStyleGradientStop> mStops;
  imgIContainer* GetImage(const nsSize& aSize, float aPixelSize);
  void ToString(nsAString& aResult, float aTwipsToPixels) const;
private:
  ~nsStyleGradient();
  nsSize mCachedSize;
  float mCachedPixelSize;
  nsCOMPtr<imgIContainer> mCachedImage;
};
#endif
