/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
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
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"), in
 * which case the provisions of the GPL or the LGPL are applicable instead of
 * those above.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsScreenManagerCocoa_h_
#define nsScreenManagerCocoa_h_

#include "nsIScreenManager.h"

class nsScreenManagerCocoa : public nsIScreenManager
{
public:
  nsScreenManagerCocoa();
  virtual ~nsScreenManagerCocoa();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISCREENMANAGER
};

#endif // nsScreenManagerCocoa_h_
