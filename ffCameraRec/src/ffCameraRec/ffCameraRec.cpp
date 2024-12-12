
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

int main(int argc, char * [] argv)
{
    av::StdConsole console;

    console.println("Hello world!");
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

    return 0;
}
