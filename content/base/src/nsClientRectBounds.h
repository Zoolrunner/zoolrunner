/* CSSOM View's bounding rectangle algorithm.
 * License: MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef nsClientRectBounds_h__
#define nsClientRectBounds_h__

// Layout fragments have nonnegative dimensions. Keep the first fragment for
// the all-empty case, independently of the union of nonzero-size fragments.
class nsClientRectBounds {
public:
  struct Rect {
    double x, y, width, height;
    Rect(double aX=0, double aY=0, double aWidth=0, double aHeight=0)
      : x(aX), y(aY), width(aWidth), height(aHeight) {}
  };
  nsClientRectBounds() : mHaveFirst(false), mHaveExtent(false), mHaveArea(false),
                         mLeft(0), mTop(0), mRight(0), mBottom(0) {}
  void Add(double x, double y, double width, double height) {
    if (!mHaveFirst) {
      mFirst=Rect(x,y,width,height);
      mHaveFirst=true;
    }
    if (width==0 && height==0)
      return;
    if (!mHaveExtent) {
      mLeft=x; mTop=y; mRight=x+width; mBottom=y+height;
      mHaveExtent=true;
    } else {
      if (x<mLeft) mLeft=x;
      if (y<mTop) mTop=y;
      if (x+width>mRight) mRight=x+width;
      if (y+height>mBottom) mBottom=y+height;
    }
    if (width!=0 && height!=0)
      mHaveArea=true;
  }
  Rect Get() const {
    return mHaveArea ? Rect(mLeft,mTop,mRight-mLeft,mBottom-mTop) : mFirst;
  }
private:
  Rect mFirst;
  bool mHaveFirst, mHaveExtent, mHaveArea;
  double mLeft, mTop, mRight, mBottom;
};
#endif
