#pragma once

#include <DShow.h>
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

enum AVPixelFormat;
enum AVSampleFormat;

class CCameraCapture
{
public:
    CCameraCapture(HWND hwndPreview = NULL);
    ~CCameraCapture(void);

    std::vector<std::string> videoDeviceList_;
    std::vector<std::string> audioDeviceList_;

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

    HRESULT CreateInterfaces();
    HRESULT StopAndReleaseInterfaces();

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

    int ffmpeg_test();

    int ListVideoConfigures();
    int ListAudioConfigures();

    // 枚举视频采集设备, 返回设备数量
    int ListVideoDevices();

    // 枚举音频采集设备
    int ListAudioDevices();

    // 枚举视频压缩格式
    int ListVideoCompressFormat();

    // 枚举音频压缩格式
    int ListAudioCompressFormat();

    // 根据选择的设备创建 Video Capture Filter
    bool CreateVideoFilter(const char * videoDevice = NULL);
    // 根据选择的设备创建 Audio Capture Filter
    bool CreateAudioFilter(const char * audioDevice = NULL);

    // 渲染预览窗口
    bool Render(int mode, TCHAR * videoPath = NULL,
                const char * videoDevice = NULL,
                const char * audioDevice = NULL);

    // 停止当前操作
    bool StopCurrentOperating(int _stop_type = 0);

    // 暂停播放本地视频
    bool PausePlayingLocalVideo();

    // 继续播放本地视频
    bool ContinuePlayingLocalVideo();

protected:
    void InitInterfaces();
    void ReleaseInterfaces();

    int avcodec_encode_video_frame(AVFormatContext * outputFormatCtx,
                                   AVCodecContext * videoCodecCtx,
                                   AVFrame * srcFrame, AVPixelFormat src_format,
                                   AVPixelFormat dest_format);
    int avcodec_encode_audio_frame(AVFormatContext * outputFormatCtx,
                                   AVCodecContext * audioCodecCtx,
                                   AVFrame * srcFrame, AVSampleFormat src_format,
                                   AVSampleFormat dest_format);

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
};
