/* Native PopStateEvent. Values are traced through the event's owning realm;
 * the native event never creates an unconditional JavaScript GC root.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "nsDOMEvent.h"
#include "nsContentUtils.h"
#include "nsIDOMPopStateEvent.h"

class nsDOMPopStateEvent : public nsDOMEvent, public nsIDOMPopStateEvent
{
public:
  nsDOMPopStateEvent() : nsDOMEvent(nsnull, nsnull) {}
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_NSDOMEVENT
};
NS_IMPL_ADDREF_INHERITED(nsDOMPopStateEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMPopStateEvent, nsDOMEvent)
NS_INTERFACE_MAP_BEGIN(nsDOMPopStateEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMPopStateEvent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(PopStateEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

nsresult
NS_NewDOMPopStateEvent(nsIDOMEvent** aResult)
{
  nsDOMPopStateEvent* event = new nsDOMPopStateEvent();
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(event, aResult);
}
