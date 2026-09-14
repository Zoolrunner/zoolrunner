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
#include <stdlib.h>
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsobj.h"
#include "jsnum.h"
#include "jsregexp.h"
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
           !(((JSScopeProperty *) prop)->attrs & (JSPROP_GETTER | JSPROP_SETTER)) &&
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

/* Keep descriptor values rooted while conversion calls user getters. */
enum { D_ENUM, D_CONFIG, D_VALUE, D_WRITE, D_GET, D_SET, D_COUNT };
#define D_BIT(n) (1U << (n))
#define D_DATA (D_BIT(D_VALUE) | D_BIT(D_WRITE))
#define D_ACCESS (D_BIT(D_GET) | D_BIT(D_SET))
typedef struct ES5Descriptor {
    jsval v[D_COUNT];
    uintN present;
} ES5Descriptor;

static JSBool
DescriptorError(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_DESCRIPTOR);
    return JS_FALSE;
}

static JSBool
ToDescriptor(JSContext *cx, jsval value, ES5Descriptor *d)
{
    static const char *names[D_COUNT] = {
        "enumerable", "configurable", "value", "writable", "get", "set"
    };
    JSObject *source;
    JSBool has, b;
    uintN i;
    d->present = 0;
    for (i = 0; i < D_COUNT; ++i)
        d->v[i] = JSVAL_VOID;
    if (JSVAL_IS_PRIMITIVE(value))
        return DescriptorError(cx);
    source = JSVAL_TO_OBJECT(value);
    for (i = 0; i < D_COUNT; ++i) {
        if (!JS_HasProperty(cx, source, names[i], &has))
            return JS_FALSE;
        if (!has)
            continue;
        if (!JS_GetProperty(cx, source, names[i], &d->v[i]))
            return JS_FALSE;
        d->present |= D_BIT(i);
        if (i == D_ENUM || i == D_CONFIG || i == D_WRITE) {
            if (!JS_ValueToBoolean(cx, d->v[i], &b))
                return JS_FALSE;
            d->v[i] = BOOLEAN_TO_JSVAL(b);
        } else if (i == D_GET || i == D_SET) {
            if (!JSVAL_IS_VOID(d->v[i]) &&
                (JSVAL_IS_PRIMITIVE(d->v[i]) ||
                 (!VALUE_IS_FUNCTION(cx, d->v[i]) &&
                  !OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(d->v[i]))->call)))
                return DescriptorError(cx);
        }
    }
    if ((d->present & D_DATA) && (d->present & D_ACCESS))
        return DescriptorError(cx);
    return JS_TRUE;
}

static JSBool
SameValue(jsval a, jsval b)
{
    if (JSVAL_IS_NUMBER(a) && JSVAL_IS_NUMBER(b)) {
        jsdouble x = JSVAL_IS_INT(a) ? JSVAL_TO_INT(a) : *JSVAL_TO_DOUBLE(a);
        jsdouble y = JSVAL_IS_INT(b) ? JSVAL_TO_INT(b) : *JSVAL_TO_DOUBLE(b);
        return (JSDOUBLE_IS_NaN(x) && JSDOUBLE_IS_NaN(y)) ||
               (x == y && (x != 0 || JSDOUBLE_IS_NEGZERO(x) == JSDOUBLE_IS_NEGZERO(y)));
    }
    if (JSVAL_IS_STRING(a) && JSVAL_IS_STRING(b))
        return js_EqualStrings(JSVAL_TO_STRING(a), JSVAL_TO_STRING(b));
    return a == b;
}

static JSBool
IsExtensible(JSContext *cx, JSObject *target)
{
    JSBool extensible;
    if (!OBJ_IS_NATIVE(target)) return JS_TRUE;
    JS_LOCK_OBJ(cx, target);
    extensible = OBJ_SCOPE(target)->object != target ||
                  !(OBJ_SCOPE(target)->flags & (SCOPE_NONEXTENSIBLE | SCOPE_SEALED));
    JS_UNLOCK_OBJ(cx, target);
    return extensible;
}

/* DefineOwn uses this snapshot to delete sparse array indices in descending
 * numeric order, including non-enumerable indices. */
static JSIdArray *OwnNames(JSContext *cx, JSObject *target);

static int
DescendingIndex(const void *a, const void *b)
{
    jsuint x, y;
    js_IdIsIndex(ID_TO_VALUE(*(const jsid *)a), &x);
    js_IdIsIndex(ID_TO_VALUE(*(const jsid *)b), &y);
    return x > y ? -1 : x < y ? 1 : 0;
}

JSBool
js_ShrinkArray(JSContext *cx, JSObject *target, jsuint length, jsval *value,
            JSBool *blocked)
{
    RootedIds ids;
    jsint i, n = 0;
    jsuint index;
    jsval deleted;
    JSBool ok = JS_FALSE;
    *blocked = JS_FALSE;
    ids.ids = OwnNames(cx, target);
    if (!ids.ids) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkIds, &ids.root);
    for (i = 0; i < ids.ids->length; ++i) {
        if (js_IdIsIndex(ID_TO_VALUE(ids.ids->vector[i]), &index) && index >= length)
            ids.ids->vector[n++] = ids.ids->vector[i];
    }
    ids.ids->length = n;
    qsort(ids.ids->vector, n, sizeof(jsid), DescendingIndex);
    for (i = 0; i < n; ++i) {
        if (!OBJ_DELETE_PROPERTY(cx, target, ids.ids->vector[i], &deleted)) goto out;
        if (deleted == JSVAL_FALSE) {
            js_IdIsIndex(ID_TO_VALUE(ids.ids->vector[i]), &index);
            if (!JS_NewNumberValue(cx, (jsdouble)index + 1, value)) goto out;
            *blocked = JS_TRUE;
            break;
        }
    }
    ok = JS_TRUE;
out:
    JS_POP_TEMP_ROOT(cx, &ids.root);
    JS_DestroyIdArray(cx, ids.ids);
    return ok;
}

/* Descriptor conversion is finished before this function mutates the target. */
static JSBool
DefineOwn(JSContext *cx, JSObject *target, jsid id, ES5Descriptor *d)
{
    jsval callargs[2], oldValue = JSVAL_VOID, checked = JSVAL_VOID;
    ES5Descriptor old;
    JSTempValueRooter oldRoot, valueRoot, callRoot;
    JSObject *owner;
    JSProperty *prop;
    JSScopeProperty *sprop;
    JSPropertyOp getter = JS_PropertyStub, setter = JS_PropertyStub;
    uintN attrs = 0, flags = JSDNP_REPLACE, checkedAttrs, i;
    intN shortid = 0;
    JSBool exists, access, oldAccess, ok = JS_FALSE, arrayLength = JS_FALSE;
    JSBool blocked = JS_FALSE;
    jsuint oldLength = 0, newLength = 0, index;
    jsdouble number;
    jsid lengthId = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);

    if (js_IdIsIndex(ID_TO_VALUE(id), &index) && index <= JSVAL_INT_MAX)
        id = INT_TO_JSID(index);
    JS_PUSH_TEMP_ROOT(cx, D_COUNT, old.v, &oldRoot);
    for (i = 0; i < D_COUNT; ++i)
        old.v[i] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 1, &oldValue, &valueRoot);
    callargs[0] = OBJECT_TO_JSVAL(target);
    callargs[1] = ID_TO_VALUE(id);
    JS_PUSH_TEMP_ROOT(cx, 2, callargs, &callRoot);
    ok = obj_getOwnPropertyDescriptor(cx, NULL, 2, callargs, &oldValue);
    JS_POP_TEMP_ROOT(cx, &callRoot);
    if (!ok) goto out;
    ok = JS_FALSE;
    exists = !JSVAL_IS_VOID(oldValue);
    if (exists) {
        if (!ToDescriptor(cx, oldValue, &old))
            goto out;
    } else {
        old.present = D_DATA | D_BIT(D_ENUM) | D_BIT(D_CONFIG);
        old.v[D_ENUM] = old.v[D_CONFIG] = old.v[D_WRITE] = JSVAL_FALSE;
        if (!IsExtensible(cx, target))
            goto reject;
    }
    if (OBJ_GET_CLASS(cx, target) == &js_ArrayClass) {
        arrayLength = (id == lengthId);
        if (arrayLength && (d->present & D_BIT(D_VALUE))) {
            if (!JS_ValueToECMAUint32(cx, d->v[D_VALUE], &newLength) ||
                !JS_ValueToNumber(cx, d->v[D_VALUE], &number)) goto out;
            if (number != (jsdouble)newLength) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_ARRAY_LENGTH);
                goto out;
            }
            if (!JS_NewNumberValue(cx, newLength, &d->v[D_VALUE])) goto out;
            if (!js_GetLengthProperty(cx, target, &oldLength)) goto out;
        } else if (js_IdIsIndex(ID_TO_VALUE(id), &index)) {
            if (!js_GetLengthProperty(cx, target, &oldLength) ||
                !OBJ_LOOKUP_PROPERTY(cx, target, lengthId, &owner, &prop)) goto out;
            if (prop) {
                ok = OBJ_GET_ATTRIBUTES(cx, owner, lengthId, prop, &checkedAttrs);
                OBJ_DROP_PROPERTY(cx, owner, prop);
                if (!ok) goto out;
                ok = JS_FALSE;
                if (index >= oldLength && (checkedAttrs & JSPROP_READONLY)) goto reject;
            }
        }
    }
    if (!d->present) {
        if (exists) { ok = JS_TRUE; goto out; }
    }
    oldAccess = (old.present & D_ACCESS) != 0;
    access = (d->present & D_ACCESS) ? JS_TRUE :
             (d->present & D_DATA) ? JS_FALSE : oldAccess;
    if (exists && old.v[D_CONFIG] == JSVAL_FALSE) {
        if (((d->present & D_BIT(D_CONFIG)) && d->v[D_CONFIG] == JSVAL_TRUE) ||
            ((d->present & D_BIT(D_ENUM)) && d->v[D_ENUM] != old.v[D_ENUM]) ||
            access != oldAccess)
            goto reject;
        if (oldAccess) {
            if (((d->present & D_BIT(D_GET)) && !SameValue(d->v[D_GET], old.v[D_GET])) ||
                ((d->present & D_BIT(D_SET)) && !SameValue(d->v[D_SET], old.v[D_SET])))
                goto reject;
        } else if (old.v[D_WRITE] == JSVAL_FALSE) {
            if (((d->present & D_BIT(D_WRITE)) && d->v[D_WRITE] == JSVAL_TRUE) ||
                ((d->present & D_BIT(D_VALUE)) && !SameValue(d->v[D_VALUE], old.v[D_VALUE])))
                goto reject;
        }
    }
    if (access != oldAccess) {
        old.v[D_GET] = old.v[D_SET] = old.v[D_VALUE] = JSVAL_VOID;
        old.v[D_WRITE] = JSVAL_FALSE;
    }
    for (i = 0; i < D_COUNT; ++i) {
        if (d->present & D_BIT(i))
            old.v[i] = d->v[i];
    }
    if (old.v[D_ENUM] == JSVAL_TRUE) attrs |= JSPROP_ENUMERATE;
    if (old.v[D_CONFIG] != JSVAL_TRUE) attrs |= JSPROP_PERMANENT;
    if (access) {
        /* GETTER also identifies the accessor with both functions absent. */
        attrs |= JSPROP_SHARED | JSPROP_GETTER;
        if (!JSVAL_IS_VOID(old.v[D_GET]))
            getter = (JSPropertyOp) JSVAL_TO_OBJECT(old.v[D_GET]);
        else
            getter = NULL;
        if (!JSVAL_IS_VOID(old.v[D_SET])) {
            attrs |= JSPROP_SETTER;
            setter = (JSPropertyOp) JSVAL_TO_OBJECT(old.v[D_SET]);
        } else {
            setter = NULL;
        }
    } else {
        if (old.v[D_WRITE] != JSVAL_TRUE) attrs |= JSPROP_READONLY;
        /* Preserve native instance hooks, e.g. Array.length and arguments. */
        if (exists && !oldAccess) {
            if (!OBJ_LOOKUP_PROPERTY(cx, target, id, &owner, &prop))
                goto out;
            if (prop) {
                if (OBJ_IS_NATIVE(owner)) {
                    sprop = (JSScopeProperty *) prop;
                    getter = sprop->getter ? sprop->getter : JS_PropertyStub;
                    setter = sprop->setter ? sprop->setter : JS_PropertyStub;
                    flags |= sprop->flags;
                    shortid = sprop->shortid;
                }
                OBJ_DROP_PROPERTY(cx, owner, prop);
            }
        }
    }
    if (!OBJ_CHECK_ACCESS(cx, target, id, JSACC_WRITE, &checked, &checkedAttrs))
        goto out;
    if (arrayLength && (d->present & D_BIT(D_VALUE)) && newLength < oldLength) {
        if (!js_ShrinkArray(cx, target, newLength, &old.v[D_VALUE], &blocked)) goto out;
    }
    if (OBJ_IS_NATIVE(target)) {
        ok = js_DefineNativeProperty(cx, target, id, old.v[D_VALUE],
                                      getter, setter, attrs, flags, shortid, NULL);
    } else {
        ok = OBJ_DEFINE_PROPERTY(cx, target, id, old.v[D_VALUE],
                                  getter, setter, attrs, NULL);
    }
    if (ok && OBJ_GET_CLASS(cx, target) == &js_RegExpClass &&
        !access && (d->present & D_BIT(D_VALUE)) && setter != JS_PropertyStub)
        ok = setter(cx, target, (flags & SPROP_HAS_SHORTID) ? INT_TO_JSVAL(shortid) : ID_TO_VALUE(id),
                    &old.v[D_VALUE]);
    if (ok && OBJ_GET_CLASS(cx, target) == &js_ArgumentsClass)
        ok = js_UpdateArgumentsProperty(cx, target, id, &old.v[D_VALUE],
                                         (d->present & D_BIT(D_VALUE)) != 0,
                                         access || old.v[D_WRITE] == JSVAL_FALSE);
    if (ok && blocked) ok = DescriptorError(cx);
    goto out;
reject:
    DescriptorError(cx);
out:
    JS_POP_TEMP_ROOT(cx, &valueRoot);
    JS_POP_TEMP_ROOT(cx, &oldRoot);
    return ok;
}

static JSBool
obj_defineProperty(JSContext *cx, JSObject *obj, uintN argc,
                   jsval *argv, jsval *rval)
{
    JSObject *target = RequireObject(cx, argc, argv);
    JSString *str;
    jsid id;
    ES5Descriptor d;
    JSTempValueRooter root;
    JSBool ok;
    uintN i;
    if (!target) return JS_FALSE;
    str = js_ValueToString(cx, argv[1]);
    if (!str) return JS_FALSE;
    argv[1] = STRING_TO_JSVAL(str);
    if (!JS_ValueToId(cx, argv[1], &id)) return JS_FALSE;
    for (i = 0; i < D_COUNT; ++i) d.v[i] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, D_COUNT, d.v, &root);
    ok = ToDescriptor(cx, argv[2], &d) && DefineOwn(cx, target, id, &d);
    JS_POP_TEMP_ROOT(cx, &root);
    if (ok) *rval = OBJECT_TO_JSVAL(target);
    return ok;
}

static JSBool
DefineProperties(JSContext *cx, JSObject *target, jsval *properties)
{
    JSObject *source, *owner;
    JSProperty *prop;
    uintN attrs;
    jsid id;
    RootedIds ids;
    ES5Descriptor *descs = NULL;
    JSTempValueRooter *roots = NULL, valueRoot;
    jsval value = JSVAL_VOID;
    jsint i, rooted = 0;
    uintN j;
    JSBool ok = JS_FALSE;
    if (!js_ValueToObject(cx, *properties, &source) || !source) {
        if (!JS_IsExceptionPending(cx)) DescriptorError(cx);
        return JS_FALSE;
    }
    *properties = OBJECT_TO_JSVAL(source);
    ids.ids = OwnNames(cx, source);
    if (!ids.ids) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkIds, &ids.root);
    JS_PUSH_TEMP_ROOT(cx, 1, &value, &valueRoot);
    if (ids.ids->length) {
        if ((size_t)ids.ids->length > ((size_t)-1) / sizeof(*descs) ||
            (size_t)ids.ids->length > ((size_t)-1) / sizeof(*roots)) {
            JS_ReportOutOfMemory(cx);
            goto out;
        }
        descs = (ES5Descriptor *) JS_malloc(cx, ids.ids->length * sizeof(*descs));
        roots = (JSTempValueRooter *) JS_malloc(cx, ids.ids->length * sizeof(*roots));
        if (!descs || !roots) goto out;
    }
    for (i = 0; i < ids.ids->length; ++i) {
        id = ids.ids->vector[i];
        if (!OBJ_LOOKUP_PROPERTY(cx, source, id, &owner, &prop)) goto out;
        if (!prop) continue;
        ok = OBJ_GET_ATTRIBUTES(cx, owner, id, prop, &attrs);
        if (owner != source && !IsVirtualOwn(cx, source, owner, prop)) attrs = 0;
        OBJ_DROP_PROPERTY(cx, owner, prop);
        if (!ok) goto out;
        ok = JS_FALSE;
        if (!(attrs & JSPROP_ENUMERATE)) continue;
        for (j = 0; j < D_COUNT; ++j) descs[rooted].v[j] = JSVAL_VOID;
        JS_PUSH_TEMP_ROOT(cx, D_COUNT, descs[rooted].v, &roots[rooted]);
        ++rooted;
        if (!OBJ_GET_PROPERTY(cx, source, id, &value) ||
            !ToDescriptor(cx, value, &descs[rooted - 1]))
            goto out;
        ids.ids->vector[rooted - 1] = id;
    }
    for (i = 0; i < rooted; ++i) {
        if (!DefineOwn(cx, target, ids.ids->vector[i], &descs[i])) goto out;
    }
    ok = JS_TRUE;
out:
    while (rooted) { --rooted; JS_POP_TEMP_ROOT(cx, &roots[rooted]); }
    JS_free(cx, roots);
    JS_free(cx, descs);
    JS_POP_TEMP_ROOT(cx, &valueRoot);
    JS_POP_TEMP_ROOT(cx, &ids.root);
    JS_DestroyIdArray(cx, ids.ids);
    return ok;
}

static JSBool
obj_defineProperties(JSContext *cx, JSObject *obj, uintN argc,
                     jsval *argv, jsval *rval)
{
    JSObject *target = RequireObject(cx, argc, argv);
    if (!target || !DefineProperties(cx, target, &argv[1])) return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(target);
    return JS_TRUE;
}

static JSBool
obj_create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *target;
    if (!argc || (!JSVAL_IS_NULL(argv[0]) && JSVAL_IS_PRIMITIVE(argv[0])))
        return DescriptorError(cx);
    target = js_NewObject(cx, &js_ObjectClass, JSVAL_TO_OBJECT(argv[0]), NULL);
    if (!target) return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(target);
    if (JSVAL_IS_NULL(argv[0]) && !JS_SetPrototype(cx, target, NULL)) return JS_FALSE;
    return JSVAL_IS_VOID(argv[1]) || DefineProperties(cx, target, &argv[1]);
}

static JSBool
obj_isExtensible(JSContext *cx, JSObject *obj, uintN argc,
                 jsval *argv, jsval *rval)
{
    JSObject *target = RequireObject(cx, argc, argv);
    if (!target) return JS_FALSE;
    *rval = BOOLEAN_TO_JSVAL(IsExtensible(cx, target));
    return JS_TRUE;
}

static JSBool
obj_preventExtensions(JSContext *cx, JSObject *obj, uintN argc,
                      jsval *argv, jsval *rval)
{
    JSObject *target = RequireObject(cx, argc, argv);
    JSIdArray *ids;
    JSScope *scope;
    if (!target) return JS_FALSE;
    /* Materialize lazy own properties before closing the object. */
    ids = JS_Enumerate(cx, target);
    if (!ids) return JS_FALSE;
    JS_DestroyIdArray(cx, ids);
    if (!OBJ_IS_NATIVE(target)) return DescriptorError(cx);
    JS_LOCK_OBJ(cx, target);
    scope = js_GetMutableScope(cx, target);
    if (scope && !SCOPE_IS_SEALED(scope)) scope->flags |= SCOPE_NONEXTENSIBLE;
    JS_UNLOCK_OBJ(cx, target);
    if (!scope) return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(target);
    return JS_TRUE;
}

/* Snapshot names before allocating result strings or invoking user code. */
static JSIdArray *
OwnNames(JSContext *cx, JSObject *target)
{
    JSIdArray *ids, *grown;
    JSObject *owner;
    JSClass *clasp;
    JSScope *scope;
    JSScopeProperty *sprop;
    jsint i, n, capacity = 0, nextCapacity;
    jsid swap;
    ids = JS_Enumerate(cx, target);
    if (!ids || !OBJ_IS_NATIVE(target)) return ids;
    JS_DestroyIdArray(cx, ids);
    ids = js_NewIdArray(cx, 0);
    if (!ids) return NULL;
    clasp = OBJ_GET_CLASS(cx, target);
    for (owner = target; owner && OBJ_IS_NATIVE(owner);
         owner = OBJ_GET_PROTO(cx, owner)) {
        if (owner != target && (clasp == &js_ObjectClass ||
                                OBJ_GET_CLASS(cx, owner) != clasp)) break;
        JS_LOCK_OBJ(cx, owner);
        scope = OBJ_SCOPE(owner);
        if (scope->object == owner) {
            for (sprop = SCOPE_LAST_PROP(scope); sprop; sprop = sprop->parent) {
                if (!SCOPE_HAS_PROPERTY(scope, sprop) ||
                    (sprop->flags & SPROP_IS_ALIAS) ||
                    (owner != target && !IsVirtualOwn(cx, target, owner, (JSProperty *)sprop)))
                    continue;
                n = ids->length;
                if (owner != target) {
                    for (i = 0; i < n && ids->vector[i] != sprop->id; ++i) {}
                    if (i != n) continue;
                }
                if (n == JSVAL_INT_MAX ||
                    (size_t)n >= (((size_t)-1) - sizeof(JSIdArray)) / sizeof(jsid)) {
                    JS_ReportOutOfMemory(cx);
                    JS_UNLOCK_OBJ(cx, owner);
                    JS_DestroyIdArray(cx, ids);
                    return NULL;
                }
                if (n == capacity) {
                    nextCapacity = capacity ?
                        (capacity > JSVAL_INT_MAX / 2 ? JSVAL_INT_MAX : capacity * 2) : 8;
                    if ((size_t)nextCapacity > (((size_t)-1) - sizeof(JSIdArray)) / sizeof(jsid))
                        nextCapacity = n + 1;
                    grown = js_SetIdArrayLength(cx, ids, nextCapacity);
                    if (!grown) {
                        JS_UNLOCK_OBJ(cx, owner);
                        return NULL; /* js_SetIdArrayLength frees on failure. */
                    }
                    ids = grown;
                    capacity = nextCapacity;
                }
                ids->vector[n] = sprop->id;
                ids->length = n + 1;
            }
        }
        JS_UNLOCK_OBJ(cx, owner);
    }
    for (i = 0, n = ids->length - 1; i < n; ++i, --n) {
        swap = ids->vector[i];
        ids->vector[i] = ids->vector[n];
        ids->vector[n] = swap;
    }
    return ids;
}

static JSBool
obj_getOwnPropertyNames(JSContext *cx, JSObject *obj, uintN argc,
                       jsval *argv, jsval *rval)
{
    JSObject *target = RequireObject(cx, argc, argv), *array;
    RootedIds ids;
    JSString *str;
    JSTempValueRooter strRoot;
    jsint i;
    JSBool ok = JS_FALSE;
    if (!target) return JS_FALSE;
    ids.ids = OwnNames(cx, target);
    if (!ids.ids) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkIds, &ids.root);
    array = js_NewArrayObject(cx, 0, NULL);
    if (!array) goto out;
    *rval = OBJECT_TO_JSVAL(array);
    for (i = 0; i < ids.ids->length; ++i) {
        str = js_ValueToString(cx, ID_TO_VALUE(ids.ids->vector[i]));
        if (!str) goto out;
        JS_PUSH_TEMP_ROOT_STRING(cx, str, &strRoot);
        ok = JS_DefineElement(cx, array, i, STRING_TO_JSVAL(str),
                              NULL, NULL, JSPROP_ENUMERATE);
        JS_POP_TEMP_ROOT(cx, &strRoot);
        if (!ok) goto out;
    }
    ok = JS_TRUE;
out:
    JS_POP_TEMP_ROOT(cx, &ids.root);
    JS_DestroyIdArray(cx, ids.ids);
    return ok;
}

static JSBool
ObjectIntegrity(JSContext *cx, uintN argc, jsval *argv, jsval *rval,
                JSBool freeze, JSBool query)
{
    JSObject *target = RequireObject(cx, argc, argv);
    RootedIds ids;
    JSObject *owner;
    JSProperty *prop;
    uintN attrs;
    jsint i;
    ES5Descriptor d;
    JSBool ok = JS_FALSE;
    if (!target) return JS_FALSE;
    if (query && IsExtensible(cx, target)) {
        *rval = JSVAL_FALSE;
        return JS_TRUE;
    }
    ids.ids = OwnNames(cx, target);
    if (!ids.ids) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkIds, &ids.root);
    for (i = 0; i < D_COUNT; ++i) d.v[i] = JSVAL_VOID;
    d.v[D_CONFIG] = d.v[D_WRITE] = JSVAL_FALSE;
    for (i = 0; i < ids.ids->length; ++i) {
        if (!OBJ_LOOKUP_PROPERTY(cx, target, ids.ids->vector[i], &owner, &prop)) goto out;
        if (!prop) continue;
        ok = OBJ_GET_ATTRIBUTES(cx, owner, ids.ids->vector[i], prop, &attrs);
        OBJ_DROP_PROPERTY(cx, owner, prop);
        if (!ok) goto out;
        ok = JS_FALSE;
        if (query) {
            if (!(attrs & JSPROP_PERMANENT) ||
                (freeze && !(attrs & (JSPROP_GETTER | JSPROP_SETTER | JSPROP_READONLY)))) {
                *rval = JSVAL_FALSE;
                ok = JS_TRUE;
                goto out;
            }
        } else {
            d.present = D_BIT(D_CONFIG);
            if (freeze && !(attrs & (JSPROP_GETTER | JSPROP_SETTER)))
                d.present |= D_BIT(D_WRITE);
            if (!DefineOwn(cx, target, ids.ids->vector[i], &d)) goto out;
        }
    }
    if (query) {
        *rval = JSVAL_TRUE;
        ok = JS_TRUE;
    } else {
        ok = obj_preventExtensions(cx, NULL, argc, argv, rval);
    }
out:
    JS_POP_TEMP_ROOT(cx, &ids.root);
    JS_DestroyIdArray(cx, ids.ids);
    return ok;
}

#define INTEGRITY_METHOD(name, freeze, query) \
static JSBool name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ return ObjectIntegrity(cx, argc, argv, rval, freeze, query); }
INTEGRITY_METHOD(obj_seal, JS_FALSE, JS_FALSE)
INTEGRITY_METHOD(obj_freeze, JS_TRUE, JS_FALSE)
INTEGRITY_METHOD(obj_isSealed, JS_FALSE, JS_TRUE)
INTEGRITY_METHOD(obj_isFrozen, JS_TRUE, JS_TRUE)
#undef INTEGRITY_METHOD

JSBool
js_InitObjectES5(JSContext *cx, JSObject *proto)
{
    JSObject *ctor = JS_GetConstructor(cx, proto);
    if (!ctor)
        return JS_FALSE;
    return JS_DefineFunction(cx, ctor, "getOwnPropertyNames", obj_getOwnPropertyNames, 1, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "seal", obj_seal, 1, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "freeze", obj_freeze, 1, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "isSealed", obj_isSealed, 1, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "isFrozen", obj_isFrozen, 1, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "defineProperty", obj_defineProperty, 3, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "defineProperties", obj_defineProperties, 2, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "create", obj_create, 2, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "isExtensible", obj_isExtensible, 1, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "preventExtensions", obj_preventExtensions, 1, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "getPrototypeOf", obj_getPrototypeOf,
                              1, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "keys", obj_keys, 1, JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, ctor, "getOwnPropertyDescriptor",
                              obj_getOwnPropertyDescriptor, 2, JSFUN_NO_CONSTRUCT);
}
