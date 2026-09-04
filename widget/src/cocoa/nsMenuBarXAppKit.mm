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
#include "nsGUIEvent.h"
#include "CarbonMenuCompat.h"

@interface ZoolAppKitMenuItem : NSMenuItem
{
  nsIMenuItem* mGeckoMenuItem;
}
- (id)initWithGeckoMenuItem:(nsIMenuItem*)aMenuItem;
- (void)activateGeckoMenuItem:(id)aSender;
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

static NSMenu*
CreateAppKitMenu(nsIMenu* aGeckoMenu)
{
  nsString label;
  NSString* title;
  NSMenu* menu;
  PRUint32 count = 0;

  aGeckoMenu->GetLabel(label);
  title = [[[NSString alloc] initWithCharacters:(const unichar*)label.get()
                                         length:label.Length()] autorelease];
  menu = [[[NSMenu alloc] initWithTitle:title] autorelease];
  [menu setAutoenablesItems:NO];

  ConstructGeckoMenu(aGeckoMenu);
  aGeckoMenu->GetItemCount(count);
  for (PRUint32 i = 0; i < count; ++i) {
    nsCOMPtr<nsISupports> itemSupports;
    aGeckoMenu->GetItemAt(i, *getter_AddRefs(itemSupports));

    nsCOMPtr<nsIMenuItem> geckoItem = do_QueryInterface(itemSupports);
    if (geckoItem) {
      PRBool separator = PR_FALSE;
      geckoItem->IsSeparator(separator);
      if (separator) {
        [menu addItem:[NSMenuItem separatorItem]];
      } else {
        ZoolAppKitMenuItem* item =
          [[[ZoolAppKitMenuItem alloc] initWithGeckoMenuItem:geckoItem] autorelease];
        [menu addItem:item];
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
        [[[NSMenuItem alloc] initWithTitle:submenuTitle
                                   action:nil
                            keyEquivalent:@""] autorelease];
      [submenuItem setSubmenu:CreateAppKitMenu(geckoSubmenu)];
      [menu addItem:submenuItem];
    }
  }

  return menu;
}

static void
AddApplicationMenu(NSMenu* aMainMenu)
{
  NSString* appName = [[NSProcessInfo processInfo] processName];
  NSMenu* appMenu = [[[NSMenu alloc] initWithTitle:appName] autorelease];
  NSMenuItem* appItem =
    [[[NSMenuItem alloc] initWithTitle:appName action:nil keyEquivalent:@""] autorelease];
  NSString* hideTitle = [NSString stringWithFormat:@"Hide %@", appName];
  NSString* quitTitle = [NSString stringWithFormat:@"Quit %@", appName];

  [appMenu addItemWithTitle:hideTitle action:@selector(hide:) keyEquivalent:@"h"];
  [appMenu addItem:[NSMenuItem separatorItem]];
  [appMenu addItemWithTitle:quitTitle action:@selector(terminate:) keyEquivalent:@"q"];
  [appItem setSubmenu:appMenu];
  [aMainMenu addItem:appItem];
}

void
InstallAppKitMenuBar(nsIMenuBar* aMenuBar)
{
  NSMenu* mainMenu = [[[NSMenu alloc] initWithTitle:@""] autorelease];
  PRUint32 count = 0;

  AddApplicationMenu(mainMenu);
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
      [[[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""] autorelease];
    [item setSubmenu:CreateAppKitMenu(geckoMenu)];
    [mainMenu addItem:item];
  }

  [NSApp setMainMenu:mainMenu];
}
