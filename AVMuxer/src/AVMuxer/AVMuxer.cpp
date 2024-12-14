
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

#include "error_code.h"
#include "CmdParser.h"
#include "AVConsole.h"
#include "ffHwAccel.h"
#include "ffMuxer.h"

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
#if 1 || (LIBAVFORMAT_BUILD < AV_VERSION_INT(58, 9, 100))
    // 该函数从 avformat 库的 58.9.100 版本开始被废弃
    // (2018-02-06, 大约是 FFmepg 4.0 版本)
    av_register_all();
#endif
    console.set_log_level(av::LogLevel::Error);

    std::string video_file, audio_file;
    std::string output_file = "output_264.mp4";

    CmdParser cmd_parser(argc, argv);
    bool has_key_v = cmd_parser.has_key("v");
    bool has_key_a = cmd_parser.has_key("a");
    bool has_key_o = cmd_parser.has_key("o");

    int args_err = E_NO_ERROR;
    if (!has_key_v && !has_key_a && !has_key_o) {
        if (cmd_parser.no_key_count() >= 3) {
            video_file = cmd_parser.no_key_values(0);
            audio_file = cmd_parser.no_key_values(1);
            output_file = cmd_parser.no_key_values(2);
        }
        else {
            // Error
            args_err = E_ERROR;
        }
    }
    else if (has_key_v && has_key_a) {
        video_file = cmd_parser.get_value("v");
        audio_file = cmd_parser.get_value("a");
        if (has_key_o) {
            output_file = cmd_parser.get_value("o");
        }
        else {
            if (cmd_parser.no_key_count() >= 1) {
                output_file = cmd_parser.no_key_values(0);
            }
        }
    }
    if (args_err != E_NO_ERROR) {
        console.error("Command line input parameter error!");
        goto pause_and_exit;
    }

    {
        int ret;
        bool supported_nvenc = is_support_nvenc();
        console.println("supported_nvenc = %d", (int)supported_nvenc);

        ffHwAccel hwAccel;
        hwAccel.init();

        MuxerSettings settings;
        settings.codec_id_v = AV_CODEC_ID_H264;
        settings.bit_rate_v = 200000;
        settings.width = 0;
        settings.height = 0;
        settings.out_width = 0;
        settings.out_height = 0;
        settings.frame_rate = 30;
        settings.qb = 25;
        settings.pix_fmt = AV_PIX_FMT_YUV420P;

        settings.codec_id_a = AV_CODEC_ID_AAC;
        settings.bit_rate_a = 16000;
        settings.nb_channel = 2;
        settings.sample_rate = 48000;
        settings.sample_fmt = AV_SAMPLE_FMT_FLTP;

        ffMuxer muxer;
        ret = muxer.init(video_file, audio_file, output_file, &settings);
        if (ret == 0) {
            muxer.start();
        }
    }

pause_and_exit:
#if defined(_WIN32) || defined(_WIN64)
    console.println("");
    ::system("pause");
#endif
    return 0;
}
