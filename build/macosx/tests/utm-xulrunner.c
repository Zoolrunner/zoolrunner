/* Entry point for the copied UTM environment's XULRunner Test.app bundle.
 * Follow nsXULStubOSX.cpp: preserve the bundle launcher's argv[0] so Cocoa
 * retains its application identity, and supply the actual runtime separately.
 * These paths belong to the dedicated VM, not a general XUL installation. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    char *binary = "/tmp/work/zoolrunner-macos-powerpc-xulrunner-target10.0/xulrunner/xulrunner-bin";
    char *arguments[] = {
        NULL, "-profile", "/tmp/work/profiles/xulrunner", NULL
    };
    if (argc < 1) return 2;
    arguments[0] = argv[0];
    if (setenv("XRE_BINARY_PATH", binary, 1) ||
        setenv("DYLD_LIBRARY_PATH",
               "/tmp/work/zoolrunner-macos-powerpc-xulrunner-target10.0/xulrunner", 1) ||
        setenv("XUL_APP_FILE",
               "/tmp/work/zoolrunner-macos-powerpc-xulrunner-target10.0/applications/simple/application.ini", 1)) {
        perror("Preparing the UTM XULRunner example");
        return 1;
    }
    execv(binary, arguments);
    perror("Starting the UTM XULRunner example");
    return 1;
}
