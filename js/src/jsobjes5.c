/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sw=4 et tw=78:
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

/* ES5 Object methods implemented over the classic object and scope APIs. */
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsobj.h"
#include "jsscope.h"
#include "jsstr.h"

static JSObject *
RequireObject(JSContext *cx, uintN argc, jsval *argv)
{
    if (!argc || JSVAL_IS_PRIMITIVE(argv[0])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OBJECT_REQUIRED);
        return NULL;
    }
    return JSVAL_TO_OBJECT(argv[0]);
}

static JSBool
obj_getPrototypeOf(JSContext *cx, JSObject *obj, uintN argc,
                   jsval *argv, jsval *rval)
{
    JSObject *target, *proto;
    JSClass *clasp;
    JSExtendedClass *xclasp;
    uintN attrs;

    target = RequireObject(cx, argc, argv);
    if (!target)
        return JS_FALSE;
    /* Use the same access checks and outer-object boundary as __proto__,
     * without looking up a user property of that name. */
    if (!OBJ_CHECK_ACCESS(cx, target,
                          ATOM_TO_JSID(cx->runtime->atomState.protoAtom),
                          JSACC_PROTO, rval, &attrs))
        return JS_FALSE;
    proto = JSVAL_TO_OBJECT(*rval);
    /* Cloned closures delegate to a shared function object internally. That
     * implementation object is not the function's ECMAScript prototype. */
    if (OBJ_GET_CLASS(cx, target) == &js_FunctionClass) {
        JSFunction *fun = (JSFunction *) JS_GetPrivate(cx, target);
        while (proto && OBJ_GET_CLASS(cx, proto) == &js_FunctionClass &&
               JS_GetPrivate(cx, proto) == fun) {
            if (!OBJ_CHECK_ACCESS(cx, proto,
                                  ATOM_TO_JSID(cx->runtime->atomState.protoAtom),
                                  JSACC_PROTO, rval, &attrs))
                return JS_FALSE;
            proto = JSVAL_TO_OBJECT(*rval);
        }
    }
    if (proto) {
        clasp = OBJ_GET_CLASS(cx, proto);
        if (clasp == &js_CallClass || clasp == &js_BlockClass) {
            *rval = JSVAL_NULL;
        } else if (clasp->flags & JSCLASS_IS_EXTENDED) {
            xclasp = (JSExtendedClass *) clasp;
            if (xclasp->outerObject) {
                proto = xclasp->outerObject(cx, proto);
                if (!proto)
                    return JS_FALSE;
                *rval = OBJECT_TO_JSVAL(proto);
            }
        }
    }
    return JS_TRUE;
}

typedef struct RootedIds {
    JSTempValueRooter root;
    JSIdArray *ids;
} RootedIds;

JS_STATIC_DLL_CALLBACK(void)
MarkIds(JSContext *cx, JSTempValueRooter *root)
{
    RootedIds *ids = (RootedIds *) root;
    jsint i;
    for (i = 0; i < ids->ids->length; ++i) {
        jsid id = ids->ids->vector[i];
        if (JSID_IS_ATOM(id))
            js_MarkAtom(cx, JSID_TO_ATOM(id));
        else if (JSID_IS_OBJECT(id))
            GC_MARK(cx, JSID_TO_OBJECT(id), "Object property names");
    }
}

static JSBool
obj_keys(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *target, *result;
    RootedIds ids;
    jsint i;
    JSString *str;
    JSTempValueRooter stringRoot;
    JSBool ok = JS_FALSE;

    target = RequireObject(cx, argc, argv);
    if (!target)
        return JS_FALSE;
    ids.ids = JS_Enumerate(cx, target);
    if (!ids.ids)
        return JS_FALSE;
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkIds, &ids.root);
    result = js_NewArrayObject(cx, ids.ids->length, NULL);
    if (!result)
        goto out;
    *rval = OBJECT_TO_JSVAL(result);
    for (i = 0; i < ids.ids->length; ++i) {
        str = js_ValueToString(cx, ID_TO_VALUE(ids.ids->vector[i]));
        if (!str)
            goto out;
        JS_PUSH_TEMP_ROOT_STRING(cx, str, &stringRoot);
        /* Define own elements: inherited setters must not intercept them. */
        ok = JS_DefineElement(cx, result, i, STRING_TO_JSVAL(str),
                              NULL, NULL, JSPROP_ENUMERATE);
        JS_POP_TEMP_ROOT(cx, &stringRoot);
        if (!ok)
            goto out;
    }
    ok = JS_TRUE;
out:
    JS_POP_TEMP_ROOT(cx, &ids.root);
    JS_DestroyIdArray(cx, ids.ids);
    return ok;
}

/* Native built-ins share permanent instance fields with their class
 * prototype (String.length, function.length and RegExp fields). Preserve that
 * representation while reporting those fields as own data properties. */
static JSBool
IsVirtualOwn(JSContext *cx, JSObject *target, JSObject *owner,
             JSProperty *prop)
{
    JSClass *clasp = OBJ_GET_CLASS(cx, target);
    return clasp != &js_ObjectClass && OBJ_IS_NATIVE(owner) &&
           OBJ_GET_CLASS(cx, owner) == clasp &&
           SPROP_IS_SHARED_PERMANENT((JSScopeProperty *) prop);
}

static JSBool
DescriptorField(JSContext *cx, JSObject *desc, const char *name, jsval value)
{
    return JS_DefineProperty(cx, desc, name, value, NULL, NULL, JSPROP_ENUMERATE);
}

static JSBool
obj_getOwnPropertyDescriptor(JSContext *cx, JSObject *obj, uintN argc,
                             jsval *argv, jsval *rval)
{
    JSObject *target, *owner, *desc;
    JSProperty *prop;
    JSScopeProperty *sprop;
    JSString *key;
    jsid id;
    uintN attrs, checkedAttrs;
    jsval values[3] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter roots;
    JSBool accessor, ok = JS_FALSE;

    target = RequireObject(cx, argc, argv);
    if (!target)
        return JS_FALSE;
    /* Force ES5 ToString, bypassing JS_ValueToId's E4X object-key extension. */
    key = js_ValueToString(cx, argv[1]);
    if (!key)
        return JS_FALSE;
    argv[1] = STRING_TO_JSVAL(key);
    if (!JS_ValueToId(cx, argv[1], &id))
        return JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 3, values, &roots);
    if (!OBJ_CHECK_ACCESS(cx, target, id, JSACC_READ, &values[0], &checkedAttrs))
        goto out;
    if (!OBJ_LOOKUP_PROPERTY(cx, target, id, &owner, &prop))
        goto out;
    if (!prop || (owner != target && !IsVirtualOwn(cx, target, owner, prop))) {
        if (prop)
            OBJ_DROP_PROPERTY(cx, owner, prop);
        *rval = JSVAL_VOID;
        ok = JS_TRUE;
        goto out;
    }
    if (!OBJ_GET_ATTRIBUTES(cx, owner, id, prop, &attrs)) {
        OBJ_DROP_PROPERTY(cx, owner, prop);
        goto out;
    }
    accessor = JS_FALSE;
    if (OBJ_IS_NATIVE(owner)) {
        sprop = (JSScopeProperty *) prop;
        accessor = (attrs & (JSPROP_GETTER | JSPROP_SETTER)) != 0;
        if (accessor) {
            if ((attrs & JSPROP_GETTER) && sprop->getter)
                values[1] = OBJECT_TO_JSVAL(sprop->getter);
            if ((attrs & JSPROP_SETTER) && sprop->setter)
                values[2] = OBJECT_TO_JSVAL(sprop->setter);
        }
    }
    OBJ_DROP_PROPERTY(cx, owner, prop);
    if (!accessor && !OBJ_GET_PROPERTY(cx, target, id, &values[0]))
        goto out;
    desc = js_NewObject(cx, &js_ObjectClass, NULL, NULL);
    if (!desc)
        goto out;
    *rval = OBJECT_TO_JSVAL(desc);
    if (accessor) {
        if (!DescriptorField(cx, desc, "get", values[1]) ||
            !DescriptorField(cx, desc, "set", values[2]))
            goto out;
    } else {
        if (!DescriptorField(cx, desc, "value", values[0]) ||
            !DescriptorField(cx, desc, "writable",
                              BOOLEAN_TO_JSVAL(!(attrs & JSPROP_READONLY))))
            goto out;
    }
    ok = DescriptorField(cx, desc, "enumerable",
                          BOOLEAN_TO_JSVAL((attrs & JSPROP_ENUMERATE) != 0)) &&
         DescriptorField(cx, desc, "configurable",
                          BOOLEAN_TO_JSVAL(!(attrs & JSPROP_PERMANENT)));
out:
    JS_POP_TEMP_ROOT(cx, &roots);
    return ok;
}

JSBool
js_InitObjectES5(JSContext *cx, JSObject *proto)
{
    JSObject *ctor = JS_GetConstructor(cx, proto);
    if (!ctor)
        return JS_FALSE;
    return JS_DefineFunction(cx, ctor, "getPrototypeOf", obj_getPrototypeOf,
                              1, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "keys", obj_keys, 1, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "getOwnPropertyDescriptor",
                              obj_getOwnPropertyDescriptor, 2, JSFUN_NO_CONSTRUCT);
}
