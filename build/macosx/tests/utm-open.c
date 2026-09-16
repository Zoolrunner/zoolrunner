/* Launch a bundle through the original Mac OS X 10.0 Launch Services API.
 * Direct shell launches do not establish the same native application menu
 * registration in the copied installer environment. */
#include <CoreServices/CoreServices.h>
#include <ApplicationServices/ApplicationServices.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    FSRef application;
    OSStatus status;
    if (argc != 2) return 2;
    status = FSPathMakeRef((const UInt8 *)argv[1], &application, NULL);
    if (status == noErr) status = LSOpenFSRef(&application, NULL);
    printf("Launch Services: %ld\n", (long)status);
    return status == noErr ? 0 : 1;
}
