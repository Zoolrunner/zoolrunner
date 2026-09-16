/* Entry point for the copied UTM environment's XULRunner Test.app bundle.
 * Launch Services starts this application before exec preserves its process
 * identity while supplying the standalone runtime's manifest and profile.
 * These paths belong to the dedicated VM, not a general XUL installation. */
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    char *arguments[] = {
        "/tmp/work/zoolrunner-macos-powerpc-xulrunner-target10.0/xulrunner/xulrunner-bin",
        "/tmp/work/zoolrunner-macos-powerpc-xulrunner-target10.0/applications/simple/application.ini",
        "-profile", "/tmp/work/profiles/xulrunner", NULL
    };
    execv(arguments[0], arguments);
    perror("Starting the UTM XULRunner example");
    return 1;
}
