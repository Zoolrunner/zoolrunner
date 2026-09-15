/* Original-OS find pasteboard bridge. Same MPL/GPL/LGPL terms as
 * nsWebBrowserFind.cpp in this directory. */
#import <Cocoa/Cocoa.h>
#include "nsString.h"

void ZRReadFindPasteboard(nsAString &value)
{
    NSPasteboard *board = [NSPasteboard pasteboardWithName:NSFindPboard];
    NSString *string = [board stringForType:NSStringPboardType];
    if (!string)
        return;
    unsigned int length = [string length];
    nsAutoString buffer;
    buffer.SetLength(length);
    if (buffer.Length() != length)
        return;
    [string getCharacters:(unichar *)buffer.BeginWriting()];
    value.Assign(buffer);
}

void ZRWriteFindPasteboard(const nsAString &value)
{
    NSString *string = [[NSString alloc]
        initWithCharacters:(const unichar *)PromiseFlatString(value).get()
                    length:value.Length()];
    if (!string)
        return;
    NSPasteboard *board = [NSPasteboard pasteboardWithName:NSFindPboard];
    [board declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
    [board setString:string forType:NSStringPboardType];
    [string release];
}
