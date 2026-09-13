/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Boris Zbarsky <bzbarsky@mit.edu>.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#ifndef nsMediaQuery_h_
#define nsMediaQuery_h_

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "nsTArray.h"
class nsPresContext;
class MediaQueryParser;

// Immutable after Parse(), so cloned media lists can share parsed queries.
class nsMediaQuery : public nsISupports {
public:
  NS_DECL_ISUPPORTS
  nsMediaQuery() : mValid(PR_FALSE), mNegated(PR_FALSE) {}
  nsresult Parse(const nsAString& aText);
  PRBool Matches(nsPresContext* aContext) const;
  const nsString& Text() const { return mText; }

private:
  ~nsMediaQuery() {}
  friend class MediaQueryParser;
  struct Expression {
    enum Feature { Width, Height, Orientation } mFeature;
    enum Range { Equal, Minimum, Maximum } mRange;
    enum Unit { Pixels, Em, Ex } mUnit;
    double mValue;
    PRBool mHasValue;
    PRBool mLandscape;
  };
  nsString mText;
  nsCOMPtr<nsIAtom> mType;
  nsTArray<Expression> mExpressions;
  PRBool mValid;
  PRBool mNegated;
};
#endif
