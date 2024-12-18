
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

#include "global.h"
#include "CameraRecoder.h"

#define OUTPUT_FILENAME "output.mp4"

void console_test()
{
    console.set_log_level(av::LogLevel::Error);

    console.fatal("This a fatal message.");
    console.error("This a error message.");
    console.info("This a info message.");
    console.verbose("This a verbose message.");

    if (console.try_fatal()) {
        console << "This a try fatal message." << std::endl;
    }
    if (console.try_error()) {
        console << "This a try error message." << std::endl;
    }
    if (console.try_info()) {
        console << "This a try info message." << std::endl;
    }
    if (console.try_verbose()) {
        console << "This a try verbose message." << std::endl;
    }

    av::StdFileLog file_log("test.log", false);
    file_log.set_log_level(av::LogLevel::Info);

    file_log.println("Hello world!");
    file_log.fatal("This a fatal message.");
    file_log.error("This a error message.");
    file_log.info("This a info message.");
    file_log.verbose("This a verbose message.");

    if (file_log.try_fatal()) {
        file_log << "This a try fatal message." << std::endl;
    }
    if (file_log.try_error()) {
        file_log << "This a try error message." << std::endl;
    }
    if (file_log.try_info()) {
        file_log << "This a try info message." << std::endl;
    }
    if (file_log.try_verbose()) {
        file_log << "This a try verbose message." << std::endl;
    }
}

int main(int argc, const char * argv[])
{
#if LIBAVFORMAT_BUILD < AV_VERSION_INT(58, 9, 100)
    // 该函数从 avformat 库的 58.9.100 版本开始被废弃
    // (2018-02-06, 大约是 FFmepg 4.0 版本)
    av_register_all();
#endif

    avdevice_register_all();
    avformat_network_init();

    av_log_set_level(AV_LOG_INFO);
    console.set_log_level(av::LogLevel::Info);

    int ret;

    CameraRecoder camera_recoder;
    ret = camera_recoder.init(OUTPUT_FILENAME);
    if (ret == 0) {
        console.info("camera_recoder.start();");
        camera_recoder.start();
    }

    avformat_network_deinit();

#if defined(_WIN32) || defined(_WIN64)
    //console.println("");
    //::system("pause");
#endif
    return 0;
}
