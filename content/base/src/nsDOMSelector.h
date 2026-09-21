/* Helpers shared by Document and Element selector bindings. */
#ifndef nsDOMSelector_h___
#define nsDOMSelector_h___

#include "nsError.h"
#include "nsString.h"

class nsIContent;
class nsIDOMElement;
class nsIDOMNodeList;

nsresult NS_QuerySelector(nsIContent* aScope, PRBool aIncludeScope,
                          const nsAString& aSelectors,
                          nsIDOMElement** aResult);
nsresult NS_QuerySelectorAll(nsIContent* aScope, PRBool aIncludeScope,
                             const nsAString& aSelectors,
                             nsIDOMNodeList** aResult);

#endif
