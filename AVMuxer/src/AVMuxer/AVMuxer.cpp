
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

#include "ffHwAccel.h"

void cmd_parser_test(int argc, char * argv[])
{
    CmdParser cmd_parser(argc, argv);
    cmd_parser.dump();

    if (cmd_parser.has_key("v")) {
        console.print("cmd_parser.get_value(\"v\") = %s\n", cmd_parser.get_value("v"));
    }

    if (cmd_parser.has_key("a")) {
        console.print("cmd_parser.get_value(\"a\") = %s\n", cmd_parser.get_value("a"));
    }

    if (cmd_parser.has_key("Host")) {
        console.print("cmd_parser.get_value(\"Host\") = %s\n", cmd_parser.get_value("Host"));
    }
}

int main(int argc, char * argv[])
{
#if LIBAVFORMAT_BUILD < AV_VERSION_INT(58, 9, 100)
    // 该函数从 avformat 库的 58.9.100 版本开始被废弃
    // (2018-02-06, 大约是 FFmepg 4.0 版本)
    av_register_all();
#endif

    //cmd_parser_test(argc, argv);

    bool supported_nvenc = is_support_nvenc();
    console.println("supported_nvenc = %d", (int)supported_nvenc);

#if defined(_WIN32) || defined(_WIN64)
    console.println("");
    ::system("pause");
#endif
    return 0;
}
