/* Run against the shipped NSPR on original early Darwin, not the host NSPR. */
#include "nspr.h"
#include "prpriv.h"
#include "private/pprthred.h"
#include <stdio.h>
#include <string.h>

static PRInt32 counter, done;

static void PR_CALLBACK worker(void *argument)
{
    while (!PR_AtomicAdd(&done, 0))
        PR_AtomicIncrement(&counter);
}

int main(int argc, char **argv)
{
    PRThread *thread;
    PRSem *sem;
    PRLibrary *library;
    PRAddrInfo *address;
    PRNetAddr net;
    int round;
    int (*value)(void);
    setbuf(stdout, NULL);
    if (argc != 2 && argc != 3) return 1;
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    puts("NSPR: named semaphores");
    sem = PR_OpenSemaphore("zoolrunner-early-probe", PR_SEM_CREATE | PR_SEM_EXCL, 0600, 1);
    if (!sem || PR_WaitSemaphore(sem) != PR_SUCCESS ||
        PR_PostSemaphore(sem) != PR_SUCCESS || PR_CloseSemaphore(sem) != PR_SUCCESS ||
        PR_DeleteSemaphore("zoolrunner-early-probe") != PR_SUCCESS) {
        printf("semaphore error %d/%d\n", PR_GetError(), PR_GetOSError());
        return 2;
    }
    puts("NSPR: original dyld library loading");
    library = PR_LoadLibrary(argv[1]);
    if (!library) { printf("load error %d/%d\n", PR_GetError(), PR_GetOSError()); return 3; }
    value = (int (*)(void))PR_FindSymbol(library, "zr_nspr_plugin_value");
    if (!value || value() != 42 || PR_FindSymbol(library, "zr_missing_symbol")) return 4;
    if (PR_UnloadLibrary(library) != PR_SUCCESS) return 5;
    if (argc == 3) {
        puts("NSPR: missing dependency and subsequent valid load");
        library = PR_LoadLibrary(argv[2]);
        if (library || PR_GetError() != PR_LOAD_LIBRARY_ERROR) return 12;
        library = PR_LoadLibrary(argv[1]);
        if (!library) return 13;
        value = (int (*)(void))PR_FindSymbol(library, "zr_nspr_plugin_value");
        if (!value || value() != 42 || PR_UnloadLibrary(library) != PR_SUCCESS) return 14;
    }
    puts("NSPR: numeric IPv4 address lookup");
    address = PR_GetAddrInfoByName("127.0.0.1", PR_AF_INET,
                                  PR_AI_NOCANONNAME | PR_AI_ADDRCONFIG);
    if (!address || !PR_EnumerateAddrInfo(NULL, address, 0, &net) ||
        net.inet.ip != PR_htonl(0x7f000001)) return 6;
    PR_FreeAddrInfo(address);
    puts("NSPR: suspend, register snapshot and resume");
    thread = PR_CreateThreadGCAble(PR_USER_THREAD, worker, NULL, PR_PRIORITY_NORMAL,
                                  PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);
    if (!thread) return 7;
    while (!PR_AtomicAdd(&counter, 0)) PR_Sleep(PR_MillisecondsToInterval(1));
    for (round = 0; round < 10; ++round) {
        PRInt32 before;
        PRWord *registers;
        int count;
        PR_SuspendAll();
        before = PR_AtomicAdd(&counter, 0);
        registers = PR_GetGCRegisters(thread, 0, &count);
        if (!registers || count < 32 || !PR_GetSP(thread)) return 8;
        PR_Sleep(PR_MillisecondsToInterval(10));
        if (PR_AtomicAdd(&counter, 0) != before) return 9;
        PR_ResumeAll();
        while (PR_AtomicAdd(&counter, 0) == before) PR_Sleep(PR_MillisecondsToInterval(1));
    }
    PR_AtomicSet(&done, 1);
    if (PR_JoinThread(thread) != PR_SUCCESS) return 10;
    puts("NSPR: all early-platform checks passed");
    return PR_Cleanup() == PR_SUCCESS ? 0 : 11;
}
