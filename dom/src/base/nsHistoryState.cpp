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
  if (JSVAL_IS_GCTHING(mHistoryStateValue) && !JSVAL_IS_NULL(mHistoryStateValue))
    JS_MarkGCThing(cx, JSVAL_TO_GCTHING(mHistoryStateValue), "history state", aArg);
}

void
nsGlobalWindow::ClearHistoryState()
{
  mHistoryStateData = nsnull;
  mHistoryStateValue = JSVAL_NULL;
}
