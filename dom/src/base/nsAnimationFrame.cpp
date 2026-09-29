/* Native animation callback scheduling for classic window globals.
 * License: MPL 1.1/GPL 2.0/LGPL 2.1, as with the surrounding DOM code.
 */
#include "nsAnimationFrame.h"
#include "nsGlobalWindow.h"
#include "nsIScriptContext.h"
#include "nsIDocument.h"
#include "nsIDocShell.h"
#include "nsIBaseWindow.h"
#include "nsITimer.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMArray.h"
#include "mozFlushType.h"
#include "prtime.h"
#include "prinrval.h"
#if defined(XP_WIN)
#include <windows.h>
#elif defined(XP_MACOSX)
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

// Use a monotonic source independently of wall-clock corrections. The fallback
// extends NSPR's wrapping interval counter for platforms without such an API.
double
NS_AnimationFrameClock()
{
#if defined(XP_WIN)
  LARGE_INTEGER frequency, counter;
  if (QueryPerformanceFrequency(&frequency) && frequency.QuadPart > 0 &&
      QueryPerformanceCounter(&counter))
    return double(counter.QuadPart) * 1000.0 / double(frequency.QuadPart);
#elif defined(XP_MACOSX)
  mach_timebase_info_data_t scale;
  if (mach_timebase_info(&scale) == KERN_SUCCESS && scale.denom)
    return double(mach_absolute_time()) * double(scale.numer) /
           double(scale.denom) / 1000000.0;
#elif defined(CLOCK_MONOTONIC)
  struct timespec now;
  if (clock_gettime(CLOCK_MONOTONIC, &now) == 0)
    return double(now.tv_sec) * 1000.0 + double(now.tv_nsec) / 1000000.0;
#endif
  static PRIntervalTime previous = PR_IntervalNow();
  static PRUint64 elapsed = 0;
  PRIntervalTime current = PR_IntervalNow();
  elapsed += PRIntervalTime(current - previous);
  previous = current;
  return double(elapsed) * 1000.0 / PR_TicksPerSecond();
}

struct AnimationRequest {
  AnimationRequest(nsGlobalWindow* aWindow, JSRuntime* aRuntime)
    : next(nsnull), window(aWindow), runtime(aRuntime), callback(nsnull),
      global(nsnull), handle(0), timestamp(0), canceled(PR_FALSE),
      callbackRooted(PR_FALSE), globalRooted(PR_FALSE) {}
  ~AnimationRequest() {
    if (callbackRooted) JS_RemoveRootRT(runtime, &callback);
    if (globalRooted) JS_RemoveRootRT(runtime, &global);
  }
  AnimationRequest* next;
  nsRefPtr<nsGlobalWindow> window;
  JSRuntime* runtime;
  JSObject* callback;
  JSObject* global;
  PRUint32 handle;
  double timestamp;
  PRBool canceled, callbackRooted, globalRooted;
};

static AnimationRequest* sPendingFrames;
static AnimationRequest* sRunningFrames;
static nsCOMPtr<nsITimer> sAnimationTimer;
static PRUint32 sFrameHandle;
static PRBool sSamplingFrames;

static PRBool
CanAnimate(nsGlobalWindow* aWindow)
{
  if (aWindow->IsFrozen() || !aWindow->GetExtantDocument())
    return PR_FALSE;
  nsIScriptContext* context = aWindow->GetContextInternal();
  if (!context || !context->GetScriptsEnabled())
    return PR_FALSE;
  nsCOMPtr<nsIBaseWindow> base = do_QueryInterface(aWindow->GetDocShell());
  PRBool visible = PR_FALSE;
  return base && NS_SUCCEEDED(base->GetVisibility(&visible)) && visible;
}

static nsresult ScheduleAnimationFrames(PRUint32 aDelay);

static void
SampleAnimationFrames(nsITimer*, void*)
{
  sAnimationTimer = nsnull;
  if (sSamplingFrames)
    return;
  sSamplingFrames = PR_TRUE;

  // Snapshot every eligible document before invoking any script. A callback
  // registered during this batch belongs to the next rendering opportunity.
  double now = NS_AnimationFrameClock();
  AnimationRequest** pending = &sPendingFrames;
  AnimationRequest** tail = &sRunningFrames;
  while (*pending) {
    AnimationRequest* request = *pending;
    if (CanAnimate(request->window)) {
      *pending = request->next;
      request->next = nsnull;
      request->timestamp = request->window->AnimationFrameTime(now);
      *tail = request;
      tail = &request->next;
    } else {
      pending = &request->next;
    }
  }

  nsCOMArray<nsIDocument> documents;
  while (sRunningFrames) {
    AnimationRequest* request = sRunningFrames;
    // Leave this entry in the running list during the call so cancellation
    // and window teardown can mark the entire snapshot, even under reentry.
    if (!request->canceled && CanAnimate(request->window)) {
      nsCOMPtr<nsIScriptContext> context = request->window->GetContextInternal();
      JSContext* cx = NS_STATIC_CAST(JSContext*, context->GetNativeContext());
      jsval argument = JSVAL_VOID, result;
      if (JS_AddNamedRoot(cx, &argument, "animation frame timestamp")) {
        if (JS_NewNumberValue(cx, request->timestamp, &argument)) {
          nsAutoPopupStatePusher popupState(openAbused);
          context->CallEventHandler(request->global, request->callback,
                                     1, &argument, &result);
        }
        JS_RemoveRoot(cx, &argument);
      }
      nsCOMPtr<nsIDocument> doc =
        do_QueryInterface(request->window->GetExtantDocument());
      if (doc && documents.IndexOf(doc) < 0)
        documents.AppendObject(doc);
    }
    sRunningFrames = request->next;
    delete request;
  }

  // Complete layout for the batch before the event loop paints the views.
  for (PRInt32 i = 0; i < documents.Count(); ++i)
    documents[i]->FlushPendingNotifications(Flush_Layout);

  sSamplingFrames = PR_FALSE;
  PRBool active = PR_FALSE;
  for (AnimationRequest* r = sPendingFrames; r && !active; r = r->next)
    active = CanAnimate(r->window);
  if (sPendingFrames && NS_FAILED(ScheduleAnimationFrames(active ? 16 : 250))) {
    // Do not retain document globals indefinitely if the timer cannot be made.
    while (sPendingFrames) {
      AnimationRequest* request = sPendingFrames;
      sPendingFrames = request->next;
      delete request;
    }
  }
}

static nsresult
ScheduleAnimationFrames(PRUint32 aDelay)
{
  if (sAnimationTimer || sSamplingFrames)
    return NS_OK;
  nsCOMPtr<nsITimer> timer = do_CreateInstance("@mozilla.org/timer;1");
  NS_ENSURE_TRUE(timer, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = timer->InitWithFuncCallback(SampleAnimationFrames, nsnull,
                                            aDelay, nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);
  sAnimationTimer = timer;
  return NS_OK;
}

static PRBool
HasFrameHandle(PRUint32 aHandle)
{
  for (AnimationRequest* r = sPendingFrames; r; r = r->next)
    if (r->handle == aHandle) return PR_TRUE;
  for (AnimationRequest* r = sRunningFrames; r; r = r->next)
    if (r->handle == aHandle) return PR_TRUE;
  return PR_FALSE;
}

nsresult
NS_RequestAnimationFrame(nsGlobalWindow* aWindow, JSContext* aContext,
                          JSObject* aCallback, PRUint32* aHandle)
{
  NS_ENSURE_STATE(aWindow && aWindow->IsInnerWindow() &&
                  aWindow->GetGlobalJSObject());
  do { ++sFrameHandle; } while (!sFrameHandle || HasFrameHandle(sFrameHandle));
  *aHandle = sFrameHandle;
  // A retained Window from a destroyed docshell cannot produce another frame.
  if (!aWindow->GetExtantDocument() || !aWindow->GetDocShell())
    return NS_OK;
  nsAutoPtr<AnimationRequest> request(
    new AnimationRequest(aWindow, JS_GetRuntime(aContext)));
  NS_ENSURE_TRUE(request, NS_ERROR_OUT_OF_MEMORY);
  request->callbackRooted = JS_AddNamedRootRT(request->runtime,
    &request->callback, "animation frame callback");
  if (!request->callbackRooted) return NS_ERROR_OUT_OF_MEMORY;
  request->callback = aCallback;
  request->globalRooted = JS_AddNamedRootRT(request->runtime,
    &request->global, "animation frame window");
  if (!request->globalRooted) return NS_ERROR_OUT_OF_MEMORY;
  request->global = aWindow->GetGlobalJSObject();

  request->handle = *aHandle;
  nsresult rv = ScheduleAnimationFrames(16);
  NS_ENSURE_SUCCESS(rv, rv);
  AnimationRequest** tail = &sPendingFrames;
  while (*tail) tail = &(*tail)->next;
  *aHandle = request->handle;
  *tail = request.forget();
  return NS_OK;
}

static void
CancelAnimationFrames(nsGlobalWindow* aWindow, PRUint32 aHandle, PRBool aAll)
{
  AnimationRequest** pending = &sPendingFrames;
  while (*pending) {
    AnimationRequest* r = *pending;
    if (r->window == aWindow && (aAll || r->handle == aHandle)) {
      *pending = r->next;
      delete r;
    } else {
      pending = &r->next;
    }
  }
  for (AnimationRequest* r = sRunningFrames; r; r = r->next)
    if (r->window == aWindow && (aAll || r->handle == aHandle))
      r->canceled = PR_TRUE;
  if (!sPendingFrames && sAnimationTimer) {
    sAnimationTimer->Cancel();
    sAnimationTimer = nsnull;
  }
}

void NS_CancelAnimationFrame(nsGlobalWindow* aWindow, PRUint32 aHandle)
{
  CancelAnimationFrames(aWindow, aHandle, PR_FALSE);
}

void NS_ClearAnimationFrames(nsGlobalWindow* aWindow)
{
  CancelAnimationFrames(aWindow, 0, PR_TRUE);
}
