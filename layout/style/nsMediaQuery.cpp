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
#include "nsMediaQuery.h"
#include "nsCSSScanner.h"
#include "nsIUnicharInputStream.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsPresContext.h"
#include "nsLayoutAtoms.h"
#include "nsFont.h"
#include "nsIFontMetrics.h"
#include <float.h>

NS_IMPL_ISUPPORTS0(nsMediaQuery)

class MediaQueryParser {
public:
  MediaQueryParser(nsMediaQuery& aQuery) : mQuery(aQuery), mError(NS_OK) {}
  nsresult Parse(const nsAString& aText) {
    nsCOMPtr<nsIUnicharInputStream> input;
    nsresult rv = NS_NewStringUnicharInputStream(getter_AddRefs(input), &aText, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    mScanner.Init(input, nsnull, 0);
    mQuery.mValid = ParseQuery();
    if (!mQuery.mValid)
      mQuery.mText.AssignLiteral("not all");
    mScanner.Close();
    return mError;
  }
private:
  typedef nsMediaQuery::Expression Expression;
  PRBool Next() {
    while (mScanner.Next(mError, mToken)) {
      if (mToken.mType != eCSSToken_WhiteSpace) {
        if (mToken.mType == eCSSToken_Ident ||
            mToken.mType == eCSSToken_Dimension)
          ToLowerCase(mToken.mIdent);
        return PR_TRUE;
      }
    }
    return PR_FALSE;
  }
  PRBool Ident(const char* aName) {
    return mToken.mType == eCSSToken_Ident &&
           mToken.mIdent.EqualsASCII(aName);
  }
  PRBool ParseQuery() {
    if (!Next()) return PR_FALSE;
    PRBool hasType = PR_FALSE;
    if (Ident("not") || Ident("only")) {
      mQuery.mNegated = Ident("not");
      mQuery.mText.Assign(mToken.mIdent);
      mQuery.mText.Append(PRUnichar(' '));
      if (!Next() || mToken.mType != eCSSToken_Ident) return PR_FALSE;
    }
    if (mToken.mType == eCSSToken_Ident) {
      if (Ident("not") || Ident("only") || Ident("and") ||
          Ident("or") || Ident("layer")) return PR_FALSE;
      hasType = PR_TRUE;
      mQuery.mType = do_GetAtom(mToken.mIdent);
      if (!mQuery.mType) { mError = NS_ERROR_OUT_OF_MEMORY; return PR_FALSE; }
      mQuery.mText.Append(mToken.mIdent);
      if (!Next()) return NS_SUCCEEDED(mError);
      if (!Ident("and") || !Next()) return PR_FALSE;
    } else {
      mQuery.mType = nsLayoutAtoms::all;
    }
    for (;;) {
      if (!mToken.IsSymbol('(')) return PR_FALSE;
      if (hasType || !mQuery.mExpressions.IsEmpty())
        mQuery.mText.AppendLiteral(" and ");
      if (!ParseExpression()) return PR_FALSE;
      if (!Next()) return NS_SUCCEEDED(mError);
      if (!Ident("and") || !Next()) return PR_FALSE;
    }
  }
  PRBool ParseExpression() {
    if (!Next() || mToken.mType != eCSSToken_Ident) return PR_FALSE;
    nsAutoString name(mToken.mIdent);
    mQuery.mText.Append(PRUnichar('('));
    mQuery.mText.Append(name);
    Expression expr;
    expr.mRange = Expression::Equal;
    expr.mUnit = Expression::Pixels;
    expr.mValue = 0;
    expr.mHasValue = PR_FALSE;
    expr.mLandscape = PR_FALSE;
    if (StringBeginsWith(name, NS_LITERAL_STRING("min-"))) {
      expr.mRange = Expression::Minimum;
      name.Cut(0, 4);
    } else if (StringBeginsWith(name, NS_LITERAL_STRING("max-"))) {
      expr.mRange = Expression::Maximum;
      name.Cut(0, 4);
    }
    if (name.EqualsLiteral("width")) expr.mFeature = Expression::Width;
    else if (name.EqualsLiteral("height")) expr.mFeature = Expression::Height;
    else if (name.EqualsLiteral("orientation") && expr.mRange == Expression::Equal)
      expr.mFeature = Expression::Orientation;
    else return PR_FALSE; // Unknown features make this query "not all".
    if (!Next()) return PR_FALSE;
    if (mToken.IsSymbol(':')) {
      expr.mHasValue = PR_TRUE;
      mQuery.mText.AppendLiteral(": ");
      if (!Next()) return PR_FALSE;
      if (expr.mFeature == Expression::Orientation) {
        if (!Ident("portrait") && !Ident("landscape")) return PR_FALSE;
        expr.mLandscape = Ident("landscape");
      } else {
        if (!mToken.IsDimension() || !(mToken.mNumber >= 0 &&
                                      mToken.mNumber <= FLT_MAX)) return PR_FALSE;
        expr.mValue = mToken.mNumber;
        if (mToken.mType == eCSSToken_Dimension) {
          const nsString& unit = mToken.mIdent;
          if (unit.EqualsLiteral("em")) expr.mUnit = Expression::Em;
          else if (unit.EqualsLiteral("ex")) expr.mUnit = Expression::Ex;
          else if (unit.EqualsLiteral("in")) expr.mValue *= 96;
          else if (unit.EqualsLiteral("cm")) expr.mValue *= 96 / 2.54;
          else if (unit.EqualsLiteral("mm")) expr.mValue *= 96 / 25.4;
          else if (unit.EqualsLiteral("pt")) expr.mValue *= 96 / 72.0;
          else if (unit.EqualsLiteral("pc")) expr.mValue *= 16;
          else if (!unit.EqualsLiteral("px")) return PR_FALSE;
        }
      }
      mToken.AppendToString(mQuery.mText);
      if (!Next()) return PR_FALSE;
    } else if (expr.mRange != Expression::Equal) {
      return PR_FALSE;
    }
    if (!mToken.IsSymbol(')')) return PR_FALSE;
    mQuery.mText.Append(PRUnichar(')'));
    if (!mQuery.mExpressions.AppendElement(expr)) {
      mError = NS_ERROR_OUT_OF_MEMORY;
      return PR_FALSE;
    }
    return PR_TRUE;
  }
  nsMediaQuery& mQuery;
  nsCSSScanner mScanner;
  nsCSSToken mToken;
  nsresult mError;
};

nsresult
nsMediaQuery::Parse(const nsAString& aText)
{
  MediaQueryParser parser(*this);
  return parser.Parse(aText);
}

PRBool
nsMediaQuery::Matches(nsPresContext* aContext) const
{
  if (!mValid) return PR_FALSE;
  if (!mExpressions.IsEmpty()) aContext->SetHasViewportMediaQueries();
  PRBool match = mType == nsLayoutAtoms::all || mType == aContext->Medium();
  nsRect viewport = aContext->GetVisibleArea();
  for (PRUint32 i = 0; match && i < mExpressions.Length(); ++i) {
    const Expression& expr = mExpressions[i];
    if (expr.mFeature == Expression::Orientation) {
      match = !expr.mHasValue ||
              expr.mLandscape == (viewport.width > viewport.height);
      continue;
    }
    double actual = expr.mFeature == Expression::Width ? viewport.width : viewport.height;
    if (!expr.mHasValue) { match = actual != 0; continue; }
    double value = expr.mValue;
    if (expr.mUnit == Expression::Pixels) {
      value *= aContext->ScaledPixelsToTwips();
    } else {
      const nsFont* font = aContext->GetDefaultFont(kPresContext_DefaultVariableFont_ID);
      if (expr.mUnit == Expression::Em) {
        value *= font->size;
      } else {
        nsCOMPtr<nsIFontMetrics> metrics;
        metrics = aContext->GetMetricsFor(*font);
        if (!metrics) return PR_FALSE;
        nscoord xHeight;
        metrics->GetXHeight(xHeight);
        value *= xHeight;
      }
    }
    if (expr.mRange == Expression::Minimum) match = actual >= value;
    else if (expr.mRange == Expression::Maximum) match = actual <= value;
    else match = actual == value;
  }
  return mNegated ? !match : match;
}
