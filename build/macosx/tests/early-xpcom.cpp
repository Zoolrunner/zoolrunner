/* Target-OS diagnostics for component discovery and native loading. */
#include <stdio.h>
#include <Carbon/Carbon.h>
#include "nsXPCOM.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIProperties.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsISimpleEnumerator.h"
#include "nsString.h"
#include "prlink.h"
#include "prerror.h"

int main(int argc, char **argv)
{
    FSRef fsref;
    Boolean directory = false;
    OSStatus fsstatus = FSPathMakeRef((const UInt8 *) "/tmp/components", &fsref, &directory);
    printf("FSPathMakeRef: %ld directory=%d\n", (long)fsstatus, directory);
    CFURLRef url = CFURLCreateWithFileSystemPath(NULL, CFSTR("/tmp/components"),
                                                kCFURLPOSIXPathStyle, true);
    printf("CFURLGetFSRef: %d\n", CFURLGetFSRef(url, &fsref));
    CFRelease(url);
    nsCOMPtr<nsIServiceManager> manager;
    nsresult rv = NS_InitXPCOM2(getter_AddRefs(manager), NULL, NULL);
    printf("InitXPCOM: %08lx\n", (unsigned long)rv);
    if (NS_FAILED(rv)) return 1;
    {
        nsCOMPtr<nsIFile> dir;
        rv = NS_GetSpecialDirectory(NS_XPCOM_COMPONENT_DIR, getter_AddRefs(dir));
        printf("component directory: %08lx\n", (unsigned long)rv);
        if (dir) {
            nsCString path;
            dir->GetNativePath(path);
            printf("path: %s\n", path.get());
            nsCOMPtr<nsISimpleEnumerator> entries;
            rv = dir->GetDirectoryEntries(getter_AddRefs(entries));
            printf("enumerator: %08lx\n", (unsigned long)rv);
            PRBool more = PR_FALSE;
            while (entries && NS_SUCCEEDED(rv = entries->HasMoreElements(&more)) && more) {
                nsCOMPtr<nsISupports> item;
                entries->GetNext(getter_AddRefs(item));
                nsCOMPtr<nsIFile> file = do_QueryInterface(item);
                if (file) {
                    file->GetNativePath(path);
                    printf("entry: %s\n", path.get());
                }
            }
            printf("enumeration ended: %08lx\n", (unsigned long)rv);
        }
        if (argc > 1) {
            PRLibrary *library = PR_LoadLibrary(argv[1]);
            printf("load %s: %p error=%d\n", argv[1], library, PR_GetError());
            if (library)
                printf("NSGetModule: %p\n", PR_FindSymbol(library, "NSGetModule"));
        }
        nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(manager);
        rv = registrar->AutoRegister(NULL);
        printf("AutoRegister: %08lx\n", (unsigned long)rv);
        nsCOMPtr<nsISupports> runtime;
        rv = manager->GetServiceByContractID("@mozilla.org/js/xpc/RuntimeService;1",
            NS_GET_IID(nsISupports), getter_AddRefs(runtime));
        printf("runtime service: %08lx %p\n", (unsigned long)rv, runtime.get());
    }
    manager = NULL;
    NS_ShutdownXPCOM(NULL);
    return NS_FAILED(rv) ? 1 : 0;
}
