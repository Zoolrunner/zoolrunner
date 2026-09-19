/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=2 sw=4 et tw=80:
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   John Bandhauer <jband@netscape.com>
 *   Pierre Phaneuf <pp@ludusdesign.com>
 *   IBM Corp.
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

/* XPConnect JavaScript interactive shell. */

#include <stdio.h>
#include "nsIXPConnect.h"
#include "nsIXPCScriptable.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIXPCScriptable.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "jsapi.h"
#include "jsprf.h"
#include "nscore.h"
#include "nsMemory.h"
#include "nsIGenericFactory.h"
#include "nsIJSRuntimeService.h"
#include "nsCOMPtr.h"
#ifdef MOZ_XPCSHELL_META_LIBRARY
#include "nsXPCOM.h"
#include "prlink.h"
#endif
#include "nsIXPCSecurityManager.h"

#ifndef XPCONNECT_STANDALONE
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#endif

// all this crap is needed to do the interactive shell stuff
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#if defined(XP_WIN) || defined(XP_OS2)
#include <io.h>     /* for isatty() */
#elif defined(XP_UNIX) || defined(XP_BEOS)
#include <unistd.h>     /* for isatty() */
#endif

#include "nsIJSContextStack.h"

/***************************************************************************/

#ifdef JS_THREADSAFE
#define DoBeginRequest(cx) JS_BeginRequest((cx))
#define DoEndRequest(cx)   JS_EndRequest((cx))
#else
#define DoBeginRequest(cx) ((void)0)
#define DoEndRequest(cx)   ((void)0)
#endif

/***************************************************************************/

#define EXITCODE_RUNTIME_ERROR 3
#define EXITCODE_FILE_NOT_FOUND 4

FILE *gOutFile = NULL;
FILE *gErrFile = NULL;

int gExitCode = 0;
JSBool gQuitting = JS_FALSE;
static JSBool reportWarnings = JS_TRUE;
static JSBool compileOnly = JS_FALSE;

JSPrincipals *gJSPrincipals = nsnull;

JS_STATIC_DLL_CALLBACK(void)
my_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    int i, j, k, n;
    char *prefix = NULL, *tmp;
    const char *ctmp;

    if (!report) {
        fprintf(gErrFile, "%s\n", message);
        return;
    }

    /* Conditionally ignore reported warnings. */
    if (JSREPORT_IS_WARNING(report->flags) && !reportWarnings)
        return;

    if (report->filename)
        prefix = JS_smprintf("%s:", report->filename);
    if (report->lineno) {
        tmp = prefix;
        prefix = JS_smprintf("%s%u: ", tmp ? tmp : "", report->lineno);
        JS_free(cx, tmp);
    }
    if (JSREPORT_IS_WARNING(report->flags)) {
        tmp = prefix;
        prefix = JS_smprintf("%s%swarning: ",
                             tmp ? tmp : "",
                             JSREPORT_IS_STRICT(report->flags) ? "strict " : "");
        JS_free(cx, tmp);
    }

    /* embedded newlines -- argh! */
    while ((ctmp = strchr(message, '\n')) != 0) {
        ctmp++;
        if (prefix) fputs(prefix, gErrFile);
        fwrite(message, 1, ctmp - message, gErrFile);
        message = ctmp;
    }
    /* If there were no filename or lineno, the prefix might be empty */
    if (prefix)
        fputs(prefix, gErrFile);
    fputs(message, gErrFile);

    if (!report->linebuf) {
        fputc('\n', gErrFile);
        goto out;
    }

    fprintf(gErrFile, ":\n%s%s\n%s", prefix, report->linebuf, prefix);
    n = report->tokenptr - report->linebuf;
    for (i = j = 0; i < n; i++) {
        if (report->linebuf[i] == '\t') {
            for (k = (j + 8) & ~7; j < k; j++) {
                fputc('.', gErrFile);
            }
            continue;
        }
        fputc('.', gErrFile);
        j++;
    }
    fputs("^\n", gErrFile);
 out:
    if (!JSREPORT_IS_WARNING(report->flags))
        gExitCode = EXITCODE_RUNTIME_ERROR;
    JS_free(cx, prefix);
}

JS_STATIC_DLL_CALLBACK(JSBool)
Print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i, n;
    JSString *str;

    for (i = n = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        fprintf(gOutFile, "%s%s", i ? " " : "", JS_GetStringBytes(str));
    }
    n++;
    if (n)
        fputc('\n', gOutFile);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
Dump(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    if (!argc)
        return JS_TRUE;
    
    str = JS_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;

    char *bytes = JS_GetStringBytes(str);
    bytes = strdup(bytes);

    fputs(bytes, gOutFile);
    free(bytes);
    return JS_TRUE;
}

/* Compile Unicode source as a global script, without eval's scope semantics
 * or the historical file loader's byte-to-code-unit conversion. */
JS_STATIC_DLL_CALLBACK(JSBool)
Evaluate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *source, *name;
    JSScript *script;
    const char *filename = "evaluate";
    JSBool ok;

    source = JS_ValueToString(cx, argc ? argv[0] : JSVAL_VOID);
    if (!source)
        return JS_FALSE;
    argv[0] = STRING_TO_JSVAL(source);
    if (argc > 1) {
        name = JS_ValueToString(cx, argv[1]);
        if (!name)
            return JS_FALSE;
        argv[1] = STRING_TO_JSVAL(name);
        filename = JS_GetStringBytes(name);
        if (!filename)
            return JS_FALSE;
    }
    script = JS_CompileUCScriptForPrincipals(cx, obj, gJSPrincipals,
                                            JS_GetStringChars(source),
                                            JS_GetStringLength(source), filename, 1);
    if (!script)
        return JS_FALSE;
    ok = compileOnly || JS_ExecuteScript(cx, obj, script, rval);
    JS_DestroyScript(cx, script);
    return ok;
}

/* Module records are explicit host objects; ordinary load/evaluate keep
 * their historical script behavior. The caller supplies dependency records. */
static JSBool
ModuleArgument(JSContext *cx, uintN argc, jsval *argv, uintN index)
{
    if (argc <= index || JSVAL_IS_PRIMITIVE(argv[index])) {
        JS_ReportError(cx, "expected a module record");
        return JS_FALSE;
    }
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
CompileModule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *source, *name;
    JSObject *module;
    const char *filename = "module";
    source = JS_ValueToString(cx, argc ? argv[0] : JSVAL_VOID);
    if (!source) return JS_FALSE;
    argv[0] = STRING_TO_JSVAL(source);
    if (argc > 1) {
        name = JS_ValueToString(cx, argv[1]);
        if (!name) return JS_FALSE;
        argv[1] = STRING_TO_JSVAL(name);
        filename = JS_GetStringBytes(name);
        if (!filename) return JS_FALSE;
    }
    module = JS_CompileUCModule(cx, obj, gJSPrincipals,
               JS_GetStringChars(source), JS_GetStringLength(source), filename, 1);
    if (!module) return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(module);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
ModuleRequests(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *array;
    if (!ModuleArgument(cx, argc, argv, 0)) return JS_FALSE;
    array = JS_GetModuleRequests(cx, JSVAL_TO_OBJECT(argv[0]));
    if (!array) return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(array);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
LinkModule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *name;
    if (!ModuleArgument(cx, argc, argv, 0) || !ModuleArgument(cx, argc, argv, 2))
        return JS_FALSE;
    name = JS_ValueToString(cx, argv[1]);
    if (!name) return JS_FALSE;
    argv[1] = STRING_TO_JSVAL(name);
    *rval = JSVAL_VOID;
    return JS_SetModuleDependency(cx, JSVAL_TO_OBJECT(argv[0]), name,
                                   JSVAL_TO_OBJECT(argv[2]));
}

JS_STATIC_DLL_CALLBACK(JSBool)
InstantiateModule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval = JSVAL_VOID;
    return ModuleArgument(cx, argc, argv, 0) &&
           JS_InstantiateModule(cx, JSVAL_TO_OBJECT(argv[0]));
}

JS_STATIC_DLL_CALLBACK(JSBool)
EvaluateModule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval = JSVAL_VOID;
    return ModuleArgument(cx, argc, argv, 0) &&
           JS_EvaluateModule(cx, JSVAL_TO_OBJECT(argv[0]));
}

JS_STATIC_DLL_CALLBACK(JSBool)
ModuleNamespace(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *ns;
    if (!ModuleArgument(cx, argc, argv, 0)) return JS_FALSE;
    ns = JS_GetModuleNamespace(cx, JSVAL_TO_OBJECT(argv[0]));
    if (!ns) return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(ns);
    return JS_TRUE;
}

#if defined(_MSC_VER) && !defined(_DLL)
static JSScript*
CompileLocalFile(JSContext *cx, JSObject *obj, const char *filename, FILE *file)
{
    // With /MT, this executable and the JS DLL have separate CRT stream
    // tables and locks. Read and close our FILE here; only bytes cross JSAPI.
    char *source = NULL;
    size_t length = 0, capacity = 0;
    JSScript *script = NULL;
    for (;;) {
        if (length == capacity) {
            if (capacity > (size_t)INT_MAX / 2) {
                JS_ReportError(cx, "Script file is too large: %s",
                               filename ? filename : "<stdin>");
                break;
            }
            size_t next = capacity ? capacity * 2 : 8192;
            char *buffer = (char*)JS_realloc(cx, source, next);
            if (!buffer)
                break;
            source = buffer;
            capacity = next;
        }
        size_t count = fread(source + length, 1, capacity - length, file);
        length += count;
        if (!count) {
            if (ferror(file))
                JS_ReportError(cx, "Cannot read script file: %s",
                               filename ? filename : "<stdin>");
            else
                script = JS_CompileScriptForPrincipals(cx, obj, gJSPrincipals,
                                                       source, length,
                                                       filename, 1);
            break;
        }
    }
    JS_free(cx, source);
    fclose(file);
    return script;
}
#endif

JS_STATIC_DLL_CALLBACK(JSBool)
Load(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i;
    JSString *str;
    const char *filename;
    JSScript *script;
    JSBool ok;
    jsval result;
    FILE *file;

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        argv[i] = STRING_TO_JSVAL(str);
        filename = JS_GetStringBytes(str);
        if (!filename)
            return JS_FALSE;
        file = fopen(filename, "r");
        if (!file) {
            JS_ReportError(cx, "Cannot open script file %s: %s",
                           filename, strerror(errno));
            return JS_FALSE;
        }
#if defined(_MSC_VER) && !defined(_DLL)
        script = CompileLocalFile(cx, obj, filename, file);
#else
        script = JS_CompileFileHandleForPrincipals(cx, obj, filename, file,
                                                  gJSPrincipals);
#endif
        if (!script)
            ok = JS_FALSE;
        else {
            ok = !compileOnly
                 ? JS_ExecuteScript(cx, obj, script, &result)
                 : JS_TRUE;
            JS_DestroyScript(cx, script);
        }
        if (!ok)
            return JS_FALSE;
    }
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
Version(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (argc > 0 && JSVAL_IS_INT(argv[0]))
        *rval = INT_TO_JSVAL(JS_SetVersion(cx, JSVersion(JSVAL_TO_INT(argv[0]))));
    else
        *rval = INT_TO_JSVAL(JS_GetVersion(cx));
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
BuildDate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    fprintf(gOutFile, "built on %s at %s\n", __DATE__, __TIME__);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
Quit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
#ifdef LIVECONNECT
    JSJ_SimpleShutdown();
#endif

    gExitCode = 0;
    JS_ConvertArguments(cx, argc, argv,"/ i", &gExitCode);

    gQuitting = JS_TRUE;
//    exit(0);
    return JS_FALSE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
DumpXPC(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int32 depth = 2;

    if (argc > 0) {
        if (!JS_ValueToInt32(cx, argv[0], &depth))
            return JS_FALSE;
    }

    nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID());
    if(xpc)
        xpc->DebugDump((int16)depth);
    return JS_TRUE;
}

/* XXX needed only by GC() */
#include "jscntxt.h"

#ifdef GC_MARK_DEBUG
extern "C" JS_FRIEND_DATA(FILE *) js_DumpGCHeap;
#endif

JS_STATIC_DLL_CALLBACK(JSBool)
GC(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSRuntime *rt;
    uint32 preBytes;

    rt = cx->runtime;
    preBytes = rt->gcBytes;
#ifdef GC_MARK_DEBUG
    if (argc && JSVAL_IS_STRING(argv[0])) {
        char *name = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
        FILE *file = fopen(name, "w");
        if (!file) {
            fprintf(gErrFile, "gc: can't open %s: %s\n", strerror(errno));
            return JS_FALSE;
        }
        js_DumpGCHeap = file;
    } else {
        js_DumpGCHeap = stdout;
    }
#endif
    JS_GC(cx);
#ifdef GC_MARK_DEBUG
    if (js_DumpGCHeap != stdout)
        fclose(js_DumpGCHeap);
    js_DumpGCHeap = NULL;
#endif
    fprintf(gOutFile, "before %lu, after %lu, break %08lx\n",
           (unsigned long)preBytes, (unsigned long)rt->gcBytes,
#ifdef XP_UNIX
           (unsigned long)sbrk(0)
#else
           0
#endif
           );
#ifdef JS_GCMETER
    js_DumpGCStats(rt, stdout);
#endif
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
Clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (argc > 0 && !JSVAL_IS_PRIMITIVE(argv[0])) {
        JS_ClearScope(cx, JSVAL_TO_OBJECT(argv[0]));
    } else {
        JS_ReportError(cx, "'clear' requires an object");
        return JS_FALSE;
    }    
    return JS_TRUE;
}

static JSBool EnqueueJob(JSContext *, JSObject *, uintN, jsval *, jsval *);
static JSBool DrainJobQueue(JSContext *, JSObject *, uintN, jsval *, jsval *);

/* Test262 host realms keep tested globals separate from shell bookkeeping. */
static JSClass test262GlobalClass = {
    "Test262 global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
RealmCall(JSContext *cx, uintN argc, jsval *argv, jsval *rval, JSNative native)
{
    JSFunction *fun = JS_ValueToFunction(cx, argv[-2]);
    JSObject *global, *previous = JS_GetGlobalObject(cx);
    JSVersion version;
    JSBool ok;
    if (!fun) return JS_FALSE;
    global = JS_GetParent(cx, JS_GetFunctionObject(fun));
    if (!global || !JS_AddNamedRoot(cx, &previous, "previous host realm"))
        return JS_FALSE;
    version = JS_SetVersion(cx, JSVERSION_ECMA_2015);
    JS_SetGlobalObject(cx, global);
    ok = native(cx, global, argc, argv, rval);
    JS_SetGlobalObject(cx, previous);
    JS_SetVersion(cx, version);
    JS_RemoveRoot(cx, &previous);
    return ok;
}

/* Keep compiled scripts opaque and tied to their compilation realm. */
static JSClass test262ScriptClass = {
    "Test262 script", JSCLASS_HAS_RESERVED_SLOTS(1),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
CompileRealmScript(JSContext *cx, JSObject *global, uintN argc, jsval *argv, jsval *rval)
{
    JSString *source;
    JSScript *script;
    JSObject *record, *owner;
    source = JS_ValueToString(cx, argc ? argv[0] : JSVAL_VOID);
    if (!source) return JS_FALSE;
    argv[0] = STRING_TO_JSVAL(source);
    record = JS_NewObject(cx, &test262ScriptClass, NULL, global);
    if (!record) return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(record);
    if (!JS_SetPrototype(cx, record, NULL)) return JS_FALSE;
    script = JS_CompileUCScriptForPrincipals(cx, global, gJSPrincipals,
        JS_GetStringChars(source), JS_GetStringLength(source), "test262-script", 1);
    if (!script) return JS_FALSE;
    owner = JS_NewScriptObject(cx, script);
    if (!owner) {
        JS_DestroyScript(cx, script);
        return JS_FALSE;
    }
    return JS_SetReservedSlot(cx, record, 0, OBJECT_TO_JSVAL(owner));
}

static JSBool
ExecuteRealmScript(JSContext *cx, JSObject *global, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *record;
    jsval owner;
    if (!argc || JSVAL_IS_PRIMITIVE(argv[0]) ||
        JS_GET_CLASS(cx, JSVAL_TO_OBJECT(argv[0])) != &test262ScriptClass) {
        JS_ReportError(cx, "expected a Test262 compiled script");
        return JS_FALSE;
    }
    record = JSVAL_TO_OBJECT(argv[0]);
    if (JS_GetParent(cx, record) != global) {
        JS_ReportError(cx, "compiled script belongs to another realm");
        return JS_FALSE;
    }
    if (!JS_GetReservedSlot(cx, record, 0, &owner)) return JS_FALSE;
    return JS_ExecuteScript(cx, global,
        (JSScript *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(owner)), rval);
}

JS_STATIC_DLL_CALLBACK(JSBool)
RealmCompileScript(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    return RealmCall(cx, argc, argv, rval, CompileRealmScript);
}
JS_STATIC_DLL_CALLBACK(JSBool)
RealmExecuteScript(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    return RealmCall(cx, argc, argv, rval, ExecuteRealmScript);
}

JS_STATIC_DLL_CALLBACK(JSBool)
RealmEvaluate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    return RealmCall(cx, argc, argv, rval, Evaluate);
}
JS_STATIC_DLL_CALLBACK(JSBool)
RealmCompileModule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    return RealmCall(cx, argc, argv, rval, CompileModule);
}
JS_STATIC_DLL_CALLBACK(JSBool)
RealmEvaluateModule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    return RealmCall(cx, argc, argv, rval, EvaluateModule);
}
JS_STATIC_DLL_CALLBACK(JSBool)
RealmInstantiateModule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    return RealmCall(cx, argc, argv, rval, InstantiateModule);
}
JS_STATIC_DLL_CALLBACK(JSBool)
RealmLinkModule(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    return RealmCall(cx, argc, argv, rval, LinkModule);
}

JS_STATIC_DLL_CALLBACK(JSBool)
DetachArrayBuffer(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (!argc || JSVAL_IS_PRIMITIVE(argv[0])) {
        /* Let the engine supply the standard TypeError for a wrong object. */
        return JS_DetachArrayBuffer(cx, NULL);
    }
    *rval = JSVAL_VOID;
    return JS_DetachArrayBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
}

JS_STATIC_DLL_CALLBACK(JSBool)
RealmDetachArrayBuffer(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    return RealmCall(cx, argc, argv, rval, DetachArrayBuffer);
}

JS_STATIC_DLL_CALLBACK(JSBool)
CreateTest262Realm(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *global = NULL, *host = NULL, *previous = JS_GetGlobalObject(cx);
    jsval value = JSVAL_VOID;
    JSVersion version = JS_GetVersion(cx);
    JSFunction *fun;
    JSBool ok = JS_FALSE;
    unsigned rooted = 0, i;
    void *addresses[] = { &global, &host, &previous, &value };
    const char *names[] = { "test realm global", "test realm host", "prior realm", "host method" };
    static const struct {
        const char *name;
        JSNative native;
        uintN nargs;
    } methods[] = {
        { "evalScript", RealmEvaluate, 1 },
        { "compileScript", RealmCompileScript, 1 },
        { "executeScript", RealmExecuteScript, 1 },
        { "createRealm", CreateTest262Realm, 0 },
        { "gc", GC, 0 },
        { "detachArrayBuffer", RealmDetachArrayBuffer, 1 },
        { "compileModule", RealmCompileModule, 1 },
        { "instantiateModule", RealmInstantiateModule, 1 },
        { "evaluateModule", RealmEvaluateModule, 1 },
        { "linkModule", RealmLinkModule, 3 }
    };
    for (i = 0; i < 4; ++i) {
        if (!JS_AddNamedRoot(cx, addresses[i], names[i])) goto out;
        ++rooted;
    }
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &test262GlobalClass, NULL, NULL);
    if (!global || !JS_SetParent(cx, global, NULL) || !JS_SetPrototype(cx, global, NULL))
        goto out;
    JS_SetGlobalObject(cx, global);
    if (!JS_InitStandardClasses(cx, global)) goto out;
    host = JS_NewObject(cx, NULL, NULL, global);
    if (!host ||
        !JS_DefineProperty(cx, host, "global", OBJECT_TO_JSVAL(global), NULL, NULL, JSPROP_ENUMERATE) ||
        !JS_DefineProperty(cx, global, "$262", OBJECT_TO_JSVAL(host), NULL, NULL, 0) ||
        !JS_DefineFunction(cx, global, "print", Print, 0, 0)) goto out;
    for (i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i) {
        fun = JS_NewFunction(cx, methods[i].native, methods[i].nargs, 0, global, methods[i].name);
        if (!fun) goto out;
        value = OBJECT_TO_JSVAL(JS_GetFunctionObject(fun));
        if (!JS_DefineProperty(cx, host, methods[i].name, value, NULL, NULL, JSPROP_ENUMERATE))
            goto out;
    }
    *rval = OBJECT_TO_JSVAL(host);
    ok = JS_TRUE;
 out:
    JS_SetGlobalObject(cx, previous);
    JS_SetVersion(cx, version);
    while (rooted) JS_RemoveRoot(cx, addresses[--rooted]);
    return ok;
}


static JSFunctionSpec glob_functions[] = {
    {"createTest262Realm", CreateTest262Realm, 0, 0, 0},
    {"enqueueJob",      EnqueueJob,     1},
    {"drainJobQueue",   DrainJobQueue,  0},
    {"print",           Print,          0},
    {"load",            Load,           1},
    {"evaluate",        Evaluate,       1},
    {"compileModule",   CompileModule,  1},
    {"moduleRequests",  ModuleRequests, 1},
    {"linkModule",      LinkModule,     3},
    {"instantiateModule", InstantiateModule, 1},
    {"evaluateModule",  EvaluateModule, 1},
    {"namespaceModule", ModuleNamespace, 1},
    {"quit",            Quit,           0},
    {"version",         Version,        1},
    {"build",           BuildDate,      0},
    {"dumpXPC",         DumpXPC,        1},
    {"dump",            Dump,           1},
    {"gc",              GC,             0},
    {"clear",           Clear,          1},
    {0}
};

JSClass global_class = {
    "global", 0,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub
};

static JSBool
env_setProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
/* XXX porting may be easy, but these don't seem to supply setenv by default */
#if !defined XP_BEOS && !defined XP_OS2 && !defined SOLARIS
    JSString *idstr, *valstr;
    const char *name, *value;
    int rv;

    idstr = JS_ValueToString(cx, id);
    valstr = JS_ValueToString(cx, *vp);
    if (!idstr || !valstr)
        return JS_FALSE;
    name = JS_GetStringBytes(idstr);
    value = JS_GetStringBytes(valstr);
#if defined XP_WIN || defined HPUX || defined OSF1 || defined IRIX
    {
        char *waste = JS_smprintf("%s=%s", name, value);
        if (!waste) {
            JS_ReportOutOfMemory(cx);
            return JS_FALSE;
        }
        rv = putenv(waste);
#ifdef XP_WIN
        /*
         * HPUX9 at least still has the bad old non-copying putenv.
         *
         * Per mail from <s.shanmuganathan@digital.com>, OSF1 also has a putenv
         * that will crash if you pass it an auto char array (so it must place
         * its argument directly in the char *environ[] array).
         */
        free(waste);
#endif
    }
#else
    rv = setenv(name, value, 1);
#endif
    if (rv < 0) {
        JS_ReportError(cx, "can't set envariable %s to %s", name, value);
        return JS_FALSE;
    }
    *vp = STRING_TO_JSVAL(valstr);
#endif /* !defined XP_BEOS && !defined XP_OS2 && !defined SOLARIS */
    return JS_TRUE;
}

static JSBool
env_enumerate(JSContext *cx, JSObject *obj)
{
    static JSBool reflected;
    char **evp, *name, *value;
    JSString *valstr;
    JSBool ok;

    if (reflected)
        return JS_TRUE;

    for (evp = (char **)JS_GetPrivate(cx, obj); (name = *evp) != NULL; evp++) {
        value = strchr(name, '=');
        if (!value)
            continue;
        *value++ = '\0';
        valstr = JS_NewStringCopyZ(cx, value);
        if (!valstr) {
            ok = JS_FALSE;
        } else {
            ok = JS_DefineProperty(cx, obj, name, STRING_TO_JSVAL(valstr),
                                   NULL, NULL, JSPROP_ENUMERATE);
        }
        value[-1] = '=';
        if (!ok)
            return JS_FALSE;
    }

    reflected = JS_TRUE;
    return JS_TRUE;
}

static JSBool
env_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
            JSObject **objp)
{
    JSString *idstr, *valstr;
    const char *name, *value;

    if (flags & JSRESOLVE_ASSIGNING)
        return JS_TRUE;

    idstr = JS_ValueToString(cx, id);
    if (!idstr)
        return JS_FALSE;
    name = JS_GetStringBytes(idstr);
    value = getenv(name);
    if (value) {
        valstr = JS_NewStringCopyZ(cx, value);
        if (!valstr)
            return JS_FALSE;
        if (!JS_DefineProperty(cx, obj, name, STRING_TO_JSVAL(valstr),
                               NULL, NULL, JSPROP_ENUMERATE)) {
            return JS_FALSE;
        }
        *objp = obj;
    }
    return JS_TRUE;
}

static JSClass env_class = {
    "environment", JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE,
    JS_PropertyStub,  JS_PropertyStub,
    JS_PropertyStub,  env_setProperty,
    env_enumerate, (JSResolveOp) env_resolve,
    JS_ConvertStub,   JS_FinalizeStub
};

/***************************************************************************/

typedef enum JSShellErrNum {
#define MSG_DEF(name, number, count, exception, format) \
    name = number,
#include "jsshell.msg"
#undef MSG_DEF
    JSShellErr_Limit
#undef MSGDEF
} JSShellErrNum;

JSErrorFormatString jsShell_ErrorFormatString[JSErr_Limit] = {
#if JS_HAS_DFLT_MSG_STRINGS
#define MSG_DEF(name, number, count, exception, format) \
    { format, count } ,
#else
#define MSG_DEF(name, number, count, exception, format) \
    { NULL, count } ,
#endif
#include "jsshell.msg"
#undef MSG_DEF
};

JS_STATIC_DLL_CALLBACK(const JSErrorFormatString *)
my_GetErrorMessage(void *userRef, const char *locale, const uintN errorNumber)
{
    if ((errorNumber > 0) && (errorNumber < JSShellErr_Limit))
            return &jsShell_ErrorFormatString[errorNumber];
        else
            return NULL;
}

#ifdef EDITLINE
extern "C" {
extern char     *readline(const char *prompt);
extern void     add_history(char *line);
}
#endif

static JSBool
GetLine(JSContext *cx, char *bufp, FILE *file, const char *prompt) {
#ifdef EDITLINE
    /*
     * Use readline only if file is stdin, because there's no way to specify
     * another handle.  Are other filehandles interactive?
     */
    if (file == stdin) {
        char *linep = readline(prompt);
        if (!linep)
            return JS_FALSE;
        if (*linep)
            add_history(linep);
        strcpy(bufp, linep);
        JS_free(cx, linep);
        bufp += strlen(bufp);
        *bufp++ = '\n';
        *bufp = '\0';
    } else
#endif
    {
        char line[256];
        fprintf(gOutFile, prompt);
        fflush(gOutFile);
        if (!fgets(line, sizeof line, file))
            return JS_FALSE;
        strcpy(bufp, line);
    }
    return JS_TRUE;
}

/* Shell hooks expose the engine queue without making them web globals. */
static JSBool
EnqueueJob(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval = JSVAL_VOID;
    if (!argc || JSVAL_IS_PRIMITIVE(argv[0])) {
        JS_ReportError(cx, "enqueueJob requires a callable object");
        return JS_FALSE;
    }
    return JS_EnqueueJob(cx, JSVAL_TO_OBJECT(argv[0]));
}

static JSBool
DrainJobQueue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval = JSVAL_VOID;
    return JS_RunJobs(cx);
}

/* Only command-line turns checkpoint automatically. Nested load/evaluate calls
 * must finish the surrounding script before its pending jobs can execute. */
static JSBool
RunShellJobs(JSContext *cx)
{
    if (gQuitting || !JS_HasPendingJobs(cx))
        return JS_TRUE;
    if (JS_RunJobs(cx))
        return JS_TRUE;
    if (JS_IsExceptionPending(cx))
        JS_ReportPendingException(cx);
    if (!gQuitting && !gExitCode)
        gExitCode = EXITCODE_RUNTIME_ERROR;
    return JS_FALSE;
}

static void
ProcessFile(JSContext *cx, JSObject *obj, const char *filename, FILE *file)
{
    JSScript *script;
    jsval result;
    int lineno, startline;
    JSBool ok, hitEOF;
    char *bufp, buffer[4096];
    JSString *str;

    if (!isatty(fileno(file))) {
        /*
         * It's not interactive - just execute it.
         *
         * Support the UNIX #! shell hack; gobble the first line if it starts
         * with '#'.  TODO - this isn't quite compatible with sharp variables,
         * as a legal js program (using sharp variables) might start with '#'.
         * But that would require multi-character lookahead.
         */
        int ch = fgetc(file);
        if (ch == '#') {
            while((ch = fgetc(file)) != EOF) {
                if(ch == '\n' || ch == '\r')
                    break;
            }
        }
        ungetc(ch, file);
        DoBeginRequest(cx);

#if defined(_MSC_VER) && !defined(_DLL)
        script = CompileLocalFile(cx, obj, filename, file);
#else
        script = JS_CompileFileHandleForPrincipals(cx, obj, filename, file,
                                                  gJSPrincipals);
#endif

        if (script) {
            if (!compileOnly) {
                (void)JS_ExecuteScript(cx, obj, script, &result);
                (void)RunShellJobs(cx);
            }
            JS_DestroyScript(cx, script);
        }
        DoEndRequest(cx);
        return;
    }

    /* It's an interactive filehandle; drop into read-eval-print loop. */
    lineno = 1;
    hitEOF = JS_FALSE;
    do {
        bufp = buffer;
        *bufp = '\0';

        /*
         * Accumulate lines until we get a 'compilable unit' - one that either
         * generates an error (before running out of source) or that compiles
         * cleanly.  This should be whenever we get a complete statement that
         * coincides with the end of a line.
         */
        startline = lineno;
        do {
            if (!GetLine(cx, bufp, file, startline == lineno ? "js> " : "")) {
                hitEOF = JS_TRUE;
                break;
            }
            bufp += strlen(bufp);
            lineno++;
        } while (!JS_BufferIsCompilableUnit(cx, obj, buffer, strlen(buffer)));
        
        DoBeginRequest(cx);
        /* Clear any pending exception from previous failed compiles.  */
        JS_ClearPendingException(cx);
        script = JS_CompileScriptForPrincipals(cx, obj, gJSPrincipals, buffer,
                                               strlen(buffer), "typein", startline);
        if (script) {
            JSErrorReporter older;

            if (!compileOnly) {
                ok = JS_ExecuteScript(cx, obj, script, &result);
                if (ok && result != JSVAL_VOID) {
                    /* Suppress error reports from JS_ValueToString(). */
                    older = JS_SetErrorReporter(cx, NULL);
                    str = JS_ValueToString(cx, result);
                    JS_SetErrorReporter(cx, older);
    
                    if (str)
                        fprintf(gOutFile, "%s\n", JS_GetStringBytes(str));
                    else
                        ok = JS_FALSE;
                }
                if (!RunShellJobs(cx))
                    ok = JS_FALSE;
#if 0
#if JS_HAS_ERROR_EXCEPTIONS
                /*
                 * Require that any time we return failure, an exception has
                 * been set.
                 */
                JS_ASSERT(ok || JS_IsExceptionPending(cx));
    
                /*
                 * Also that any time an exception has been set, we've
                 * returned failure.
                 */
                JS_ASSERT(!JS_IsExceptionPending(cx) || !ok);
#endif /* JS_HAS_ERROR_EXCEPTIONS */
#endif
            }
            JS_DestroyScript(cx, script);
        }
        DoEndRequest(cx);
    } while (!hitEOF && !gQuitting);
    fprintf(gOutFile, "\n");
    return;
}

static void
Process(JSContext *cx, JSObject *obj, const char *filename)
{
    FILE *file;

    if (!filename || strcmp(filename, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(filename, "r");
        if (!file) {
            JS_ReportErrorNumber(cx, my_GetErrorMessage, NULL,
                                 JSSMSG_CANT_OPEN,
                                 filename, strerror(errno));
            gExitCode = EXITCODE_FILE_NOT_FOUND;
            return;
        }
    }

    ProcessFile(cx, obj, filename, file);
}

static int
usage(void)
{
    fprintf(gErrFile, "%s\n", JS_GetImplementationVersion());
    fprintf(gErrFile, "usage: xpcshell [-EPswWxC] [-v version] [-f scriptfile] [-e script] [scriptfile] [scriptarg...]\n");
    return 2;
}

extern JSClass global_class;

static int
ProcessArgs(JSContext *cx, JSObject *obj, char **argv, int argc)
{
    const char rcfilename[] = "xpcshell.js";
    FILE *rcfile;
    int i, j, length;
    JSObject *argsObj;
    char *filename = NULL;
    JSBool isInteractive = JS_TRUE;

    rcfile = fopen(rcfilename, "r");
    if (rcfile) {
        printf("[loading '%s'...]\n", rcfilename);
        ProcessFile(cx, obj, rcfilename, rcfile);
    }

    /*
     * Scan past all optional arguments so we can create the arguments object
     * before processing any -f options, which must interleave properly with
     * -v and -w options.  This requires two passes, and without getopt, we'll
     * have to keep the option logic here and in the second for loop in sync.
     */
    for (i = 0; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == '\0') {
            ++i;
            break;
        }
        switch (argv[i][1]) {
          case 'v':
          case 'f':
          case 'e':
            ++i;
            break;
          default:;
        }
    }

    /*
     * Create arguments early and define it to root it, so it's safe from any
     * GC calls nested below, and so it is available to -f <file> arguments.
     */
    argsObj = JS_NewArrayObject(cx, 0, NULL);
    if (!argsObj)
        return 1;
    if (!JS_DefineProperty(cx, obj, "arguments", OBJECT_TO_JSVAL(argsObj),
                           NULL, NULL, 0)) {
        return 1;
    }

    length = argc - i;
    for (j = 0; j < length; j++) {
        JSString *str = JS_NewStringCopyZ(cx, argv[i++]);
        if (!str)
            return 1;
        if (!JS_DefineElement(cx, argsObj, j, STRING_TO_JSVAL(str),
                              NULL, NULL, JSPROP_ENUMERATE)) {
            return 1;
        }
    }

    for (i = 0; i < argc; i++) {
        if (argv[i][0] != '-' || argv[i][1] == '\0') {
            filename = argv[i++];
            isInteractive = JS_FALSE;
            break;
        }
        switch (argv[i][1]) {
        case 'E':
            JS_SetVersion(cx, JSVERSION_ECMA_2015);
            break;
        case 'v':
            if (++i == argc) {
                return usage();
            }
            JS_SetVersion(cx, JSVersion(atoi(argv[i])));
            break;
        case 'W':
            reportWarnings = JS_FALSE;
            break;
        case 'w':
            reportWarnings = JS_TRUE;
            break;
        case 's':
            JS_ToggleOptions(cx, JSOPTION_STRICT);
            break;
        case 'x':
            JS_ToggleOptions(cx, JSOPTION_XML);
            break;
        case 'P':
            if (JS_GET_CLASS(cx, JS_GetPrototype(cx, obj)) != &global_class) {
                JSObject *gobj;

                if (!JS_SealObject(cx, obj, JS_TRUE))
                    return JS_FALSE;
                gobj = JS_NewObject(cx, &global_class, NULL, NULL);
                if (!gobj)
                    return JS_FALSE;
                if (!JS_SetPrototype(cx, gobj, obj))
                    return JS_FALSE;
                JS_SetParent(cx, gobj, NULL);
                JS_SetGlobalObject(cx, gobj);
                obj = gobj;
            }
            break;
        case 'f':
            if (++i == argc) {
                return usage();
            }
            Process(cx, obj, argv[i]);
            /*
             * XXX: js -f foo.js should interpret foo.js and then
             * drop into interactive mode, but that breaks test
             * harness. Just execute foo.js for now.
             */
            isInteractive = JS_FALSE;
            break;

        case 'e':
        {
            jsval rval;

            if (++i == argc) {
                return usage();
            }

            DoBeginRequest(cx);
            JSBool evaluated = JS_EvaluateScriptForPrincipals(
                cx, obj, gJSPrincipals, argv[i], strlen(argv[i]), "-e", 1, &rval);
            if (!RunShellJobs(cx))
                evaluated = JS_FALSE;
            DoEndRequest(cx);
            if (!evaluated && !gQuitting && !gExitCode)
                gExitCode = EXITCODE_RUNTIME_ERROR;

            isInteractive = JS_FALSE;
            break;
        }
        case 'C':
            compileOnly = JS_TRUE;
            isInteractive = JS_FALSE;
            break;

        default:
            return usage();
        }
    }

    if (filename || isInteractive)
        Process(cx, obj, filename);
    return gExitCode;
}

/***************************************************************************/

class FullTrustSecMan : public nsIXPCSecurityManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSECURITYMANAGER
  FullTrustSecMan();
};

NS_IMPL_ISUPPORTS1(FullTrustSecMan, nsIXPCSecurityManager)

FullTrustSecMan::FullTrustSecMan()
{
}

NS_IMETHODIMP
FullTrustSecMan::CanCreateWrapper(JSContext * aJSContext, const nsIID & aIID, nsISupports *aObj, nsIClassInfo *aClassInfo, void * *aPolicy)
{
    return NS_OK;
}

NS_IMETHODIMP
FullTrustSecMan::CanCreateInstance(JSContext * aJSContext, const nsCID & aCID)
{
    return NS_OK;
}

NS_IMETHODIMP
FullTrustSecMan::CanGetService(JSContext * aJSContext, const nsCID & aCID)
{
    return NS_OK;
}

/* void CanAccess (in PRUint32 aAction, in nsIXPCNativeCallContext aCallContext, in JSContextPtr aJSContext, in JSObjectPtr aJSObject, in nsISupports aObj, in nsIClassInfo aClassInfo, in JSVal aName, inout voidPtr aPolicy); */
NS_IMETHODIMP 
FullTrustSecMan::CanAccess(PRUint32 aAction, nsIXPCNativeCallContext *aCallContext, JSContext * aJSContext, JSObject * aJSObject, nsISupports *aObj, nsIClassInfo *aClassInfo, jsval aName, void * *aPolicy)
{
    return NS_OK;
}

/***************************************************************************/

// #define TEST_InitClassesWithNewWrappedGlobal

#ifdef TEST_InitClassesWithNewWrappedGlobal
// XXX hacky test code...
#include "xpctest.h"

class TestGlobal : public nsIXPCTestNoisy, public nsIXPCScriptable
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTNOISY
    NS_DECL_NSIXPCSCRIPTABLE

    TestGlobal(){}
};

NS_IMPL_ISUPPORTS2(TestGlobal, nsIXPCTestNoisy, nsIXPCScriptable)

// The nsIXPCScriptable map declaration that will generate stubs for us...
#define XPC_MAP_CLASSNAME           TestGlobal
#define XPC_MAP_QUOTED_CLASSNAME   "TestGlobal"
#define XPC_MAP_FLAGS               nsIXPCScriptable::USE_JSSTUB_FOR_ADDPROPERTY |\
                                    nsIXPCScriptable::USE_JSSTUB_FOR_DELPROPERTY |\
                                    nsIXPCScriptable::USE_JSSTUB_FOR_SETPROPERTY
#include "xpc_map_end.h" /* This will #undef the above */

NS_IMETHODIMP TestGlobal::Squawk() {return NS_OK;}

#endif

// uncomment to install the test 'this' translator
// #define TEST_TranslateThis

#ifdef TEST_TranslateThis

#include "xpctest.h"

class nsXPCFunctionThisTranslator : public nsIXPCFunctionThisTranslator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCFUNCTIONTHISTRANSLATOR

  nsXPCFunctionThisTranslator();
  virtual ~nsXPCFunctionThisTranslator();
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsXPCFunctionThisTranslator, nsIXPCFunctionThisTranslator)

nsXPCFunctionThisTranslator::nsXPCFunctionThisTranslator()
{
  /* member initializers and constructor code */
}

nsXPCFunctionThisTranslator::~nsXPCFunctionThisTranslator()
{
  /* destructor code */
#ifdef DEBUG_jband
    printf("destroying nsXPCFunctionThisTranslator\n");
#endif
}

/* nsISupports TranslateThis (in nsISupports aInitialThis, in nsIInterfaceInfo aInterfaceInfo, in PRUint16 aMethodIndex, out PRBool aHideFirstParamFromJS, out nsIIDPtr aIIDOfResult); */
NS_IMETHODIMP 
nsXPCFunctionThisTranslator::TranslateThis(nsISupports *aInitialThis, 
                                           nsIInterfaceInfo *aInterfaceInfo, 
                                           PRUint16 aMethodIndex, 
                                           PRBool *aHideFirstParamFromJS, 
                                           nsIID * *aIIDOfResult, 
                                           nsISupports **_retval)
{
    NS_IF_ADDREF(aInitialThis);
    *_retval = aInitialThis;
    *aHideFirstParamFromJS = JS_FALSE;
    *aIIDOfResult = nsnull;
    return NS_OK;
}

#endif

int
main(int argc, char **argv, char **envp)
{
    JSRuntime *rt;
    JSContext *cx;
    JSObject *glob, *envobj;
    int result;
    nsresult rv;

    gErrFile = stderr;
    gOutFile = stdout;
#ifdef MOZ_XPCSHELL_META_LIBRARY
    // Keep the aggregate loaded for the process lifetime, as when the Suite
    // links it directly. xpcshell itself is built before this library exists.
    PRLibrary* aggregate = PR_LoadLibrary(MOZ_XPCSHELL_META_LIBRARY);
    nsStaticModuleInfo aggregateModule = { "mozcomps", nsnull };
    if (aggregate)
        aggregateModule.getModule = (nsGetModuleProc)
            PR_FindFunctionSymbol(aggregate, "nsMetaModule_nsGetModule");
    if (!aggregateModule.getModule) {
        fprintf(gErrFile, "Cannot load the XPCOM component aggregate: %s\n",
                MOZ_XPCSHELL_META_LIBRARY);
        return 1;
    }
#endif
    {
        nsCOMPtr<nsIServiceManager> servMan;
#ifdef MOZ_XPCSHELL_META_LIBRARY
        rv = NS_InitXPCOM3(getter_AddRefs(servMan), nsnull, nsnull,
                          &aggregateModule, 1);
#else
        rv = NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, nsnull);
#endif
        if (NS_FAILED(rv)) {
            printf("NS_InitXPCOM failed!\n");
            return 1;
        }
        {
            nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(servMan);
            NS_ASSERTION(registrar, "Null nsIComponentRegistrar");
            if (registrar)
                registrar->AutoRegister(nsnull);
        }

        nsCOMPtr<nsIJSRuntimeService> rtsvc = do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
        // get the JSRuntime from the runtime svc
        if (!rtsvc) {
            printf("failed to get nsJSRuntimeService!\n");
            return 1;
        }
    
        if (NS_FAILED(rtsvc->GetRuntime(&rt)) || !rt) {
            printf("failed to get JSRuntime from nsJSRuntimeService!\n");
            return 1;
        }

        cx = JS_NewContext(rt, 8192);
        if (!cx) {
            printf("JS_NewContext failed!\n");
            return 1;
        }

        JS_SetErrorReporter(cx, my_ErrorReporter);

        nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID());
        if (!xpc) {
            printf("failed to get nsXPConnect service!\n");
            return 1;
        }

        // Since the caps security system might set a default security manager
        // we will be sure that the secman on this context gives full trust.
        // That way we can avoid getting principals from the caps security manager
        // just to shut it up. Also, note that even though our secman will allow
        // anything, we set the flags to '0' so it ought never get called anyway.
        nsCOMPtr<nsIXPCSecurityManager> secman =
            NS_STATIC_CAST(nsIXPCSecurityManager*, new FullTrustSecMan());
        xpc->SetSecurityManagerForJSContext(cx, secman, 0);

        //    xpc->SetCollectGarbageOnMainThreadOnly(PR_TRUE);
        //    xpc->SetDeferReleasesUntilAfterGarbageCollection(PR_TRUE);

#ifndef XPCONNECT_STANDALONE
        // Fetch the system principal and store it away in a global, to use for
        // script compilation in Load() and ProcessFile() (including interactive
        // eval loop)
        {
            nsCOMPtr<nsIPrincipal> princ;

            nsCOMPtr<nsIScriptSecurityManager> securityManager =
                do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
            if (NS_SUCCEEDED(rv) && securityManager) {
                rv = securityManager->GetSystemPrincipal(getter_AddRefs(princ));
                if (NS_FAILED(rv)) {
                    fprintf(gErrFile, "+++ Failed to obtain SystemPrincipal from ScriptSecurityManager service.\n");
                } else {
                    // fetch the JS principals and stick in a global
                    rv = princ->GetJSPrincipals(cx, &gJSPrincipals);
                    if (NS_FAILED(rv)) {
                        fprintf(gErrFile, "+++ Failed to obtain JS principals from SystemPrincipal.\n");
                    }
                }
            } else {
                fprintf(gErrFile, "+++ Failed to get ScriptSecurityManager service, running without principals");
            }
        }
#endif

#ifdef TEST_TranslateThis
        nsCOMPtr<nsIXPCFunctionThisTranslator>
            translator(new nsXPCFunctionThisTranslator);
        xpc->SetFunctionThisTranslator(NS_GET_IID(nsITestXPCFunctionCallback), translator, nsnull);
#endif
    
        nsCOMPtr<nsIJSContextStack> cxstack = do_GetService("@mozilla.org/js/xpc/ContextStack;1");
        if (!cxstack) {
            printf("failed to get the nsThreadJSContextStack service!\n");
            return 1;
        }

        if(NS_FAILED(cxstack->Push(cx))) {
            printf("failed to push the current JSContext on the nsThreadJSContextStack!\n");
            return 1;
        }

        nsCOMPtr<nsIXPCScriptable> backstagePass;
        nsresult rv = rtsvc->GetBackstagePass(getter_AddRefs(backstagePass));
        if (NS_FAILED(rv)) {
            fprintf(gErrFile, "+++ Failed to get backstage pass from rtsvc: %8x\n",
                    rv);
            return 1;
        }

        /* -E selects a modern global before its built-ins are initialized.
         * -v retains its historical role of switching subsequent scripts. */
        for (int arg = 1; arg < argc; ++arg) {
            if (argv[arg][0] != '-' || argv[arg][1] == '\0')
                break;
            if (strcmp(argv[arg], "-E") == 0)
                JS_SetVersion(cx, JSVERSION_ECMA_2015);
            else if (argv[arg][1] == 'v' || argv[arg][1] == 'f' ||
                     argv[arg][1] == 'e')
                ++arg;
        }

        nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
        rv = xpc->InitClassesWithNewWrappedGlobal(cx, backstagePass,
                                                  NS_GET_IID(nsISupports),
                                                  nsIXPConnect::
                                                      FLAG_SYSTEM_GLOBAL_OBJECT,
                                                  getter_AddRefs(holder));
        if (NS_FAILED(rv))
            return 1;
        
        rv = holder->GetJSObject(&glob);
        if (NS_FAILED(rv)) {
            NS_ASSERTION(glob == nsnull, "bad GetJSObject?");
            return 1;
        }
        if (!JS_DefineFunctions(cx, glob, glob_functions))
            return 1;

        envobj = JS_DefineObject(cx, glob, "environment", &env_class, NULL, 0);
        if (!envobj || !JS_SetPrivate(cx, envobj, envp))
            return 1;

        argc--;
        argv++;

        result = ProcessArgs(cx, glob, argv, argc);


//#define TEST_CALL_ON_WRAPPED_JS_AFTER_SHUTDOWN 1

#ifdef TEST_CALL_ON_WRAPPED_JS_AFTER_SHUTDOWN
        // test of late call and release (see below)
        nsCOMPtr<nsIJSContextStack> bogus;
        xpc->WrapJS(cx, glob, NS_GET_IID(nsIJSContextStack),
                    (void**) getter_AddRefs(bogus));
#endif

        JS_ClearScope(cx, glob);
        JS_GC(cx);
        JSContext *oldcx;
        cxstack->Pop(&oldcx);
        NS_ASSERTION(oldcx == cx, "JS thread context push/pop mismatch");
        cxstack = nsnull;
        JS_GC(cx);
        JS_DestroyContext(cx);
        xpc->SyncJSContexts();
    } // this scopes the nsCOMPtrs
    // no nsCOMPtrs are allowed to be alive when you call NS_ShutdownXPCOM
    rv = NS_ShutdownXPCOM( NULL );
    NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");

#ifdef TEST_CALL_ON_WRAPPED_JS_AFTER_SHUTDOWN
    // test of late call and release (see above)
    JSContext* bogusCX;
    bogus->Peek(&bogusCX);
    bogus = nsnull;
#endif

    return result;
}
