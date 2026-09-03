/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * LP64 compatibility implementation for the historical Mac drag helper
 * contract.  Cocoa NSDraggingDestination callbacks drive the drag service
 * directly on 64-bit macOS; the Carbon Drag Manager implementation remains
 * in widget/src/mac for historical targets.
 */

#include "nsDragHelperService.h"

NS_IMPL_ADDREF(nsDragHelperService)
NS_IMPL_RELEASE(nsDragHelperService)
NS_IMPL_QUERY_INTERFACE1(nsDragHelperService, nsIDragHelperService)

nsDragHelperService::nsDragHelperService()
  : mDragOverTimer(nsnull), mDragOverDragRef(nsnull)
{
}

nsDragHelperService::~nsDragHelperService()
{
}

NS_IMETHODIMP
nsDragHelperService::Enter(DragReference aDragRef, nsIEventSink *aSink)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDragHelperService::Tracking(DragReference aDragRef, nsIEventSink *aSink,
                              PRBool *aDropAllowed)
{
  NS_ENSURE_ARG_POINTER(aDropAllowed);
  *aDropAllowed = PR_FALSE;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDragHelperService::Leave(DragReference aDragRef, nsIEventSink *aSink)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDragHelperService::Drop(DragReference aDragRef, nsIEventSink *aSink,
                          PRBool *aAccepted)
{
  NS_ENSURE_ARG_POINTER(aAccepted);
  *aAccepted = PR_FALSE;
  return NS_ERROR_NOT_IMPLEMENTED;
}
