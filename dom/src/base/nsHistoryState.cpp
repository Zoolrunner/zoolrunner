/* Immutable serialized History API data. MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "nsHistoryState.h"

class nsHistoryStateData : public nsIHistoryStateData
{
public:
  explicit nsHistoryStateData(JSStructuredValue* aData) : mData(aData) {}
  NS_DECL_ISUPPORTS
  virtual JSBool Read(JSContext* aContext, JSObject* aGlobal, jsval* aValue)
  {
    return JS_ReadStructuredValue(aContext, aGlobal, mData, aValue);
  }
private:
  ~nsHistoryStateData() { JS_FreeStructuredValue(mData); }
  JSStructuredValue* mData;
};

NS_IMPL_ISUPPORTS1(nsHistoryStateData, nsIHistoryStateData)

nsresult
NS_NewHistoryStateData(JSStructuredValue* aData, nsIHistoryStateData** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;
  nsHistoryStateData* state = new nsHistoryStateData(aData);
  NS_ENSURE_TRUE(state, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aResult = state);
  return NS_OK;
}

#include "nsGlobalWindow.h"

JSBool
nsGlobalWindow::ReadHistoryState(JSContext* cx, nsISupports* aData, jsval* aResult)
{
  NS_ASSERTION(IsInnerWindow(), "history state belongs to an inner window");
  if (mHistoryStateData != aData) {
    jsval value = JSVAL_NULL;
    nsCOMPtr<nsIHistoryStateData> data = do_QueryInterface(aData);
    if (data && !data->Read(cx, GetGlobalJSObject(), &value))
      return JS_FALSE;
    mHistoryStateData = aData;
    mHistoryStateValue = value;
  }
  *aResult = mHistoryStateValue;
  return JS_TRUE;
}

void
nsGlobalWindow::MarkHistoryState(JSContext* cx, void* aArg)
{
  if (mEventStateMap)
    JS_MarkGCThing(cx, mEventStateMap, "event state weak map", aArg);
  if (JSVAL_IS_GCTHING(mHistoryStateValue) && !JSVAL_IS_NULL(mHistoryStateValue))
    JS_MarkGCThing(cx, JSVAL_TO_GCTHING(mHistoryStateValue), "history state", aArg);
}

void
nsGlobalWindow::ClearHistoryState()
{
  mHistoryStateData = nsnull;
  mHistoryStateValue = JSVAL_NULL;
}

JSBool
nsGlobalWindow::GetEventState(JSContext* cx, JSObject* aEvent, jsval* aResult)
{
  JSBool found = JS_FALSE;
  *aResult = JSVAL_NULL;
  if (mEventStateMap &&
      !JS_GetWeakMapEntry(cx, mEventStateMap, aEvent, aResult, &found))
    return JS_FALSE;
  if (!found) *aResult = JSVAL_NULL;
  return JS_TRUE;
}

JSBool
nsGlobalWindow::SetEventState(JSContext* cx, JSObject* aEvent, jsval aValue)
{
  // Keep this table until the global is finalized, including after document
  // teardown: a retained event must retain its state in its original realm.
  if (!mEventStateMap) {
    mEventStateMap = JS_NewWeakMapObject(cx, GetGlobalJSObject());
    if (!mEventStateMap) return JS_FALSE;
  }
  return JS_SetWeakMapEntry(cx, mEventStateMap, aEvent, aValue);
}

#include "nsContentUtils.h"
#include "nsDOMClassInfo.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMEvent.h"

NS_IMETHODIMP
nsGlobalWindow::ResetHistoryState()
{
  nsGlobalWindow* inner = IsOuterWindow() ? GetCurrentInnerWindowInternal() : this;
  if (inner) inner->ClearHistoryState();
  return NS_OK;
}

NS_IMETHODIMP
nsGlobalWindow::DispatchHistoryState(nsISupports* aData)
{
  nsRefPtr<nsGlobalWindow> inner = IsOuterWindow() ? GetCurrentInnerWindowInternal() : this;
  NS_ENSURE_TRUE(inner && inner->GetExtantDocument(), NS_ERROR_NOT_AVAILABLE);
  nsCOMPtr<nsIScriptContext> context = inner->GetContext();
  NS_ENSURE_TRUE(context, NS_ERROR_NOT_AVAILABLE);
  JSContext* cx = (JSContext*)context->GetNativeContext();
  NS_ENSURE_TRUE(cx, NS_ERROR_NOT_AVAILABLE);
  JSAutoRequest request(cx);
  jsval value = JSVAL_NULL, eventValue = JSVAL_VOID;
  nsresult rv;
  nsAutoGCRoot valueRoot(&value, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoGCRoot eventRoot(&eventValue, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!inner->ReadHistoryState(cx, aData, &value)) return NS_ERROR_FAILURE;
  nsCOMPtr<nsIDOMEvent> event;
  rv = NS_NewDOMPopStateEvent(getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = event->InitEvent(NS_LITERAL_STRING("popstate"), PR_FALSE, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIPrivateDOMEvent2> owner = do_QueryInterface(event);
  owner->SetEventGlobal(inner);
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
  privateEvent->SetTrusted(PR_TRUE);
  nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
  rv = nsDOMClassInfo::WrapNative(cx, inner->GetGlobalJSObject(), event,
                                 NS_GET_IID(nsIDOMEvent), &eventValue,
                                 getter_AddRefs(holder));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!inner->SetEventState(cx, JSVAL_TO_OBJECT(eventValue), value))
    return NS_ERROR_FAILURE;
  PRBool defaultAction;
  return inner->DispatchEvent(event, &defaultAction);
}

#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIWebNavigation.h"
#include "nsISHistory.h"
#include "nsIEventQueueService.h"
#include "nsIEventQueue.h"
#include "nsServiceManagerUtils.h"
#include "plevent.h"
#include "prthread.h"

struct nsHistoryTraversalEvent : public PLEvent
{
  nsHistoryTraversalEvent(nsGlobalWindow* aWindow, PRInt32 aDelta)
    : window(aWindow), document(aWindow->GetExtantDocument()), delta(aDelta) {}
  nsRefPtr<nsGlobalWindow> window;
  nsCOMPtr<nsIDOMDocument> document;
  PRInt32 delta;
  static void* PR_CALLBACK Handle(PLEvent* aEvent)
  {
    nsHistoryTraversalEvent* event = NS_STATIC_CAST(nsHistoryTraversalEvent*, aEvent);
    nsGlobalWindow* window = event->window;
    if (!window->GetOuterWindow() ||
        window->GetOuterWindow()->GetCurrentInnerWindow() != window ||
        window->GetExtantDocument() != event->document)
      return nsnull;
    nsCOMPtr<nsIDocShell> shell = window->GetDocShell();
    if (!shell) return nsnull;
    nsCOMPtr<nsIWebNavigation> navigation = do_QueryInterface(shell);
    if (event->delta == 0) {
      if (navigation) navigation->Reload(nsIWebNavigation::LOAD_FLAGS_NONE);
      return nsnull;
    }
    nsCOMPtr<nsIDocShellTreeItem> item = do_QueryInterface(shell), root;
    item->GetSameTypeRootTreeItem(getter_AddRefs(root));
    navigation = do_QueryInterface(root);
    nsCOMPtr<nsISHistory> history;
    if (navigation) navigation->GetSessionHistory(getter_AddRefs(history));
    if (!history) return nsnull;
    PRInt32 index = -1, count = 0;
    history->GetIndex(&index);
    history->GetCount(&count);
    // Bounds before addition avoid signed overflow for arbitrary WebIDL longs.
    if (index >= 0 && index < count && event->delta >= -index &&
        event->delta < count - index) {
      navigation = do_QueryInterface(history);
      if (navigation) navigation->GotoIndex(index + event->delta);
    }
    return nsnull;
  }
  static void PR_CALLBACK Destroy(PLEvent* aEvent)
  { delete NS_STATIC_CAST(nsHistoryTraversalEvent*, aEvent); }
};

nsresult
nsGlobalWindow::QueueHistoryTraversal(PRInt32 aDelta)
{
  nsHistoryTraversalEvent* event = new nsHistoryTraversalEvent(this, aDelta);
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);
  nsCOMPtr<nsIEventQueueService> service = do_GetService(NS_EVENTQUEUESERVICE_CONTRACTID);
  nsCOMPtr<nsIEventQueue> queue;
  if (service) service->GetThreadEventQueue(PR_GetCurrentThread(), getter_AddRefs(queue));
  nsresult rv = NS_ERROR_NOT_AVAILABLE;
  if (queue) {
    PL_InitEvent(event, nsnull, nsHistoryTraversalEvent::Handle, nsHistoryTraversalEvent::Destroy);
    rv = queue->PostEvent(event);
  }
  if (NS_FAILED(rv)) delete event;
  return rv;
}
