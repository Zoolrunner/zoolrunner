#include "nsNodeSelector.h"
#include "nsIContent.h"
#include "nsContentList.h"
#include "nsCSSSelectorQuery.h"
#include "nsICSSParser.h"
#include "nsICSSStyleRule.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDocument.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsIXPConnect.h"
#include "jsapi.h"

NS_IMPL_ADDREF(nsNodeSelector)
NS_IMPL_RELEASE(nsNodeSelector)
NS_INTERFACE_MAP_BEGIN(nsNodeSelector)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNodeSelector)
NS_INTERFACE_MAP_END_AGGREGATED(mNode)

class nsSelectorNodeList : public nsBaseContentList
{
public:
  PRBool Append(nsIContent* aContent) { return mElements.AppendObject(aContent); }
};

nsresult
nsNodeSelector::Query(const nsAString& aSelectors, nsIDOMElement** aFirst,
                      nsIDOMNodeList** aAll)
{
  if (aFirst)
    *aFirst = nsnull;
  if (aAll)
    *aAll = nsnull;
  NS_ENSURE_TRUE(mNode, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(mNode);
  if (!doc) {
    nsCOMPtr<nsIDOMDocument> owner;
    mNode->GetOwnerDocument(getter_AddRefs(owner));
    doc = do_QueryInterface(owner);
  }
  // The old DOMString binding preserves null as a void string. These new
  // non-nullable selector arguments use Web IDL's string conversion instead.
  nsAutoString source(aSelectors);
  nsCOMPtr<nsIXPCNativeCallContext> call;
  nsresult rv = nsContentUtils::XPConnect()->
    GetCurrentNativeCallContext(getter_AddRefs(call));
  NS_ENSURE_SUCCESS(rv, rv);
  if (call) {
    PRUint32 argc;
    jsval* argv;
    call->GetArgc(&argc);
    call->GetArgvPtr(&argv);
    if (argc && JSVAL_IS_NULL(argv[0]))
      source.AssignLiteral("null");
    else if (argc && JSVAL_IS_VOID(argv[0]))
      source.AssignLiteral("undefined");
  }
  nsCSSSelectorList* selectors = nsnull;
  rv = NS_ParseDOMSelectors(source, !doc || doc->IsCaseSensitive(),
                                    &selectors);
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoPtr<nsCSSSelectorList> selectorOwner(selectors);
  nsRefPtr<nsSelectorNodeList> result;
  if (aAll) {
    result = new nsSelectorNodeList();
    NS_ENSURE_TRUE(result, NS_ERROR_OUT_OF_MEMORY);
  }

  // Preorder traversal gives tree order and naturally de-duplicates selector
  // groups. Ancestor matching is not restricted to this traversal's root.
  nsCOMPtr<nsIDOMNode> node;
  rv = mNode->GetFirstChild(getter_AddRefs(node));
  NS_ENSURE_SUCCESS(rv, rv);
  while (node) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(node);
    if (content && content->IsContentOfType(nsIContent::eELEMENT) &&
        NS_MatchDOMSelectors(content, selectors)) {
      if (aFirst)
        return CallQueryInterface(node, aFirst);
      if (!result->Append(content))
        return NS_ERROR_OUT_OF_MEMORY;
    }
    nsCOMPtr<nsIDOMNode> next;
    rv = node->GetFirstChild(getter_AddRefs(next));
    NS_ENSURE_SUCCESS(rv, rv);
    while (!next) {
      rv = node->GetNextSibling(getter_AddRefs(next));
      NS_ENSURE_SUCCESS(rv, rv);
      if (next)
        break;
      nsCOMPtr<nsIDOMNode> parent;
      rv = node->GetParentNode(getter_AddRefs(parent));
      NS_ENSURE_SUCCESS(rv, rv);
      if (!parent || parent == mNode)
        break;
      node = parent;
    }
    node = next;
  }
  if (aAll)
    NS_ADDREF(*aAll = result);
  return NS_OK;
}

NS_IMETHODIMP
nsNodeSelector::QuerySelector(const nsAString& aSelectors, nsIDOMElement** aResult)
{
  return Query(aSelectors, aResult, nsnull);
}

NS_IMETHODIMP
nsNodeSelector::QuerySelectorAll(const nsAString& aSelectors, nsIDOMNodeList** aResult)
{
  return Query(aSelectors, nsnull, aResult);
}
