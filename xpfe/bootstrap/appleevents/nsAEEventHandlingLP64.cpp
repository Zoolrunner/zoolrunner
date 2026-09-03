/* Minimal LP64 bridge for the legacy XPFE Apple Event entry points.
 *
 * The full scripting implementation depends on QuickDraw WindowRef,
 * FSSpec, AEPackObject and other interfaces removed from the 64-bit macOS
 * ABI. Cocoa continues to deliver normal application open/quit events; this
 * bridge keeps the historical startup interface intact and fails unsupported
 * scripting operations safely until a Cocoa object-model adapter is added.
 */

#include "nsAEEventHandling.h"

OSErr
CreateAEHandlerClasses(Boolean suspendFirstEvent)
{
  (void)suspendFirstEvent;
  return noErr;
}

OSErr
GetSuspendedEvent(AppleEvent* event, AppleEvent* reply)
{
  (void)event;
  (void)reply;
  return errAEEventNotHandled;
}

OSErr
ResumeAEHandling(AppleEvent* event, AppleEvent* reply,
                 Boolean dispatchEvent)
{
  (void)event;
  (void)reply;
  (void)dispatchEvent;
  return errAEEventNotHandled;
}

OSErr
ShutdownAEHandlerClasses(void)
{
  return noErr;
}
