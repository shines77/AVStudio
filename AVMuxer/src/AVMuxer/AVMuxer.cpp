
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <stdio.h>
#include <windows.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/log.h>
}

#include "AVConsole.h"

int main()
{
    av::Console console;

    console.print("Hello world!\n");

    return 0;
}
