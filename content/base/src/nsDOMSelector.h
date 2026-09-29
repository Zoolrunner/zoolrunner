/* Helpers shared by Document and Element selector bindings. */
#ifndef nsDOMSelector_h___
#define nsDOMSelector_h___

#include "nsError.h"
#include "nsString.h"

class nsIContent;
class nsIDocument;
class nsIDOMElement;
class nsIDOMNodeList;

nsresult NS_QuerySelector(nsIContent* aScope, PRBool aIncludeScope,
                          const nsAString& aSelectors,
                          nsIDOMElement** aResult,
                          nsIDocument* aDocument = nsnull);
nsresult NS_QuerySelectorAll(nsIContent* aScope, PRBool aIncludeScope,
                             const nsAString& aSelectors,
                             nsIDOMNodeList** aResult,
                             nsIDocument* aDocument = nsnull);

#endif
