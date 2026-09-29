/* Native DOM CustomEvent implementation. */
#include "nsDOMEvent.h"
#include "nsIDOMCustomEvent2.h"
#include "nsIJSRuntimeService.h"
#include "nsServiceManagerUtils.h"
#include "nsContentUtils.h"

class nsDOMCustomEvent : public nsDOMEvent, public nsIDOMCustomEvent2
{
public:
  nsDOMCustomEvent() : nsDOMEvent(nsnull, nsnull), mDetail(JSVAL_NULL), mRuntime(nsnull) {}
  ~nsDOMCustomEvent() { if (mRuntime) JS_RemoveRootRT(mRuntime, &mDetail); }
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_NSDOMEVENT
  NS_DECL_NSIDOMCUSTOMEVENT2
private:
  jsval mDetail;
  JSRuntime* mRuntime;
};

NS_IMPL_ADDREF_INHERITED(nsDOMCustomEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMCustomEvent, nsDOMEvent)
NS_INTERFACE_MAP_BEGIN(nsDOMCustomEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCustomEvent2)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(CustomEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMETHODIMP
nsDOMCustomEvent::GetDetailValue(jsval* aDetail)
{
  *aDetail = mDetail;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMCustomEvent::InitCustomEventValue(const nsAString& aType, PRBool aBubbles,
                                  PRBool aCancelable, jsval aDetail)
{
  if (mEvent->flags & NS_EVENT_FLAG_DISPATCHING)
    return NS_OK;
  if (!mRuntime) {
    nsCOMPtr<nsIJSRuntimeService> service = do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
    JSRuntime* runtime = nsnull;
    NS_ENSURE_TRUE(service, NS_ERROR_NOT_AVAILABLE);
    nsresult rv = service->GetRuntime(&runtime);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!JS_AddNamedRootRT(runtime, &mDetail, "CustomEvent detail"))
      return NS_ERROR_OUT_OF_MEMORY;
    mRuntime = runtime;
  }
  nsresult rv = nsDOMEvent::InitEvent(aType, aBubbles, aCancelable);
  NS_ENSURE_SUCCESS(rv, rv);
  mEvent->flags &= ~(NS_EVENT_FLAG_CANT_BUBBLE | NS_EVENT_FLAG_CANT_CANCEL |
                     NS_EVENT_FLAG_STOP_DISPATCH | NS_EVENT_FLAG_NO_DEFAULT);
  if (!aBubbles) mEvent->flags |= NS_EVENT_FLAG_CANT_BUBBLE;
  if (!aCancelable) mEvent->flags |= NS_EVENT_FLAG_CANT_CANCEL;
  mTarget = nsnull;
  mDetail = aDetail;
  return NS_OK;
}

nsresult
NS_NewDOMCustomEvent(nsIDOMEvent** aResult)
{
  nsDOMCustomEvent* event = new nsDOMCustomEvent();
  if (!event) return NS_ERROR_OUT_OF_MEMORY;
  return CallQueryInterface(event, aResult);
}
