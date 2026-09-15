/* Exercise decoder RGB/alpha buffers across image-surface optimization. */
#include <stdio.h>
#include <string.h>
#include "nsXPCOM.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIComponentRegistrar.h"
#include "nsComponentManagerUtils.h"
#include "nsIImage.h"
#include "gfxIImageFrame.h"
#include "gfxIFormats.h"
#include "nsIInterfaceRequestorUtils.h"

static int check_frame_rows()
{
    nsCOMPtr<gfxIImageFrame> frame =
        do_CreateInstance("@mozilla.org/gfx/image/frame;2");
    if (!frame || NS_FAILED(frame->Init(0, 0, 1, 2, gfxIFormats::RGB_A8, 24)))
        return 20;
    const unsigned char red[] = {255, 0, 0};
    const unsigned char blue[] = {0, 0, 255};
    const unsigned char alpha[] = {255, 128};
    PRUint32 stride;
    if (NS_FAILED(frame->GetImageBytesPerRow(&stride)) || stride != 3 ||
        NS_FAILED(frame->SetImageData(red, 3, 0)) ||
        NS_FAILED(frame->SetImageData(blue, 3, stride)) ||
        NS_FAILED(frame->SetAlphaData(alpha, 1, 0)) ||
        NS_FAILED(frame->SetAlphaData(alpha + 1, 1, 1))) return 21;
    nsCOMPtr<nsIImage> image = do_GetInterface(frame);
    if (!image || !image->GetIsRowOrderTopToBottom() ||
        NS_FAILED(image->LockImagePixels(PR_FALSE))) return 22;
    if (memcmp(image->GetBits(), red, 3) ||
        memcmp(image->GetBits() + stride, blue, 3) ||
        memcmp(image->GetAlphaBits(), alpha, 2)) {
        puts("Image frame RGB/alpha row order mismatch");
        return 23;
    }
    image->UnlockImagePixels(PR_FALSE);
    puts("Image frame RGB/alpha top-down rows passed");
    return 0;
}

static int check_image()
{
    nsresult rv;
    nsCOMPtr<nsIImage> image = do_CreateInstance("@mozilla.org/gfx/image;1", &rv);
    if (NS_FAILED(rv)) return 1;
    if (NS_FAILED(image->Init(3, 1, 24, nsMaskRequirements_kNeeds8Bit)) ||
        NS_FAILED(image->LockImagePixels(PR_FALSE))) return 2;
    const unsigned char rgb[] = {255, 0, 0, 0, 255, 0, 0, 0, 0};
    const unsigned char alpha[] = {255, 128, 0};
    memcpy(image->GetBits(), rgb, sizeof(rgb));
    memcpy(image->GetAlphaBits(), alpha, sizeof(alpha));
    if (NS_FAILED(image->UnlockImagePixels(PR_FALSE)) ||
        NS_FAILED(image->Optimize(NULL)) ||
        NS_FAILED(image->LockImagePixels(PR_FALSE))) return 3;
    for (unsigned int i = 0; i < sizeof(rgb); ++i) {
        if (image->GetBits()[i] != rgb[i]) {
            printf("Image channel %u: got %u expected %u\n", i,
                   image->GetBits()[i], rgb[i]);
            return 4;
        }
    }
    if (memcmp(image->GetAlphaBits(), alpha, sizeof(alpha))) return 5;
    if (NS_FAILED(image->UnlockImagePixels(PR_FALSE))) return 6;
    puts("Image RGB/alpha optimization round trip passed");
    return 0;
}

int main()
{
    setbuf(stdout, NULL);
    nsCOMPtr<nsIServiceManager> manager;
    nsresult rv = NS_InitXPCOM2(getter_AddRefs(manager), NULL, NULL);
    if (NS_FAILED(rv)) return 10;
    int status;
    {
        nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(manager);
        status = !registrar || NS_FAILED(registrar->AutoRegister(NULL)) ?
                 11 : check_frame_rows();
        if (!status) status = check_image();
    }
    manager = NULL;
    NS_ShutdownXPCOM(NULL);
    return status;
}
