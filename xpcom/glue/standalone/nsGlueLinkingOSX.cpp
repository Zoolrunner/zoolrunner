/* -*- Mode: C++; tab-width: 6; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is Mozilla XPCOM.
 *
 * The Initial Developer of the Original Code is
 * Benjamin Smedberg <benjamin@smedbergs.us>
 *
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include "nsGlueLinking.h"
#include "nsXPCOMGlue.h"

#include <mach-o/dyld.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <AvailabilityMacros.h>
#include <sys/stat.h>

#ifndef NSADDIMAGE_OPTION_MATCH_FILENAME_BY_INSTALLNAME
#define NSADDIMAGE_OPTION_MATCH_FILENAME_BY_INSTALLNAME 0
#endif

static const mach_header* sXULLibImage;

static const mach_header*
LoadImage(const char *path)
{
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1010
    if (!NSAddLibraryWithSearching(path))
        return nsnull;
    struct stat requested;
    if (stat(path, &requested) == 0) {
        unsigned long count = _dyld_image_count();
        for (unsigned long i = 0; i < count; ++i) {
            struct stat candidate;
            if (stat(_dyld_get_image_name(i), &candidate) == 0 &&
                candidate.st_dev == requested.st_dev &&
                candidate.st_ino == requested.st_ino)
                return _dyld_get_image_header(i);
        }
    }
    // The library is loaded. LookupSymbol can use the global table when the
    // original dyld found it through its search path rather than this pathname.
    return nsnull;
#else
    return NSAddImage(path, NSADDIMAGE_OPTION_RETURN_ON_ERROR |
                           NSADDIMAGE_OPTION_WITH_SEARCHING |
                           NSADDIMAGE_OPTION_MATCH_FILENAME_BY_INSTALLNAME);
#endif
}

static void
ReadDependentCB(const char *aDependentLib)
{
    (void) LoadImage(aDependentLib);
}

static void*
LookupSymbol(const mach_header* aLib, const char* aSymbolName)
{
    // Try to use |NSLookupSymbolInImage| since it is faster than searching
    // the global symbol table.  If we couldn't get a mach_header pointer
    // for the XPCOM dylib, then use |NSLookupAndBindSymbol| to search the
    // global symbol table (this shouldn't normally happen, unless the user
    // has called XPCOMGlueStartup(".") and relies on libxpcom.dylib
    // already being loaded).
    NSSymbol sym = nsnull;
    if (aLib) {
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1010
        unsigned long count = _dyld_image_count();
        for (unsigned long i = 0; i < count; ++i) {
            if (_dyld_get_image_header(i) == aLib) {
                const char *hint = _dyld_get_image_name(i);
                if (NSIsSymbolNameDefinedWithHint(aSymbolName, hint))
                    sym = NSLookupAndBindSymbolWithHint(aSymbolName, hint);
                break;
            }
        }
#else
        sym = NSLookupSymbolInImage(aLib, aSymbolName,
                                 NSLOOKUPSYMBOLINIMAGE_OPTION_BIND |
                                 NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
#endif
    } else {
        if (NSIsSymbolNameDefined(aSymbolName))
            sym = NSLookupAndBindSymbol(aSymbolName);
    }
    if (!sym)
        return nsnull;

    return NSAddressOfSymbol(sym);
}

GetFrozenFunctionsFunc
XPCOMGlueLoad(const char *xpcomFile)
{
    const mach_header* lib = nsnull;

    if (xpcomFile[0] != '.' || xpcomFile[1] != '\0') {
        char xpcomDir[PATH_MAX];
        if (realpath(xpcomFile, xpcomDir)) {
            char *lastSlash = strrchr(xpcomDir, '/');
            if (lastSlash) {
                *lastSlash = '\0';

                XPCOMGlueLoadDependentLibs(xpcomDir, ReadDependentCB);

                snprintf(lastSlash, PATH_MAX - strlen(xpcomDir), "/" XUL_DLL);

                sXULLibImage = LoadImage(xpcomDir);
            }
        }

        lib = LoadImage(xpcomFile);
    }

    return (GetFrozenFunctionsFunc) LookupSymbol(lib, "_NS_GetFrozenFunctions");
}

void
XPCOMGlueUnload()
{
  // nothing to do, since we cannot unload dylibs on OS X
}

nsresult
XPCOMGlueLoadXULFunctions(const nsDynamicFunctionLoad *symbols)
{
    nsresult rv = NS_OK;
    while (symbols->functionName) {
        char buffer[512];
        snprintf(buffer, sizeof(buffer), "_%s", symbols->functionName);

        *symbols->function = (NSFuncPtr) LookupSymbol(sXULLibImage, buffer);
        if (!*symbols->function)
            rv = NS_ERROR_LOSS_OF_SIGNIFICANT_DATA;

        ++symbols;
    }

    return rv;
}
