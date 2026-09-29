/* This file is available under the MPL 1.1/GPL 2.0/LGPL 2.1 tri-license.
 * See https://www.mozilla.org/MPL/1.1/ for the MPL terms.
 */
#ifndef nsImageCompositing_h_
#define nsImageCompositing_h_

#include "prtypes.h"

// Composite an unassociated 8-bit image channel over an opaque destination.
// Round once at the output, rather than biasing every translucent layer down.
// Keep the historical, truncating MOZ_BLEND macro's public contract unchanged.
static inline PRUint8
nsBlendImageChannel(PRUint8 aBackground, PRUint8 aForeground, PRUint8 aAlpha)
{
  return PRUint8((PRUint32(aBackground) * (255 - aAlpha) +
                  PRUint32(aForeground) * aAlpha + 127) / 255);
}

#endif
