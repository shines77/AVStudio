
#include <DShow.h>

#if 0
HRESULT WINAPI CopyMediaType(
         AM_MEDIA_TYPE *pmtTarget,
   const AM_MEDIA_TYPE *pmtSource
);

STDAPI CreateAudioMediaType(
   const WAVEFORMATEX  *pwfx,
         AM_MEDIA_TYPE *pmt,
         BOOL          bSetFormat
);

AM_MEDIA_TYPE * WINAPI CreateMediaType(AM_MEDIA_TYPE const * pSrc);

void WINAPI FreeMediaType(AM_MEDIA_TYPE & mt);

void WINAPI DeleteMediaType(AM_MEDIA_TYPE * pmt);
#endif

void _FreeMediaType(AM_MEDIA_TYPE & mt);
void _DeleteMediaType(AM_MEDIA_TYPE * pmt);
