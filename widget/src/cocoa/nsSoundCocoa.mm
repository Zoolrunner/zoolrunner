/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* LP64 Cocoa sound implementation.  Historical targets retain the Sound
 * Manager and QuickTime implementation in widget/src/mac/nsSound.cpp. */

#import <Cocoa/Cocoa.h>

#include "nsSound.h"
#include "nsIURL.h"
#include "nsString.h"

nsSound::nsSound()
{
}

nsSound::~nsSound()
{
}

NS_IMPL_ISUPPORTS1(nsSound, nsISound)

NS_IMETHODIMP
nsSound::Beep()
{
  NSBeep();
  return NS_OK;
}

NS_IMETHODIMP
nsSound::Init()
{
  return NS_OK;
}

NS_IMETHODIMP
nsSound::PlaySystemSound(const char *aSoundName)
{
  NS_ENSURE_ARG(aSoundName);
  NSString *name = [NSString stringWithUTF8String:aSoundName];
  NSSound *sound = name ? [NSSound soundNamed:name] : nil;
  if (!sound || ![sound play])
    return Beep();
  return NS_OK;
}

NS_IMETHODIMP
nsSound::Play(nsIURL *aURL)
{
  NS_ENSURE_ARG(aURL);
  nsCAutoString spec;
  nsresult rv = aURL->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);
  NSString *urlString = [NSString stringWithUTF8String:spec.get()];
  NSURL *url = urlString ? [NSURL URLWithString:urlString] : nil;
  NSSound *sound = url ? [[[NSSound alloc] initWithContentsOfURL:url
                                                    byReference:YES]
                           autorelease] : nil;
  if (!sound || ![sound play])
    return NS_ERROR_FAILURE;
  return NS_OK;
}
