/* Exercise Objective-C loading after Csu startup, without linking Cocoa. */
#include <mach-o/dyld.h>
#include <stdio.h>

int main(void)
{
    void *(*get_class)(const char *);
    void *(*get_selector)(const char *);
    void *(*send_message)(void *, void *, ...);
    void *object;
    if (!NSAddLibraryWithSearching(
            "/System/Library/Frameworks/Cocoa.framework/Cocoa")) return 1;
    get_class = (void *(*)(const char *))NSAddressOfSymbol(
        NSLookupAndBindSymbol("_objc_getClass"));
    get_selector = (void *(*)(const char *))NSAddressOfSymbol(
        NSLookupAndBindSymbol("_sel_registerName"));
    send_message = (void *(*)(void *, void *, ...))NSAddressOfSymbol(
        NSLookupAndBindSymbol("_objc_msgSend"));
    if (!get_class || !get_selector || !send_message) return 2;
    object = send_message(get_class("NSObject"), get_selector("new"));
    if (!object) return 3;
    send_message(object, get_selector("release"));
    puts("Deferred Cocoa initialization with eager binding passed");
    return 0;
}
