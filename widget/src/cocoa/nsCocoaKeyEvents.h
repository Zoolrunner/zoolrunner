/* Command-key routing for the historical 32-bit Cocoa application loop. */
#ifndef nsCocoaKeyEvents_h_
#define nsCocoaKeyEvents_h_

#import <Cocoa/Cocoa.h>

#if !defined(__LP64__)
#import <Carbon/Carbon.h>

static BOOL ZRDispatchCommandKey(NSApplication* application, NSEvent* event)
{
  if ([event type] != NSKeyDown ||
      !([event modifierFlags] & NSCommandKeyMask))
    return NO;

  NSResponder* responder = [[application keyWindow] firstResponder];
  // The Suite initializes Cocoa before loading the widget component. Resolve
  // its private view class at event time without adding a bootstrap dependency
  // on that component or intercepting an embedder's native text controls.
  Class geckoView = NSClassFromString(@"ChildView");
  if (!geckoView || ![responder isKindOfClass:geckoView])
    return NO;

  // Keep native application commands (including Quit and Hide) ahead of XUL
  // bindings, as AppKit does before delivering view key equivalents.
  if ([[application mainMenu] performKeyEquivalent:event])
    return YES;

  // The 32-bit backend paints Carbon menus. They are not necessarily present
  // in NSApplication's NSMenu tree, so match their shortcuts explicitly too.
  NSData* bytes = [[event characters] dataUsingEncoding:NSMacOSRomanStringEncoding
                                allowLossyConversion:NO];
  if ([bytes length] == 1) {
    EventRef carbonEvent = NULL;
    UInt32 kind = [event isARepeat] ? kEventRawKeyRepeat : kEventRawKeyDown;
    if (CreateEvent(NULL, kEventClassKeyboard, kind, [event timestamp], 0,
                    &carbonEvent) == noErr) {
      UInt32 code = [event keyCode];
      UInt32 modifiers = cmdKey;
      unsigned flags = [event modifierFlags];
      if (flags & NSShiftKeyMask) modifiers |= shiftKey;
      if (flags & NSAlternateKeyMask) modifiers |= optionKey;
      if (flags & NSControlKeyMask) modifiers |= controlKey;
      if (flags & NSAlphaShiftKeyMask) modifiers |= alphaLock;
      HICommand command = {0};
      command.attributes = kHICommandFromMenu;
      Boolean matched = false;
      if (SetEventParameter(carbonEvent, kEventParamKeyMacCharCodes, typeChar,
                            1, [bytes bytes]) == noErr &&
          SetEventParameter(carbonEvent, kEventParamKeyCode, typeUInt32,
                            sizeof(code), &code) == noErr &&
          SetEventParameter(carbonEvent, kEventParamKeyModifiers, typeUInt32,
                            sizeof(modifiers), &modifiers) == noErr)
        matched = IsMenuKeyEvent(NULL, carbonEvent, 0,
                                 &command.menu.menuRef,
                                 &command.menu.menuItemIndex);
      ReleaseEvent(carbonEvent);
      if (matched) {
        // Use the returned MenuRef: system application menus are not always
        // available through the menu-ID lookup used by the old MenuEvent API.
        if (GetMenuItemCommandID(command.menu.menuRef,
                                 command.menu.menuItemIndex,
                                 &command.commandID) == noErr) {
          ProcessHICommand(&command);
          HiliteMenu(0);
          return YES;
        }
        HiliteMenu(0);
      }
    }
  }

  // Original AppKit does not forward unmatched Command keys to the window's
  // or view's performKeyEquivalent:. Deliver them to the normal Gecko path.
  // Menu-update callbacks can change or close the focused window.
  responder = [[application keyWindow] firstResponder];
  if (![responder isKindOfClass:geckoView])
    return NO;
  [responder keyDown:event];
  return YES;
}
#endif

#endif
