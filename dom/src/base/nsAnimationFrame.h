/* Native animation callback scheduling for classic window globals. */
#ifndef nsAnimationFrame_h___
#define nsAnimationFrame_h___
#include "nsError.h"
#include "jsapi.h"
class nsGlobalWindow;

double NS_AnimationFrameClock();
nsresult NS_RequestAnimationFrame(nsGlobalWindow* aWindow, JSContext* aContext,
                                   JSObject* aCallback, PRUint32* aHandle);
void NS_CancelAnimationFrame(nsGlobalWindow* aWindow, PRUint32 aHandle);
void NS_ClearAnimationFrames(nsGlobalWindow* aWindow);
#endif
