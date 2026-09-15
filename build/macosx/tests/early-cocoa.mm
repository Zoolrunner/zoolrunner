/* Minimal early-OS link/runtime probe, not a full platform compatibility test. */
#import <Cocoa/Cocoa.h>
#include <string.h>

class Counter {
public:
    Counter();
    int value() const;
private:
    int count;
};

Counter::Counter() : count(42) {}
int Counter::value() const { return count; }

int main(int argc, char **argv)
{
    bool foundationOnly = argc == 2 && !strcmp(argv[1], "--foundation-only");
    if (argc != 1 && !foundationOnly)
        return 2;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    Counter counter;
    if (!foundationOnly)
        [NSApplication sharedApplication];
    NSLog(@"Early Cocoa probe: %d", counter.value());
    [pool release];
    return counter.value() == 42 ? 0 : 1;
}
