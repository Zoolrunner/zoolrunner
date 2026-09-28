#ifndef nsNodeSelector_h_
#define nsNodeSelector_h_

#include "nsIDOMNodeSelector.h"
#include "nsIDOMNode.h"
#include "nsCOMPtr.h"

class nsNodeSelector : public nsIDOMNodeSelector
{
public:
  explicit nsNodeSelector(nsISupports* aNode) : mNode(do_QueryInterface(aNode)) {}
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMNODESELECTOR
private:
  ~nsNodeSelector() {}
  nsresult Query(const nsAString& aSelectors, nsIDOMElement** aFirst,
                 nsIDOMNodeList** aAll);
  nsCOMPtr<nsIDOMNode> mNode;
};

#endif
