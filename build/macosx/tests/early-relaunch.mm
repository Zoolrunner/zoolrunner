/* Regression test for relaunching the requested program through Cocoa. */
#include <Cocoa/Cocoa.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "MacLaunchHelper.h"
int main(int argc, char **argv)
{
    FILE *file;
    char text[80];
    if (argc == 2 && !strcmp(argv[1], "--child")) {
        file = fopen("/tmp/zool-relaunch-result.txt", "w");
        if (!file) return 2;
        fputs("PASS\n", file);
        return fclose(file) ? 3 : 0;
    }
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    printf("Relaunch argv0=%s bundle=%s\n", argv[0],
           [[[NSBundle mainBundle] executablePath] cString]);
    fflush(stdout);
    unlink("/tmp/zool-relaunch-result.txt");
    char *child[] = {argv[0], "--child", NULL};
    LaunchChildMac(2, child);
    for (int i = 0; i < 15; ++i) {
        file = fopen("/tmp/zool-relaunch-result.txt", "r");
        if (file) {
            char *got = fgets(text, sizeof(text), file);
            fclose(file);
            [pool release];
            if (!got || strcmp(text, "PASS\n")) {
                puts("Relaunch selected the wrong executable"); return 4;
            }
            puts("Application relaunch executable selection passed"); return 0;
        }
        sleep(1);
    }
    [pool release];
    puts("Application relaunch timed out"); return 5;
}
