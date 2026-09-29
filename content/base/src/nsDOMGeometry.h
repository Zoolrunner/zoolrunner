/* CSSOM View geometry. License: MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef nsDOMGeometry_h___
#define nsDOMGeometry_h___
#include "nsIDOMDOMRect.h"
#include "nsCOMArray.h"
#include "nsRect.h"
class nsIContent;

class nsDOMGeometryRect : public nsIDOMDOMRect {
public:
  nsDOMGeometryRect(double aX = 0, double aY = 0,
                    double aWidth = 0, double aHeight = 0)
    : mX(aX), mY(aY), mWidth(aWidth), mHeight(aHeight) {}
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMRECT
private:
  double mX, mY, mWidth, mHeight;
};

class nsDOMGeometryRectList : public nsIDOMDOMRectList {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMDOMRECTLIST
  nsresult Append(const nsRect& aRect, double aScale);
  nsresult BoundingRect(nsIDOMDOMRect** aResult);
private:
  nsCOMArray<nsIDOMDOMRect> mRects;
};

nsresult NS_GetElementClientRects(nsIContent* aContent,
                                  nsDOMGeometryRectList** aResult);
#endif
