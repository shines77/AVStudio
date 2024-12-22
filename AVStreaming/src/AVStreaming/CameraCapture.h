#pragma once

#include <dshow.h>

#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__

#include <qedit.h>      // For ISampleGrabber
#include <stdint.h>
#include <tchar.h>

#include <string>
#include <vector>

#include "string_utils.h"

#ifdef _DEBUG
#pragma comment(lib, "strmbased.lib")
#else
#pragma comment(lib, "strmbase.lib")
#endif
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "quartz.lib")

#define MODE_PREVIEW_VIDEO          1
#define MODE_RECORD_VIDEO           2
#define MODE_LOCAL_VIDEO            3

#define ACTION_STOP_PREVIEW         1
#define ACTION_STOP_RECORD          2
#define ACTION_STOP_LOCAL_VIDEO     3

// Application-defined message to notify app of filtergraph events
#define WM_GRAPH_NOTIFY             (WM_APP + 1)

struct CamVideoConfig
{
	SIZE InputSize;
    SIZE MinOutputSize;
    SIZE MaxOutputSize;
    double MinFPS;
    double MaxFPS;
    ULONG lSampleSize;
    ULONG cbFormat;
	int FormatIndex;
 
	CamVideoConfig() {
		InputSize.cx = 0;
        InputSize.cy = 0;
        MinOutputSize.cx = 0;
        MinOutputSize.cy = 0;
        MaxOutputSize.cx = 0;
        MaxOutputSize.cy = 0;
        MinFPS = 0.0;
        MaxFPS = 0.0;
        lSampleSize = 0;
        cbFormat = -1;
		FormatIndex = -1;
	}

	CamVideoConfig(const CamVideoConfig & other) {
		*this = other;
	}

	CamVideoConfig & operator = (const CamVideoConfig & other) {
		InputSize = other.InputSize;
		MinOutputSize = other.MinOutputSize;
        MaxOutputSize = other.MaxOutputSize;
        MinFPS = other.MinFPS;
        MaxFPS = other.MaxFPS;
        lSampleSize = other.lSampleSize;
        cbFormat = other.cbFormat;
		FormatIndex = other.FormatIndex;
		return *this;
	}
};

struct CamAudioConfig
{
    ULONG Channels;
    ULONG BitsPerSample;
	ULONG SampleFrequency;
    ULONG lSampleSize;
    ULONG cbFormat;
	int FormatIndex;
 
	CamAudioConfig() {
		Channels = 0;
        BitsPerSample = 0;
        SampleFrequency = 0;
        lSampleSize = 0;
        cbFormat = -1;
		FormatIndex = -1;
	}

	CamAudioConfig(const CamAudioConfig & other) {
		*this = other;
	}

	CamAudioConfig & operator = (const CamAudioConfig & other) {
		Channels = other.Channels;
		BitsPerSample = other.BitsPerSample;
        SampleFrequency = other.SampleFrequency;
        lSampleSize = other.lSampleSize;
        cbFormat = other.cbFormat;
		FormatIndex = other.FormatIndex;
		return *this;
	}
};

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVStream;

enum AVPixelFormat;
enum AVSampleFormat;

class CameraCapture
{
public:
    CameraCapture(HWND hwndPreview = NULL);
    ~CameraCapture(void);

    std::vector<std::tstring>   videoDeviceList_;
    std::vector<std::tstring>   audioDeviceList_;

    std::vector<CamVideoConfig> videoConfigures_;
    std::vector<CamAudioConfig> audioConfigures_;
    std::vector<int>            videoFPSList_;

    static const int kFrameIntervalPerSecond = 10000000;

    enum PLAY_STATE {
        Unknown = -1,
        Stopped,
        Running,
        Paused
    };

    HRESULT CreateEnv();
    HRESULT Stop();
    void OnClose();
    void Release();

    HRESULT InitCaptureFilters();
    void FreeCaptureFilters();

    HWND GetPreviewHwnd() const {
        return hwndPreview_;
    }
    HWND SetPreviewHwnd(HWND hwndPreview, bool bAttachTo = false);

    PLAY_STATE GetPlayState() const {
        return playState_;
    }

    IVideoWindow * GetVideoWindow() const {
        return pVideoWindow_;
    }

    HRESULT WindowStateChange(BOOL isShow);
    HRESULT ChangePreviewState(PLAY_STATE playState = PLAY_STATE::Running);
    HRESULT HandleGraphEvent(void);

    void ResizeVideoWindow(HWND hwndPreview = NULL);
    bool AttachToVideoWindow(HWND hwndPreview);

    size_t EnumVideoConfigures();
    size_t EnumAudioConfigures();

    size_t EnumAVDevices(std::vector<std::tstring> & deviceList, bool isVideo);

    // 枚举视频采集设备, 返回设备数量
    size_t EnumVideoDevices();

    // 枚举音频采集设备
    size_t EnumAudioDevices();

    // 枚举视频压缩格式
    size_t EnumVideoCompressFormat();

    // 枚举音频压缩格式
    size_t EnumAudioCompressFormat();

    // 根据选择的设备绑定 Video Capture Filter
    HRESULT BindVideoFilter(const TCHAR * videoDevice);
    // 根据选择的设备绑定 Audio Capture Filter
    HRESULT BindAudioFilter(const TCHAR * audioDevice);

    HRESULT StartPreview();
    HRESULT StopPreview();

    HRESULT StartCapture();
    HRESULT StopCapture();

    // 渲染预览窗口
    bool Render(int mode, TCHAR * videoPath = NULL,
                const TCHAR * videoDevice = NULL,
                const TCHAR * audioDevice = NULL);

    // 停止当前操作
    bool StopCurrentOperating(int _stop_type = 0);

    // 暂停播放本地视频
    bool PausePlayingLocalVideo();

    // 继续播放本地视频
    bool ContinuePlayingLocalVideo();

protected:
    void InitEnv();

    static const TCHAR * GetDeviceType(bool isVideo);
    static const wchar_t * GetDeviceFilterName(bool isVideo);
    static REFCLSID GetDeviceCategory(bool isVideo);

    void RemoveDownstream(IBaseFilter * filter);
    void TearDownGraph();

    HRESULT BindDeviceFilter(IBaseFilter ** ppDeviceFilter, IMoniker ** ppDeviceMoniker,
                             int nIndex, IMoniker * pMoniker,
                             const TCHAR * inDeviceName, bool isVideo);
    HRESULT BindDeviceFilter(IBaseFilter ** ppDeviceFilter, IMoniker ** ppDeviceMoniker,
                             const TCHAR * inDeviceName, bool isVideo);
    void ReleaseDeviceFilter(IBaseFilter ** ppFilter);

private:
    HWND                    hwndPreview_;           // 预览窗口句柄
    PLAY_STATE              playState_;             // 播放状态

    IGraphBuilder *         pFilterGraph_;          // filter graph (Manager)
    ICaptureGraphBuilder2 * pCaptureBuilder_;       // capture graph (Builder)

    IBaseFilter *           pVideoFilter_;          // Video filter
    IBaseFilter *           pAudioFilter_;          // Audio filter

    IBaseFilter *           pVideoMux_;             // Video file muxer (用于保存视频文件)
    IVideoWindow *          pVideoWindow_;          // 视频播放窗口
    IMediaControl *         pVideoMediaControl_;    // 摄像头流媒体的控制开关（控制其开始、暂停、结束）
    IMediaEventEx *         pVideoMediaEvent_;      // 摄像头流媒体的控制事件

    IMoniker *              pVideoMoniker_;         // Video 设备
    IMoniker *              pAudioMoniker_;         // Audio 设备

    IAMVideoCompression *   pVideoCompression_;     // Video 压缩格式

    IAMStreamConfig *       pVideoStreamConfig_;    // for video capture
    IAMStreamConfig *       pAudioStreamConfig_;    // for audio capture

    IAMDroppedFrames *      pDroppedFrames_;        // Drop info

    ISampleGrabber *        pVideoGrabber_;         // 视频抓取回调
    ISampleGrabber *        pAudioGrabber_;         // 音频抓取回调


    bool    previewGraphBuilt_;
    bool    captureGraphBuilt_;

    bool    wantPreview_;
    bool    wantCapture_;
    bool    isPreviewing_;
    bool    isCapturing_;

    long    nDroppedBase_;
    long    nNotDroppedBase_;
};
