#pragma once

#include <dshow.h>
#include <tchar.h>

#include <string>
#include <vector>

#ifdef _DEBUG
#pragma comment(lib, "strmbased.lib")
#else
#pragma comment(lib, "strmbase.lib")
#endif
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "quartz.lib")

class DShowDevice
{
public:
    DShowDevice();
    ~DShowDevice();

    HRESULT CreateInterfaces();
    HRESULT StopAndReleaseInterfaces();

    void init();

    // 枚举视频采集设备
    int ListVideoDevices();

    // 枚举音频采集设备
    int ListAudioDevices();

    std::vector<std::string> videoDeviceList_;
    std::vector<std::string> audioDeviceList_;

protected:
    void InitInterfaces();
    void ReleaseInterfaces();

    IGraphBuilder *         pFilterGraph_;          // filter graph (Manager)
    ICaptureGraphBuilder2 * pCaptureBuilder_;       // capture graph (Builder)
};
