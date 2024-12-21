
#include "stdafx.h"
#include "Mtype.h"

//
// From: https://learn.microsoft.com/zh-cn/windows/win32/directshow/deletemediatype
//

//
// Release the format block for a media type.
//
void _FreeMediaType(AM_MEDIA_TYPE & mediaType)
{
    if (mediaType.cbFormat != 0) {
        CoTaskMemFree((PVOID)mediaType.pbFormat);
        mediaType.cbFormat = 0;
        mediaType.pbFormat = NULL;
    }
    if (mediaType.pUnk != NULL) {
        // pUnk should not be used.
        mediaType.pUnk->Release();
        mediaType.pUnk = NULL;
    }
}

//
// Delete a media type structure that was allocated on the heap.
//
void _DeleteMediaType(AM_MEDIA_TYPE * pmt)
{
    if (pmt != NULL) {
        _FreeMediaType(*pmt);
        CoTaskMemFree(pmt);
    }
}
