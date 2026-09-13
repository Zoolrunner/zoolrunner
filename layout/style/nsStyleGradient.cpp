/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This file is available under the MPL 1.1/GPL 2.0/LGPL 2.1 tri-license.
 * See https://www.mozilla.org/MPL/1.1/ for the MPL terms.
 */
#include "nsStyleGradient.h"
#include "nsISupportsImpl.h"
#include "nsComponentManagerUtils.h"
#include "nsAutoPtr.h"
#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "gfxIFormats.h"
#include <math.h>
#include <string.h>

NS_IMPL_ISUPPORTS0(nsStyleGradient)

nsStyleGradient::nsStyleGradient()
  : mAngle(0), mDirection(0), mCachedSize(0, 0), mCachedPixelSize(0) {}
nsStyleGradient::~nsStyleGradient() {}

static PRUint8 GradientByte(double aValue)
{
  return PRUint8(PR_MAX(0.0, PR_MIN(255.0, aValue + 0.5)));
}

imgIContainer*
nsStyleGradient::GetImage(const nsSize& aSize, float aPixelSize)
{
  if (mCachedImage && mCachedSize == aSize && mCachedPixelSize == aPixelSize)
    return mCachedImage;
  if (aSize.width <= 0 || aSize.height <= 0 || !(aPixelSize > 0) || mStops.Length() < 2)
    return nsnull;

  double pixelWidth = ceil(double(aSize.width) / aPixelSize);
  double pixelHeight = ceil(double(aSize.height) / aPixelSize);
  // Bound both allocation sizes and work for pathological CSS dimensions.
  // Oversized generated images fail like an unavailable background image.
  if (pixelWidth > 8192 || pixelHeight > 8192 ||
      pixelWidth * pixelHeight > 32 * 1024 * 1024)
    return nsnull;
  PRInt32 width = PRInt32(pixelWidth), height = PRInt32(pixelHeight);
  double dx, dy;
  if (mDirection) {
    dx = (mDirection & 4) ? -1 : (mDirection & 8) ? 1 : 0;
    dy = (mDirection & 1) ? -1 : (mDirection & 2) ? 1 : 0;
    if (dx && dy) {
      dx *= aSize.height;
      dy *= aSize.width;
      double norm = sqrt(dx * dx + dy * dy);
      dx /= norm; dy /= norm;
    }
  } else {
    dx = sin(mAngle); dy = -cos(mAngle);
  }
  double length = fabs(dx) * aSize.width + fabs(dy) * aSize.height;
  nsTArray<double> positions;
  PRUint32 count = mStops.Length();
  if (!positions.SetLength(count))
    return nsnull;
  PRUint32 i;
  for (i = 0; i < count; ++i) {
    const nsStyleCoord& p = mStops[i].mPosition;
    positions[i] = p.GetUnit() == eStyleUnit_Percent ? p.GetPercentValue() * length :
                   p.GetUnit() == eStyleUnit_Coord ? p.GetCoordValue() : 0;
  }
  if (mStops[count - 1].mPosition.GetUnit() == eStyleUnit_Null)
    positions[count - 1] = length;
  // Clamp explicit positions in source order, then distribute omitted runs.
  PRUint32 previous = 0;
  for (i = 1; i < count; ++i) {
    if (i == count - 1 || mStops[i].mPosition.GetUnit() != eStyleUnit_Null) {
      positions[i] = PR_MAX(positions[previous], positions[i]);
      for (PRUint32 j = previous + 1; j < i; ++j)
        positions[j] = positions[previous] +
                       (positions[i] - positions[previous]) * (j - previous) / (i - previous);
      previous = i;
    }
  }

  nsCOMPtr<imgIContainer> image = do_CreateInstance("@mozilla.org/image/container;1");
  nsCOMPtr<gfxIImageFrame> frame = do_CreateInstance("@mozilla.org/gfx/image/frame;2");
  if (!image || !frame || NS_FAILED(image->Init(width, height, nsnull)))
    return nsnull;
  gfx_format format = gfxIFormats::RGB_A8;
#if defined(XP_WIN) || defined(XP_OS2) || defined(XP_BEOS) || defined(MOZ_WIDGET_PHOTON)
  format = gfxIFormats::BGR_A8;
#endif
  if (NS_FAILED(frame->Init(0, 0, width, height, format, 24)))
    return nsnull;
  PRUint32 stride = 0, alphaStride = 0;
  frame->GetImageBytesPerRow(&stride);
  frame->GetAlphaBytesPerRow(&alphaStride);
  PRUint32 bytesPerPixel = 3;
#if (defined(XP_MAC) || defined(XP_MACOSX)) && !defined(MOZ_ENABLE_CAIRO_GFX)
  bytesPerPixel = 4;
#endif
  if (stride < PRUint32(width) * bytesPerPixel || alphaStride < PRUint32(width))
    return nsnull;
  nsAutoArrayPtr<PRUint8> row(new PRUint8[stride]);
  nsAutoArrayPtr<PRUint8> alpha(new PRUint8[alphaStride]);
  if (!row || !alpha)
    return nsnull;
  memset(row, 0, stride);
  memset(alpha, 0, alphaStride);
  for (PRInt32 y = 0; y < height; ++y) {
    for (PRInt32 x = 0; x < width; ++x) {
      double position = ((x + 0.5) * aSize.width / width - aSize.width * 0.5) * dx +
                        ((y + 0.5) * aSize.height / height - aSize.height * 0.5) * dy + length * 0.5;
      // Upper bound makes coincident stops a hard edge, with the later color
      // winning on the edge. Binary search bounds work for long stop lists.
      PRUint32 lo = 0, hi = count;
      while (lo < hi) {
        PRUint32 mid = lo + (hi - lo) / 2;
        if (positions[mid] <= position) lo = mid + 1;
        else hi = mid;
      }
      PRUint32 left = lo ? lo - 1 : 0;
      PRUint32 right = lo < count ? lo : count - 1;
      double t = left == right ? 0 :
                 (position - positions[left]) / (positions[right] - positions[left]);
      nscolor c1 = mStops[left].mColor, c2 = mStops[right].mColor;
      double a1 = NS_GET_A(c1), a2 = NS_GET_A(c2);
      double a = a1 + (a2 - a1) * t;
      double r = 0, g = 0, b = 0;
      // Interpolate premultiplied sRGB, then give the legacy image frame its
      // unassociated RGB and separate alpha plane.
      if (a > 0) {
        r = (NS_GET_R(c1) * a1 * (1 - t) + NS_GET_R(c2) * a2 * t) / a;
        g = (NS_GET_G(c1) * a1 * (1 - t) + NS_GET_G(c2) * a2 * t) / a;
        b = (NS_GET_B(c1) * a1 * (1 - t) + NS_GET_B(c2) * a2 * t) / a;
      }
      PRUint8* dest = row + x * bytesPerPixel;
      if (bytesPerPixel == 4) ++dest;
      dest[0] = GradientByte(format == gfxIFormats::BGR_A8 ? b : r);
      dest[1] = GradientByte(g);
      dest[2] = GradientByte(format == gfxIFormats::BGR_A8 ? r : b);
      alpha[x] = GradientByte(a);
    }
    if (NS_FAILED(frame->SetAlphaData(alpha, alphaStride, y * alphaStride)) ||
        NS_FAILED(frame->SetImageData(row, stride, y * stride)))
      return nsnull;
  }
  if (NS_FAILED(image->AppendFrame(frame)) || NS_FAILED(frame->SetMutable(PR_FALSE)))
    return nsnull;
  mCachedSize = aSize;
  mCachedPixelSize = aPixelSize;
  mCachedImage = image;
  return mCachedImage;
}

void
nsStyleGradient::ToString(nsAString& aResult, float aTwipsToPixels) const
{
  aResult.AssignLiteral("linear-gradient(");
  nsAutoString number;
  if (mDirection) {
    aResult.AppendLiteral("to");
    if (mDirection & 1) aResult.AppendLiteral(" top");
    if (mDirection & 2) aResult.AppendLiteral(" bottom");
    if (mDirection & 4) aResult.AppendLiteral(" left");
    if (mDirection & 8) aResult.AppendLiteral(" right");
  } else {
    number.AppendFloat(mAngle * 180.0 / 3.14159265358979323846);
    aResult.Append(number);
    aResult.AppendLiteral("deg");
  }
  for (PRUint32 i = 0; i < mStops.Length(); ++i) {
    nscolor c = mStops[i].mColor;
    aResult.AppendLiteral(", ");
    if (NS_GET_A(c) == 255) aResult.AppendLiteral("rgb(");
    else aResult.AppendLiteral("rgba(");
    number.Truncate(); number.AppendInt(NS_GET_R(c)); aResult.Append(number);
    aResult.AppendLiteral(", ");
    number.Truncate(); number.AppendInt(NS_GET_G(c)); aResult.Append(number);
    aResult.AppendLiteral(", ");
    number.Truncate(); number.AppendInt(NS_GET_B(c)); aResult.Append(number);
    if (NS_GET_A(c) != 255) {
      aResult.AppendLiteral(", ");
      number.Truncate(); number.AppendFloat(NS_GET_A(c) / 255.0f); aResult.Append(number);
    }
    aResult.AppendLiteral(")");
    const nsStyleCoord& p = mStops[i].mPosition;
    if (p.GetUnit() != eStyleUnit_Null) {
      aResult.AppendLiteral(" ");
      number.Truncate();
      number.AppendFloat(p.GetUnit() == eStyleUnit_Percent ?
                         p.GetPercentValue() * 100.0f : p.GetCoordValue() * aTwipsToPixels);
      aResult.Append(number);
      if (p.GetUnit() == eStyleUnit_Percent) aResult.AppendLiteral("%");
      else aResult.AppendLiteral("px");
    }
  }
  aResult.AppendLiteral(")");
}
