/* Immutable serialized History API data. MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef nsHistoryState_h___
#define nsHistoryState_h___
#include "nsISupports.h"
#include "jsapi.h"

#define NS_IHISTORYSTATEDATA_IID \
{0x480b913a, 0x4f88, 0x444a, {0x92,0x66,0x42,0x1d,0x4f,0x74,0x7d,0xac}}

class nsIDocShell;
#define NS_IHISTORYSTATEOWNER_IID \
{0x1f3c9042, 0x3e6a, 0x43e3, {0x9f,0x30,0x66,0x9d,0x77,0x2e,0xa0,0x31}}
class nsIHistoryStateOwner : public nsISupports
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IHISTORYSTATEOWNER_IID)
  virtual nsresult GetHistoryDocShell(nsIDocShell** aResult) = 0;
  virtual nsresult LegacyGo(PRInt32 aDelta) = 0;
};

class nsIHistoryStateData : public nsISupports
{
public:
  NS_DEFINE_STATIC_IID_ACCESSOR(NS_IHISTORYSTATEDATA_IID)
  virtual JSBool Read(JSContext* aContext, JSObject* aGlobal, jsval* aValue) = 0;
};

// Takes ownership of aData on success only. The payload has no GC roots or
// document/window references, and can be shared by session-entry clones.
nsresult NS_NewHistoryStateData(JSStructuredValue* aData,
                                nsIHistoryStateData** aResult);
#endif
