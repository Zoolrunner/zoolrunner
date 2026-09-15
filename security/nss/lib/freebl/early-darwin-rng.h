/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Mac OS X 10.0/10.1 have AppleCSP, but no /dev/urandom. This is the
 * operating system's random generator, not a replacement entropy pool.
 * Keep all CSSM registrations scoped to the call so unloading NSS leaves
 * no callbacks into this library registered with the system framework. */
#include <Security/cssm.h>
#include <Security/cssmapple.h>
/* drbg.c serializes production initialization and reseeding. No additional
 * process-lifetime lock or provider registration is needed here. */

static void *EarlyRNGAlloc(uint32 size, void *ref) { return malloc(size); }
static void EarlyRNGFree(void *ptr, void *ref) { free(ptr); }
static void *EarlyRNGRealloc(void *ptr, uint32 size, void *ref)
{
    return realloc(ptr, size);
}
static void *EarlyRNGCalloc(uint32 count, uint32 size, void *ref)
{
    if (size && count > ((size_t)-1) / size)
        return NULL;
    return calloc(count, size);
}

static size_t
EarlyDarwinSystemRNG(void *dest, size_t length)
{
    CSSM_VERSION version = { 2, 0 };
    CSSM_GUID caller = { 0 };
    CSSM_PVC_MODE policy = CSSM_PVC_NONE;
    CSSM_MEMORY_FUNCS memory = {
        EarlyRNGAlloc, EarlyRNGFree, EarlyRNGRealloc, EarlyRNGCalloc, NULL
    };
    CSSM_MODULE_HANDLE module = 0;
    CSSM_CC_HANDLE context = 0;
    CSSM_DATA data;
    PRBool initialized = PR_FALSE, loaded = PR_FALSE;
    PRBool attached = PR_FALSE, created = PR_FALSE;
    size_t result = 0;

    if (!length)
        return 0;
    if (length > (uint32)-1)
        goto failure;
    if (CSSM_Init(&version, CSSM_PRIVILEGE_SCOPE_NONE, &caller,
                  CSSM_KEY_HIERARCHY_NONE, &policy, NULL) != CSSM_OK)
        goto done;
    initialized = PR_TRUE;
    if (CSSM_ModuleLoad(&gGuidAppleCSP, CSSM_KEY_HIERARCHY_NONE,
                       NULL, NULL) != CSSM_OK)
        goto done;
    loaded = PR_TRUE;
    if (CSSM_ModuleAttach(&gGuidAppleCSP, &version, &memory, 0,
                         CSSM_SERVICE_CSP, 0, CSSM_KEY_HIERARCHY_NONE,
                         NULL, 0, NULL, &module) != CSSM_OK)
        goto done;
    attached = PR_TRUE;
    if (CSSM_CSP_CreateRandomGenContext(module, CSSM_ALGID_APPLE_YARROW,
                                       NULL, (uint32)length, &context) != CSSM_OK)
        goto done;
    created = PR_TRUE;
    data.Length = (uint32)length;
    data.Data = dest;
    if (CSSM_GenerateRandom(context, &data) == CSSM_OK &&
        data.Data == dest && data.Length == length)
        result = length;
    if (data.Data && data.Data != dest)
        EarlyRNGFree(data.Data, NULL);
done:
    if (created && CSSM_DeleteContext(context) != CSSM_OK)
        result = 0;
    if (attached && CSSM_ModuleDetach(module) != CSSM_OK)
        result = 0;
    if (loaded && CSSM_ModuleUnload(&gGuidAppleCSP, NULL, NULL) != CSSM_OK)
        result = 0;
    if (initialized && CSSM_Terminate() != CSSM_OK)
        result = 0;
    if (result)
        return result;
failure:
    memset(dest, 0, length);
    PORT_SetError(SEC_ERROR_NEED_RANDOM);
    return 0;
}
