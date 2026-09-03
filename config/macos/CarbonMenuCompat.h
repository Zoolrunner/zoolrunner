/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#ifndef mozilla_config_macos_CarbonMenuCompat_h
#define mozilla_config_macos_CarbonMenuCompat_h

/*
 * The 64-bit HIToolbox library still exports the Carbon Menu Manager entry
 * points used by the original Cocoa widget implementation, but current SDK
 * headers hide their declarations behind !__LP64__.  Keep the declarations
 * local to the 64-bit compatibility path so the historical 32-bit headers and
 * implementation remain untouched.
 */
#if defined(__APPLE__) && defined(__LP64__)

#include <Carbon/Carbon.h>

extern "C" {
extern OSStatus CreateNewMenu(MenuID, MenuAttributes, MenuRef *);
extern MenuID GetMenuID(MenuRef);
extern UInt16 CountMenuItems(MenuRef);
extern OSStatus SetMenuTitleWithCFString(MenuRef, CFStringRef);
extern void DeleteMenuItem(MenuRef, MenuItemIndex);
extern OSStatus DeleteMenuItems(MenuRef, MenuItemIndex, ItemCount);
extern OSStatus InsertMenuItemTextWithCFString(MenuRef, CFStringRef,
                                               MenuItemIndex,
                                               MenuItemAttributes,
                                               MenuCommand);
extern OSStatus AppendMenuItemTextWithCFString(MenuRef, CFStringRef,
                                               MenuItemAttributes,
                                               MenuCommand,
                                               MenuItemIndex *);
extern void EnableMenuItem(MenuRef, MenuItemIndex);
extern void DisableMenuItem(MenuRef, MenuItemIndex);
extern void CheckMenuItem(MenuRef, MenuItemIndex, Boolean);
extern OSErr SetMenuItemCommandID(MenuRef, MenuItemIndex, MenuCommand);
extern OSErr GetMenuItemCommandID(MenuRef, MenuItemIndex, MenuCommand *);
extern OSErr SetMenuItemModifiers(MenuRef, MenuItemIndex, UInt8);
extern OSStatus SetMenuItemCommandKey(MenuRef, MenuItemIndex, Boolean, UInt16);
extern OSStatus SetMenuItemHierarchicalMenu(MenuRef, MenuItemIndex, MenuRef);
extern void EnableMenuCommand(MenuRef, MenuCommand);
extern void DisableMenuCommand(MenuRef, MenuCommand);
extern MenuRef AcquireRootMenu(void);
extern OSStatus SetRootMenu(MenuRef);
extern void DrawMenuBar(void);
extern EventTargetRef GetMenuEventTarget(MenuRef);
}

static inline OSStatus
ZRReleaseMenu(MenuRef aMenu)
{
  CFRelease(aMenu);
  return noErr;
}

static inline void
ZRSetItemCmd(MenuRef aMenu, MenuItemIndex aItem, CharParameter aCommand)
{
  SetMenuItemCommandKey(aMenu, aItem, false, (UInt16)(UInt8)aCommand);
}

static inline void
ZRInsertMenuItem(MenuRef aMenu, ConstStr255Param aText,
                 MenuItemIndex aAfterItem)
{
  CFStringRef text = CFStringCreateWithPascalString(kCFAllocatorDefault,
                                                     aText,
                                                     kCFStringEncodingMacRoman);
  if (text) {
    InsertMenuItemTextWithCFString(aMenu, text, aAfterItem, 0, 0);
    CFRelease(text);
  }
}

static inline void
ZRAppendMenu(MenuRef aMenu, ConstStr255Param aText)
{
  CFStringRef text = CFStringCreateWithPascalString(kCFAllocatorDefault,
                                                     aText,
                                                     kCFStringEncodingMacRoman);
  if (text) {
    AppendMenuItemTextWithCFString(aMenu, text, 0, 0, NULL);
    CFRelease(text);
  }
}

static inline OSStatus
ZRSetMenuTitle(MenuRef aMenu, ConstStr255Param aText)
{
  OSStatus result = memFullErr;
  CFStringRef text = CFStringCreateWithPascalString(kCFAllocatorDefault,
                                                     aText,
                                                     kCFStringEncodingMacRoman);
  if (text) {
    result = SetMenuTitleWithCFString(aMenu, text);
    CFRelease(text);
  }
  return result;
}

#define ReleaseMenu ZRReleaseMenu
#define SetItemCmd ZRSetItemCmd
#define InsertMenuItem ZRInsertMenuItem
#define AppendMenu ZRAppendMenu
#define SetMenuTitle ZRSetMenuTitle

#endif /* __APPLE__ && __LP64__ */

#endif /* mozilla_config_macos_CarbonMenuCompat_h */
