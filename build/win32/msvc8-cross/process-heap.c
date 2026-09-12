/*
 * Give /MT images a common allocation domain without depending on a VC8 CRT
 * DLL.  Classic Mozilla passes C and C++ allocations across DLL boundaries;
 * the separate private heaps created by each copy of LIBCMT cannot safely do
 * that.  The Win32 process heap is available on every supported Windows
 * version and remains private to the ZoolRunner process.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>

static void *
zr_heap_failure(void)
{
    errno = ENOMEM;
    return NULL;
}

void * __cdecl
malloc(size_t size)
{
    void *result;
    if (size == 0)
        size = 1;
    result = HeapAlloc(GetProcessHeap(), 0, size);
    return result ? result : zr_heap_failure();
}

void __cdecl
free(void *memory)
{
    if (memory)
        HeapFree(GetProcessHeap(), 0, memory);
}

void * __cdecl
_calloc_impl(size_t count, size_t size, int *saved_errno)
{
    size_t total;
    void *result;
    if (size != 0 && count > ((size_t)-1) / size) {
        if (saved_errno)
            *saved_errno = ENOMEM;
        return zr_heap_failure();
    }
    total = count * size;
    if (total == 0)
        total = 1;
    result = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, total);
    if (!result && saved_errno)
        *saved_errno = ENOMEM;
    return result ? result : zr_heap_failure();
}

void * __cdecl
calloc(size_t count, size_t size)
{
    return _calloc_impl(count, size, NULL);
}

void * __cdecl
realloc(void *memory, size_t size)
{
    void *result;
    if (!memory)
        return malloc(size);
    if (size == 0) {
        free(memory);
        return NULL;
    }
    result = HeapReAlloc(GetProcessHeap(), 0, memory, size);
    return result ? result : zr_heap_failure();
}

void * __cdecl
_recalloc(void *memory, size_t count, size_t size)
{
    size_t old_size = memory ? _msize(memory) : 0;
    size_t total;
    void *result;
    if (size != 0 && count > ((size_t)-1) / size)
        return zr_heap_failure();
    total = count * size;
    result = realloc(memory, total);
    if (result && total > old_size)
        ZeroMemory((char *)result + old_size, total - old_size);
    return result;
}

size_t __cdecl
_msize(void *memory)
{
    SIZE_T result;
    if (!memory) {
        errno = EINVAL;
        return (size_t)-1;
    }
    result = HeapSize(GetProcessHeap(), 0, memory);
    if (result == (SIZE_T)-1)
        errno = EINVAL;
    return (size_t)result;
}

void * __cdecl
_expand(void *memory, size_t size)
{
    void *result;
    if (!memory)
        return NULL;
    if (size == 0) {
        free(memory);
        return NULL;
    }
    result = HeapReAlloc(GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY,
                         memory, size);
    return result ? result : zr_heap_failure();
}
