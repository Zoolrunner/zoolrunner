/* Minimal early-OS link/runtime probe, not a full platform compatibility test. */
#import <Cocoa/Cocoa.h>
#include <string.h>
#include <stdio.h>

@protocol ZoolEarlyValue
- (int)value;
@end

@interface ZoolEarlyObject : NSObject <ZoolEarlyValue>
@end
@implementation ZoolEarlyObject
- (int)value { return 42; }
@end

@interface ZoolEarlyChild : ZoolEarlyObject
@end
@implementation ZoolEarlyChild
- (int)value { return [super value] + 1; }
@end

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
    bool windowTest = argc == 2 && !strcmp(argv[1], "--window");
    if (argc != 1 && !foundationOnly && !windowTest)
        return 2;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    ZoolEarlyChild *object = [[ZoolEarlyChild alloc] init];
    if ([object value] != 43 || ![object conformsToProtocol:@protocol(ZoolEarlyValue)])
        return 4;
    [object release];
    Counter counter;
    if (!foundationOnly)
        [NSApplication sharedApplication];
    if (windowTest) {
        NSWindow *window = [[NSWindow alloc]
            initWithContentRect:NSMakeRect(100, 100, 480, 200)
            styleMask:NSTitledWindowMask | NSClosableWindowMask
            backing:NSBackingStoreBuffered defer:NO];
        NSTextField *label = [[NSTextField alloc] initWithFrame:NSMakeRect(24, 70, 432, 60)];
        [label setStringValue:@"ZoolRunner: original Mac OS X 10.0"];
        [label setEditable:NO];
        [[window contentView] addSubview:label];
        [label release];
        [window setTitle:@"PowerPC Cocoa runtime test"];
        [NSApp finishLaunching];
        [window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
        [window display];
        NSDate *end = [NSDate dateWithTimeIntervalSinceNow:20];
        while ([end timeIntervalSinceNow] > 0) {
            NSEvent *event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:end
                                inMode:NSDefaultRunLoopMode dequeue:YES];
            if (event) [NSApp sendEvent:event];
            [NSApp updateWindows];
        }
        if (![window isVisible]) return 3;
        puts("Original Cocoa window creation and event processing passed");
        [window orderOut:nil];
        [window release];
    }
    NSLog(@"Early Cocoa probe: %d", counter.value());
    [pool release];
    return counter.value() == 42 ? 0 : 1;
}
