/* Native selector matching shared with the DOM selector interfaces. */
#ifndef nsCSSSelectorQuery_h_
#define nsCSSSelectorQuery_h_

#include "prtypes.h"
class nsIContent;
struct nsCSSSelectorList;

PRBool NS_MatchDOMSelectors(nsIContent* aContent, nsCSSSelectorList* aSelectors);

#endif
