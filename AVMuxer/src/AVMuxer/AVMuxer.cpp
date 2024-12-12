
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
#include "CmdParser.h"

int main(int argc, char * argv[])
{
    CmdParser cmd_parser(argc, argv);
    cmd_parser.dump();

    if (cmd_parser.is_contains("v")) {
        console.print("cmd_parser.find(\"v\") = %s\n", cmd_parser.find("v"));
    }

    if (cmd_parser.is_contains("a")) {
        console.print("cmd_parser.find(\"a\") = %s\n", cmd_parser.find("a"));
    }

    if (cmd_parser.is_contains("Host")) {
        console.print("cmd_parser.find(\"Host\") = %s\n", cmd_parser.find("Host"));
    }

    return 0;
}
