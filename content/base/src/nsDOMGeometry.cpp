/* CSSOM View geometry. License: MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "nsDOMGeometry.h"
#include "nsClientRectBounds.h"
#include "nsContentUtils.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIFrame.h"
#include "nsLayoutAtoms.h"
#include "nsStyleStruct.h"
#include "nsStyleContext.h"
#include "nsCSSAnonBoxes.h"
#include "nsAutoPtr.h"
#include "nsVoidArray.h"

NS_INTERFACE_MAP_BEGIN(nsDOMGeometryRect)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMRect)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(DOMRect)
NS_INTERFACE_MAP_END
NS_IMPL_ADDREF(nsDOMGeometryRect)
NS_IMPL_RELEASE(nsDOMGeometryRect)

#define RECT_ATTRIBUTE(Name, Member) \
  NS_IMETHODIMP nsDOMGeometryRect::Get##Name(double* aValue) \
  { *aValue = Member; return NS_OK; } \
  NS_IMETHODIMP nsDOMGeometryRect::Set##Name(double aValue) \
  { Member = aValue; return NS_OK; }
RECT_ATTRIBUTE(X, mX)
RECT_ATTRIBUTE(Y, mY)
RECT_ATTRIBUTE(Width, mWidth)
RECT_ATTRIBUTE(Height, mHeight)
#undef RECT_ATTRIBUTE
static double RectMin(double a, double b)
{
  if (a != a || b != b) return a + b;
  if (a == 0 && b == 0)
    return (1.0 / a < 0 || 1.0 / b < 0) ? -0.0 : 0.0;
  return a < b ? a : b;
}
static double RectMax(double a, double b)
{
  if (a != a || b != b) return a + b;
  if (a == 0 && b == 0)
    return (1.0 / a > 0 || 1.0 / b > 0) ? 0.0 : -0.0;
  return a > b ? a : b;
}
NS_IMETHODIMP nsDOMGeometryRect::GetTop(double* aValue)
{ *aValue = RectMin(mY, mY + mHeight); return NS_OK; }
NS_IMETHODIMP nsDOMGeometryRect::GetLeft(double* aValue)
{ *aValue = RectMin(mX, mX + mWidth); return NS_OK; }
NS_IMETHODIMP nsDOMGeometryRect::GetRight(double* aValue)
{ *aValue = RectMax(mX, mX + mWidth); return NS_OK; }
NS_IMETHODIMP nsDOMGeometryRect::GetBottom(double* aValue)
{ *aValue = RectMax(mY, mY + mHeight); return NS_OK; }

NS_INTERFACE_MAP_BEGIN(nsDOMGeometryRectList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDOMRectList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(DOMRectList)
NS_INTERFACE_MAP_END
NS_IMPL_ADDREF(nsDOMGeometryRectList)
NS_IMPL_RELEASE(nsDOMGeometryRectList)
NS_IMETHODIMP nsDOMGeometryRectList::GetLength(PRUint32* aLength)
{ *aLength = mRects.Count(); return NS_OK; }
NS_IMETHODIMP nsDOMGeometryRectList::Item(PRUint32 aIndex, nsIDOMDOMRect** aResult)
{
  *aResult = aIndex < PRUint32(mRects.Count()) ? mRects[aIndex] : nsnull;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

nsresult nsDOMGeometryRectList::Append(const nsRect& aRect, double aScale)
{
  nsRefPtr<nsDOMGeometryRect> rect = new nsDOMGeometryRect(
    aRect.x * aScale, aRect.y * aScale,
    aRect.width * aScale, aRect.height * aScale);
  NS_ENSURE_TRUE(rect && mRects.AppendObject(rect), NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

nsresult nsDOMGeometryRectList::BoundingRect(nsIDOMDOMRect** aResult)
{
  nsClientRectBounds bounds;
  for (PRInt32 i = 0; i < mRects.Count(); ++i) {
    double x, y, width, height;
    mRects[i]->GetX(&x); mRects[i]->GetY(&y);
    mRects[i]->GetWidth(&width); mRects[i]->GetHeight(&height);
    bounds.Add(x, y, width, height);
  }
  nsClientRectBounds::Rect rect = bounds.Get();
  *aResult = new nsDOMGeometryRect(rect.x, rect.y, rect.width, rect.height);
  NS_ENSURE_TRUE(*aResult, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aResult);
  return NS_OK;
}

static nsresult
AppendFrameBox(nsDOMGeometryRectList* aRects, nsIFrame* aFrame,
                nsIFrame* aViewport, double aScale)
{
  nsIAtom* pseudo = aFrame->GetStyleContext()->GetPseudoType();
  if (pseudo == nsCSSAnonBoxes::mozAnonymousBlock ||
      pseudo == nsCSSAnonBoxes::mozAnonymousPositionedBlock) {
    for (nsIFrame* child = aFrame->GetFirstChild(nsnull); child;
         child = child->GetNextSibling()) {
      nsresult rv = AppendFrameBox(aRects, child, aViewport, aScale);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
  }
  nsRect rect(nsPoint(0, 0), aFrame->GetSize());
  rect += aFrame->GetOffsetTo(aViewport);
  return aRects->Append(rect, aScale);
}

nsresult
NS_GetElementClientRects(nsIContent* aContent, nsDOMGeometryRectList** aResult)
{
  *aResult = nsnull;
  nsRefPtr<nsDOMGeometryRectList> rects = new nsDOMGeometryRectList();
  NS_ENSURE_TRUE(rects, NS_ERROR_OUT_OF_MEMORY);
  nsCOMPtr<nsIDocument> document = aContent->GetCurrentDoc();
  if (document) {
    // Reflow can run script and destroy frames. Acquire all frame pointers only
    // after flushing, and verify that the element still belongs to this doc.
    document->FlushPendingNotifications(Flush_Layout);
    nsIPresShell* shell = document->GetShellAt(0);
    if (shell && aContent->GetCurrentDoc() == document) {
      nsIFrame* frame = nsnull;
      shell->GetPrimaryFrameFor(aContent, &frame);
      nsIFrame* viewport = shell->GetRootFrame();
      double scale = 1.0 / shell->GetPresContext()->PixelsToTwips();
      while (frame && viewport) {
        nsresult rv;
        if (frame->GetType() == nsLayoutAtoms::tableOuterFrame) {
          // The anonymous wrapper includes space between caption and table;
          // CSSOM asks for the actual boxes, not that wrapper.
          nsIFrame* table = frame->GetFirstChild(nsnull);
          if (table) {
            rv = AppendFrameBox(rects, table, viewport, scale);
            NS_ENSURE_SUCCESS(rv, rv);
          }
          for (nsIFrame* caption = frame->GetFirstChild(nsLayoutAtoms::captionList);
               caption; caption = caption->GetNextSibling()) {
            rv = AppendFrameBox(rects, caption, viewport, scale);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        } else {
          rv = AppendFrameBox(rects, frame, viewport, scale);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        nsIFrame* next = frame->GetNextInFlow();
        if (!next && (frame->GetStateBits() & NS_FRAME_IS_SPECIAL)) {
          next = NS_STATIC_CAST(nsIFrame*, frame->GetFirstInFlow()->
                                  GetProperty(nsLayoutAtoms::IBSplitSpecialSibling));
        }
        frame = next;
      }
    }
  }
  *aResult = rects;
  NS_ADDREF(*aResult);
  return NS_OK;
}
