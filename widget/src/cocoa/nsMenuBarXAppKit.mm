/* -*- Mode: ObjC++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * AppKit presentation adapter for the classic Mozilla menu interfaces.
 * The Carbon menu implementation remains authoritative on 32-bit systems.
 */

#import <Cocoa/Cocoa.h>

#include "nsCOMPtr.h"
#include "nsIMenu.h"
#include "nsIMenuBar.h"
#include "nsIMenuItem.h"
#include "nsIMenuListener.h"
#include "nsMenuBarX.h"
#include "nsGUIEvent.h"
#include "CarbonMenuCompat.h"

@interface ZoolAppKitMenuItem : NSMenuItem
{
  nsIMenuItem* mGeckoMenuItem;
}
- (id)initWithGeckoMenuItem:(nsIMenuItem*)aMenuItem;
- (void)activateGeckoMenuItem:(id)aSender;
@end

@interface ZoolAppKitApplicationMenuItem : NSMenuItem
{
  nsMenuBarX* mGeckoMenuBar;
}
- (id)initWithTitle:(NSString*)aTitle
              action:(SEL)aAction
             menuBar:(nsMenuBarX*)aMenuBar
       keyEquivalent:(NSString*)aKeyEquivalent;
- (void)showAbout:(id)aSender;
- (void)showPreferences:(id)aSender;
- (void)quitApplication:(id)aSender;
@end

@interface ZoolAppKitMenu : NSMenu <NSMenuDelegate>
{
  nsIMenu* mGeckoMenu;
  nsMenuBarX* mGeckoMenuBar;
}
- (id)initWithGeckoMenu:(nsIMenu*)aMenu menuBar:(nsMenuBarX*)aMenuBar;
@end

@implementation ZoolAppKitMenuItem

- (id)initWithGeckoMenuItem:(nsIMenuItem*)aMenuItem
{
  nsString label;
  NSString* title;
  PRBool enabled = PR_TRUE;
  PRBool checked = PR_FALSE;

  aMenuItem->GetLabel(label);
  title = [[[NSString alloc] initWithCharacters:(const unichar*)label.get()
                                         length:label.Length()] autorelease];

  self = [super initWithTitle:title
                       action:@selector(activateGeckoMenuItem:)
                keyEquivalent:@""];
  if (self) {
    mGeckoMenuItem = aMenuItem;
    NS_ADDREF(mGeckoMenuItem);
    [self setTarget:self];

    aMenuItem->GetEnabled(&enabled);
    aMenuItem->GetChecked(&checked);
    [self setEnabled:enabled ? YES : NO];
    [self setState:checked ? NSOnState : NSOffState];
  }
  return self;
}

- (void)dealloc
{
  NS_IF_RELEASE(mGeckoMenuItem);
  [super dealloc];
}

- (void)activateGeckoMenuItem:(id)aSender
{
  if (mGeckoMenuItem)
    mGeckoMenuItem->DoCommand();
}

@end

@implementation ZoolAppKitApplicationMenuItem

- (id)initWithTitle:(NSString*)aTitle
              action:(SEL)aAction
             menuBar:(nsMenuBarX*)aMenuBar
       keyEquivalent:(NSString*)aKeyEquivalent
{
  self = [super initWithTitle:aTitle action:aAction keyEquivalent:aKeyEquivalent];
  if (self) {
    mGeckoMenuBar = aMenuBar;
    NS_ADDREF(mGeckoMenuBar);
    [self setTarget:self];
  }
  return self;
}

- (void)dealloc
{
  NS_IF_RELEASE(mGeckoMenuBar);
  [super dealloc];
}

- (void)showAbout:(id)aSender
{
  if (mGeckoMenuBar)
    mGeckoMenuBar->ExecuteAboutCommand();
}

- (void)showPreferences:(id)aSender
{
  if (mGeckoMenuBar)
    mGeckoMenuBar->ExecutePreferencesCommand();
}

- (void)quitApplication:(id)aSender
{
  if (mGeckoMenuBar)
    mGeckoMenuBar->ExecuteQuitCommand();
}

@end

static void
ConstructGeckoMenu(nsIMenu* aMenu)
{
  void* nativeData = nsnull;
  nsCOMPtr<nsIMenuListener> listener = do_QueryInterface(aMenu);

  if (!listener)
    return;

  aMenu->GetNativeData(&nativeData);
  if (nativeData) {
    nsMenuEvent event(PR_TRUE, 0, nsnull);
    event.mCommand = GetMenuID((MenuRef)nativeData);
    listener->MenuSelected(event);
  }
}

static void PopulateAppKitMenu(NSMenu* aMenu, nsIMenu* aGeckoMenu,
                               nsMenuBarX* aGeckoMenuBar);

@implementation ZoolAppKitMenu

- (id)initWithGeckoMenu:(nsIMenu*)aMenu menuBar:(nsMenuBarX*)aMenuBar
{
  nsString label;
  NSString* title;

  aMenu->GetLabel(label);
  title = [[[NSString alloc] initWithCharacters:(const unichar*)label.get()
                                         length:label.Length()] autorelease];
  self = [super initWithTitle:title];
  if (self) {
    mGeckoMenu = aMenu;
    NS_ADDREF(mGeckoMenu);
    // The Gecko menu keeps only a weak pointer to its change manager.  AppKit
    // can release items in the old main menu in any order when a different
    // window installs its menu bar, so keep the manager alive for as long as
    // this native menu can retain the Gecko menu.
    mGeckoMenuBar = aMenuBar;
    NS_ADDREF(mGeckoMenuBar);
    [self setAutoenablesItems:NO];
    [self setDelegate:self];
  }
  return self;
}

- (void)dealloc
{
  [self setDelegate:nil];
  NS_IF_RELEASE(mGeckoMenu);
  NS_IF_RELEASE(mGeckoMenuBar);
  [super dealloc];
}

- (void)menuWillOpen:(NSMenu*)aMenu
{
  PopulateAppKitMenu(aMenu, mGeckoMenu, mGeckoMenuBar);
}

@end


static NSMenu*
CreateAppKitMenu(nsIMenu* aGeckoMenu, nsMenuBarX* aGeckoMenuBar)
{
  return [[ZoolAppKitMenu alloc] initWithGeckoMenu:aGeckoMenu
                                           menuBar:aGeckoMenuBar];
}

static void
PopulateAppKitMenu(NSMenu* aMenu, nsIMenu* aGeckoMenu,
                   nsMenuBarX* aGeckoMenuBar)
{
  PRUint32 count = 0;

  ConstructGeckoMenu(aGeckoMenu);
  [aMenu removeAllItems];
  aGeckoMenu->GetItemCount(count);
  for (PRUint32 i = 0; i < count; ++i) {
    nsCOMPtr<nsISupports> itemSupports;
    aGeckoMenu->GetItemAt(i, *getter_AddRefs(itemSupports));

    nsCOMPtr<nsIMenuItem> geckoItem = do_QueryInterface(itemSupports);
    if (geckoItem) {
      PRBool separator = PR_FALSE;
      geckoItem->IsSeparator(separator);
      if (separator) {
        [aMenu addItem:[NSMenuItem separatorItem]];
      } else {
        ZoolAppKitMenuItem* item =
          [[ZoolAppKitMenuItem alloc] initWithGeckoMenuItem:geckoItem];
        [aMenu addItem:item];
        [item release];
      }
      continue;
    }

    nsCOMPtr<nsIMenu> geckoSubmenu = do_QueryInterface(itemSupports);
    if (geckoSubmenu) {
      nsString submenuLabel;
      geckoSubmenu->GetLabel(submenuLabel);
      NSString* submenuTitle =
        [[[NSString alloc] initWithCharacters:(const unichar*)submenuLabel.get()
                                       length:submenuLabel.Length()] autorelease];
      NSMenuItem* submenuItem =
        [[NSMenuItem alloc] initWithTitle:submenuTitle
                                  action:nil
                           keyEquivalent:@""];
      NSMenu* submenu = CreateAppKitMenu(geckoSubmenu, aGeckoMenuBar);
      [submenuItem setSubmenu:submenu];
      [submenu release];
      [aMenu addItem:submenuItem];
      [submenuItem release];
    }
  }
}

static void
AddApplicationMenu(NSMenu* aMainMenu, nsMenuBarX* aGeckoMenuBar)
{
  NSString* appName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"];
  if (!appName || ![appName length])
    appName = [[NSProcessInfo processInfo] processName];
  NSMenu* appMenu = [[NSMenu alloc] initWithTitle:appName];
  NSMenuItem* appItem =
    [[NSMenuItem alloc] initWithTitle:appName action:nil keyEquivalent:@""];
  NSString* hideTitle = [NSString stringWithFormat:@"Hide %@", appName];
  NSString* quitTitle = [NSString stringWithFormat:@"Quit %@", appName];
  NSString* aboutTitle = [NSString stringWithFormat:@"About %@", appName];
  ZoolAppKitApplicationMenuItem* commandItem;

  commandItem = [[ZoolAppKitApplicationMenuItem alloc]
    initWithTitle:aboutTitle action:@selector(showAbout:)
    menuBar:aGeckoMenuBar keyEquivalent:@""];
  [appMenu addItem:commandItem];
  [commandItem release];
  [appMenu addItem:[NSMenuItem separatorItem]];
  commandItem = [[ZoolAppKitApplicationMenuItem alloc]
    initWithTitle:@"Preferences..." action:@selector(showPreferences:)
    menuBar:aGeckoMenuBar keyEquivalent:@","];
  [appMenu addItem:commandItem];
  [commandItem release];
  [appMenu addItem:[NSMenuItem separatorItem]];
  [appMenu addItemWithTitle:hideTitle action:@selector(hide:) keyEquivalent:@"h"];
  [appMenu addItem:[NSMenuItem separatorItem]];
  commandItem = [[ZoolAppKitApplicationMenuItem alloc]
    initWithTitle:quitTitle action:@selector(quitApplication:)
    menuBar:aGeckoMenuBar keyEquivalent:@"q"];
  [appMenu addItem:commandItem];
  [commandItem release];
  [appItem setSubmenu:appMenu];
  [aMainMenu addItem:appItem];
  [appItem release];
  [appMenu release];
}

void
InstallAppKitMenuBar(nsMenuBarX* aMenuBar)
{
  NSMenu* mainMenu = [[NSMenu alloc] initWithTitle:@""];
  PRUint32 count = 0;

  AddApplicationMenu(mainMenu, aMenuBar);
  aMenuBar->GetMenuCount(count);
  for (PRUint32 i = 0; i < count; ++i) {
    nsCOMPtr<nsIMenu> geckoMenu;
    aMenuBar->GetMenuAt(i, *getter_AddRefs(geckoMenu));
    if (!geckoMenu)
      continue;

    nsString label;
    geckoMenu->GetLabel(label);
    NSString* title =
      [[[NSString alloc] initWithCharacters:(const unichar*)label.get()
                                     length:label.Length()] autorelease];
    NSMenuItem* item =
      [[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""];
    NSMenu* submenu = CreateAppKitMenu(geckoMenu, aMenuBar);
    [item setSubmenu:submenu];
    [submenu release];
    [mainMenu addItem:item];
    [item release];
  }

  [NSApp setMainMenu:mainMenu];
  [mainMenu release];
}
