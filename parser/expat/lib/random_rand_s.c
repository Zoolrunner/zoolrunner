/*
                            __  __            _
                         ___\ \/ /_ __   __ _| |_
                        / _ \\  /| '_ \ / _` | __|
                       |  __//  \| |_) | (_| | |_
                        \___/_/\_\ .__/ \__,_|\__|
                                 |_| XML parser

   Copyright (c) 2019      David Loffredo <loffredo@steptools.com>
   Copyright (c) 2019-2026 Sebastian Pipping <sebastian@pipping.org>
   Copyright (c) 2019      Ben Wagner <bungeman@chromium.org>
   Copyright (c) 2019      Vadim Zeitlin <vadim@zeitlins.org>
   Copyright (c) 2026      Matthew Fernandez <matthew.fernandez@gmail.com>
   Licensed under the MIT license:

   Permission is  hereby granted,  free of charge,  to any  person obtaining
   a  copy  of  this  software   and  associated  documentation  files  (the
   "Software"),  to  deal in  the  Software  without restriction,  including
   without  limitation the  rights  to use,  copy,  modify, merge,  publish,
   distribute, sublicense, and/or sell copies of the Software, and to permit
   persons  to whom  the Software  is  furnished to  do so,  subject to  the
   following conditions:

   The above copyright  notice and this permission notice  shall be included
   in all copies or substantial portions of the Software.

   THE  SOFTWARE  IS  PROVIDED  "AS  IS",  WITHOUT  WARRANTY  OF  ANY  KIND,
   EXPRESS  OR IMPLIED,  INCLUDING  BUT  NOT LIMITED  TO  THE WARRANTIES  OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
   NO EVENT SHALL THE AUTHORS OR  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR  OTHER LIABILITY, WHETHER  IN AN  ACTION OF CONTRACT,  TORT OR
   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   USE OR OTHER DEALINGS IN THE SOFTWARE.

   SPDX-License-Identifier: MIT
*/

#include "random_rand_s.h"

#include <windows.h>

/* Obtain entropy from the same RtlGenRandom entry point used internally by
 * rand_s().  Calling the VC8 CRT's rand_s() on systems without that entry
 * point invokes the CRT invalid-parameter handler and terminates the process.
 * Resolving it here lets Expat use its existing fallback on Windows 95 and
 * Windows NT 4 while retaining high-quality entropy on systems that provide
 * RtlGenRandom.
 */
bool
writeRandomBytes_rand_s(void *target, size_t count) {
  typedef BOOLEAN(WINAPI * RtlGenRandomFunction)(PVOID, ULONG);
  HMODULE advapi32 = LoadLibraryA("advapi32.dll");
  RtlGenRandomFunction rtlGenRandom;
  BOOLEAN result;

  if (advapi32 == NULL)
    return false;

  rtlGenRandom = (RtlGenRandomFunction)GetProcAddress(advapi32,
                                                       "SystemFunction036");
  if (rtlGenRandom == NULL) {
    FreeLibrary(advapi32);
    return false;
  }

  result = rtlGenRandom(target, (ULONG)count);
  FreeLibrary(advapi32);
  return result != FALSE;
}
