#include "stdafx.h"
#include "CameraCapture.h"

#include <stdio.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>

#include <atlconv.h>

#include "utils.h"
#include "Mtype.h"

#include "fAllocator.h"

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

bool av_q2d_eq(AVRational a, AVRational b)
{
    return ((a.num == b.num) && (a.den == b.den));
}

//
// From: https://blog.csdn.net/ett_qin/article/details/86691479
// See: https://www.cnblogs.com/linuxAndMcu/p/12068978.html
//

CCameraCapture::CCameraCapture(HWND hwndPreview /* = NULL */)
{
    hwndPreview_ = hwndPreview;

    InitInterfaces();
}

CCameraCapture::~CCameraCapture(void)
{
    StopAndReleaseInterfaces();
}

void CCameraCapture::InitInterfaces()
{
    playState_ = PLAY_STATE::Unknown;

    pFilterGraph_ = NULL;
    pCaptureBuilder_ = NULL;
    pVideoFilter_ = NULL;
    pAudioFilter_ = NULL;
    pVideoMux_ = NULL;
    pVideoWindow_ = NULL;
    pVideoMediaControl_ = NULL;
    pVideoMediaEvent_ = NULL;
}

HRESULT CCameraCapture::CreateInterfaces()
{
    HRESULT hr;

    // 创建 filter graph manager
    SAFE_COM_RELEASE(pFilterGraph_);
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pFilterGraph_);
	if (FAILED(hr))
		return hr;

    // 创建 capture graph manager
    SAFE_COM_RELEASE(pCaptureBuilder_);
    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
                          IID_ICaptureGraphBuilder2, (void**)&pCaptureBuilder_);
	if (FAILED(hr))
		return hr;

    // 创建视频播放窗口
    SAFE_COM_RELEASE(pVideoWindow_);
    hr = pFilterGraph_->QueryInterface(IID_IVideoWindow, (void **)&pVideoWindow_);
	if (FAILED(hr))
		return hr;

    playState_ = PLAY_STATE::Unknown;

    // 创建摄像头流媒体的控制开关
    SAFE_COM_RELEASE(pVideoMediaControl_);
    hr = pFilterGraph_->QueryInterface(IID_IMediaControl, (void **)&pVideoMediaControl_);
	if (FAILED(hr))
		return hr;

    // 创建摄像头流媒体的控制事件
    SAFE_COM_RELEASE(pVideoMediaEvent_);
    hr = pFilterGraph_->QueryInterface(IID_IMediaEventEx, (void **)&pVideoMediaEvent_);
	if (FAILED(hr))
		return hr;

    // 为 capture graph 指定要使用的 filter graph
    if (pCaptureBuilder_ != NULL && pFilterGraph_ != NULL) {
        hr = pCaptureBuilder_->SetFiltergraph(pFilterGraph_);
	    if (FAILED(hr))
		    return hr;
    }
    return hr;
}

void CCameraCapture::ReleaseInterfaces()
{
    SAFE_COM_RELEASE(pVideoMux_);
    SAFE_COM_RELEASE(pVideoWindow_);
    SAFE_COM_RELEASE(pVideoMediaControl_);
    SAFE_COM_RELEASE(pVideoMediaEvent_);
    SAFE_COM_RELEASE(pVideoFilter_);
    SAFE_COM_RELEASE(pAudioFilter_);
    SAFE_COM_RELEASE(pCaptureBuilder_);
    SAFE_COM_RELEASE(pFilterGraph_);
}

HRESULT CCameraCapture::StopAndReleaseInterfaces()
{
    HRESULT hr = S_OK;

    // Stop previewing data
    if (pVideoMediaControl_ != NULL) {
        hr = pVideoMediaControl_->StopWhenReady();
    }

    playState_ = PLAY_STATE::Stopped;

    // Stop receiving events
    if (pVideoMediaEvent_ != NULL) {
        hr = pVideoMediaEvent_->SetNotifyWindow(NULL, WM_GRAPH_NOTIFY, 0);
    }

    // Relinquish ownership (IMPORTANT!) of the video window.
    // Failing to call put_Owner can lead to assert failures within
    // the video renderer, as it still assumes that it has a valid
    // parent window.
    if (pVideoWindow_ != NULL) {
        pVideoWindow_->put_Visible(OAFALSE);
        pVideoWindow_->put_Owner(NULL);
    }

    hwndPreview_ = NULL;

    ReleaseInterfaces();
    return hr;
}

HWND CCameraCapture::SetPreviewHwnd(HWND hwndPreview, bool bAttachTo /* = false */)
{
    HWND oldHwnd = hwndPreview_;
    hwndPreview_ = hwndPreview;
    if (bAttachTo) {
        AttachToVideoWindow(hwndPreview);
    }
    return oldHwnd;
}

bool CCameraCapture::AttachToVideoWindow(HWND hwndPreview)
{
    // 检查视频播放窗口
    if (pVideoWindow_ == NULL)
        return false;

    if (hwndPreview != NULL && ::IsWindow(hwndPreview)) {
        HRESULT hr;

        // Set the video window to be a child of the main window
        // 视频流放入 "Picture控件" 中来预览视频
        hr = pVideoWindow_->put_Owner((OAHWND)hwndPreview);
        if (FAILED(hr))
            return false;

        // Set video window style
        hr = pVideoWindow_->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN);
        if (FAILED(hr))
            return false;

        hwndPreview_ = hwndPreview;

        // Use helper function to position video window in client rect
        // of main application window
        ResizeVideoWindow(hwndPreview);

        // Make the video window visible, now that it is properly positioned
        hr = pVideoWindow_->put_Visible(OATRUE);
        if (FAILED(hr))
            return false;

        // Start receiving events
        if (pVideoMediaEvent_ != NULL) {
            hr = pVideoMediaEvent_->SetNotifyWindow((OAHWND)hwndPreview, WM_GRAPH_NOTIFY, 0);
            if (FAILED(hr))
                return false;
        }
    }

    return true;
}

void CCameraCapture::ResizeVideoWindow(HWND hwndPreview /* = NULL */)
{
    if (hwndPreview == NULL)
        hwndPreview = hwndPreview_;

    if (pVideoWindow_ != NULL) {
        if (hwndPreview != NULL && ::IsWindow(hwndPreview)) {
            CRect rc;
            ::GetClientRect(hwndPreview, &rc);
            // 让图像充满整个窗口
            pVideoWindow_->SetWindowPosition(0, 0, rc.right, rc.bottom);
        }
    }
}

int CCameraCapture::ListVideoConfigures()
{
    int nConfigCount = 0;
	if (pCaptureBuilder_ != NULL && pVideoFilter_ != NULL) {
		videoConfigures_.clear();
		IAMStreamConfig * pStreamConfig = NULL;
		// &MEDIATYPE_Video, 如果包括其他媒体类型, 第二个参数设置为 NULL, 表示 Any media type.
		HRESULT hr = pCaptureBuilder_->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
										             pVideoFilter_, IID_IAMStreamConfig, (void **)&pStreamConfig);
        if (FAILED(hr)) {
            return -1;
        }

		int nCount = 0, nSize = 0;
		hr = pStreamConfig->GetNumberOfCapabilities(&nCount, &nSize);
        if (FAILED(hr)) {
            SAFE_COM_RELEASE(pStreamConfig);
            return -1;
        }

		// Check the size to make sure we pass in the correct structure.
		if (nSize == sizeof(VIDEO_STREAM_CONFIG_CAPS)) {
            // Use the video capabilities structure.
            for (int nIndex = 0; nIndex < nCount; nIndex++) {
                VIDEO_STREAM_CONFIG_CAPS vscc;
                AM_MEDIA_TYPE * pAmtConfig = NULL;
                hr = pStreamConfig->GetStreamCaps(nIndex, &pAmtConfig, (BYTE *)&vscc);
                if (SUCCEEDED(hr)) {
                    /* Examine the format, and possibly use it. */
                    CamVideoConfig config;
                    config.InputSize = vscc.InputSize;
                    config.MinOutputSize = vscc.MinOutputSize;
                    config.MaxOutputSize = vscc.MaxOutputSize;
                    config.MinFPS = (double)kFrameIntervalPerSecond / vscc.MinFrameInterval;
                    config.MaxFPS = (double)kFrameIntervalPerSecond / vscc.MaxFrameInterval;
                    config.lSampleSize = pAmtConfig->lSampleSize;
                    config.cbFormat = pAmtConfig->cbFormat;
                    config.FormatIndex = nIndex;

                    videoConfigures_.push_back(config);
                    nConfigCount++;

                    // Delete the media type when you are done.
                    _DeleteMediaType(pAmtConfig);
                }
            }
        }
    }
    return nConfigCount;
}

int CCameraCapture::ListAudioConfigures()
{
    int nConfigCount = 0;
	if (pCaptureBuilder_ != NULL && pAudioFilter_ != NULL) {
		audioConfigures_.clear();
		IAMStreamConfig * pStreamConfig = NULL;
		// &MEDIATYPE_Video, 如果包括其他媒体类型, 第二个参数设置为 NULL, 表示 Any media type.
		HRESULT hr = pCaptureBuilder_->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio,
										             pAudioFilter_, IID_IAMStreamConfig, (void **)&pStreamConfig);
        if (FAILED(hr)) {
            return -1;
        }

		int nCount = 0, nSize = 0;
		hr = pStreamConfig->GetNumberOfCapabilities(&nCount, &nSize);
        if (FAILED(hr)) {
            SAFE_COM_RELEASE(pStreamConfig);
            return -1;
        }

		// Check the size to make sure we pass in the correct structure.
		if (nSize == sizeof(AUDIO_STREAM_CONFIG_CAPS)) {
            // Use the audio capabilities structure.
            for (int nIndex = 0; nIndex < nCount; nIndex++) {
                AUDIO_STREAM_CONFIG_CAPS ascc;
                AM_MEDIA_TYPE * pAmtConfig = NULL;
                hr = pStreamConfig->GetStreamCaps(nIndex, &pAmtConfig, (BYTE *)&ascc);
                if (SUCCEEDED(hr)) {
                    /* Examine the format, and possibly use it. */
                    CamAudioConfig config;
                    config.Channels = ascc.MaximumChannels;
                    config.BitsPerSample = ascc.MaximumBitsPerSample;
                    config.SampleFrequency = ascc.MaximumSampleFrequency;
                    config.lSampleSize = pAmtConfig->lSampleSize;
                    config.cbFormat = pAmtConfig->cbFormat;
                    config.FormatIndex = nIndex;

                    audioConfigures_.push_back(config);
                    nConfigCount++;

                    // Delete the media type when you are done.
                    _DeleteMediaType(pAmtConfig);
                }
            }
        }
    }
    return nConfigCount;
}

// 枚举视频采集设备
int CCameraCapture::ListVideoDevices()
{
    HRESULT hr;

    // 创建系统设备枚举
    ICreateDevEnum * pCreateDevEnum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (FAILED(hr) || pCreateDevEnum == NULL) {
        return hr;
    }

    // 创建一个指定视频采集设备的枚举
    IEnumMoniker * pEnumMoniker = NULL;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);
    if (FAILED(hr) || pEnumMoniker == NULL) {
        SAFE_COM_RELEASE(pCreateDevEnum);
        return hr;
    }

    ULONG cFetched = 0;
    IMoniker * pMoniker = NULL;
    int video_dev_total = 0;
    int video_dev_count = 0;
    videoDeviceList_.clear();

    // 枚举设备
    while (1) {
        hr = pEnumMoniker->Next(1, &pMoniker, &cFetched);
        if (hr == E_FAIL)
            break;
        else if (hr == S_FALSE) {
            CString text;
            text.Format(_T("Unable to access video capture device! index = %d\n"), video_dev_total);
            OutputDebugString(text.GetBuffer());
            break;
        }
        else if (hr != S_OK) {
            break;
        }
        // 设备属性信息
        IPropertyBag * pPropBag = NULL;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if (SUCCEEDED(hr)) {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pPropBag->Read(L"Description", &var, NULL);
            if (hr != NOERROR) {
                hr = pPropBag->Read(L"FriendlyName", &var, NULL);
            }
            if (hr == NOERROR) {
                char deviceName[256] = { '\0' };
                // 获取设备名称
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, deviceName, sizeof(deviceName), "", NULL);
                SysFreeString(var.bstrVal);
                // 尝试用当前设备绑定到 video filter
                IBaseFilter * pVideoFilter = NULL;
                hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pVideoFilter);
                if (SUCCEEDED(hr) && pVideoFilter != NULL) {
                    // 绑定成功则添加到设备列表
                    videoDeviceList_.push_back(deviceName);
                    video_dev_count++;
                }
                else {
                    CString text;
                    text.Format(_T("Video BindToObject Failed. index = %d\n"), video_dev_total);
                    OutputDebugString(text.GetBuffer());
                }
                if (pVideoFilter != NULL) {
                    pVideoFilter->Release();
                }
            }
            pPropBag->Release();
        }
        pMoniker->Release();
        pMoniker = NULL;
        video_dev_total++;
    }

    pEnumMoniker->Release();
    pCreateDevEnum->Release();

    return video_dev_count;
}

// 枚举音频采集设备
int CCameraCapture::ListAudioDevices()
{
    HRESULT hr;

    // 创建系统设备枚举
    ICreateDevEnum * pCreateDevEnum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (FAILED(hr) || pCreateDevEnum == NULL) {
        return hr;
    }

    // 创建一个指定音频采集设备的枚举
    IEnumMoniker * pEnumMoniker = NULL;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pEnumMoniker, 0);
    if (FAILED(hr) || pEnumMoniker == NULL) {
        SAFE_COM_RELEASE(pCreateDevEnum);
        return hr;
    }

    ULONG cFetched = 0;
    IMoniker * pMoniker = NULL;
    int audio_dev_total = 0;
    int audio_dev_count = 0;
    audioDeviceList_.clear();

    // 枚举设备
    while (1) {
        hr = pEnumMoniker->Next(1, &pMoniker, &cFetched);
        if (hr == E_FAIL)
            break;
        else if (hr == S_FALSE) {
            CString text;
            text.Format(_T("Unable to access audio capture device! index = %d\n"), audio_dev_total);
            OutputDebugString(text.GetBuffer());
            break;
        }
        else if (hr != S_OK) {
            break;
        }
        // 设备属性信息
        IPropertyBag * pPropBag = NULL;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if (SUCCEEDED(hr)) {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pPropBag->Read(L"Description", &var, NULL);
            if (hr != NOERROR) {
                hr = pPropBag->Read(L"FriendlyName", &var, NULL);
            }
            if (hr == NOERROR) {
                char deviceName[256] = { '\0' };
                // 获取设备名称
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, deviceName, sizeof(deviceName), "", NULL);
                SysFreeString(var.bstrVal);
                // 尝试用当前设备绑定到 audio filter
                IBaseFilter * pAudioFilter = NULL;
                hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pAudioFilter);
                if (SUCCEEDED(hr) && pAudioFilter != NULL) {
                    // 绑定成功则添加到设备列表
                    audioDeviceList_.push_back(deviceName);
                    audio_dev_count++;
                }
                else {
                    CString text;
                    text.Format(_T("Audio BindToObject Failed. index = %d\n"), audio_dev_total);
                    OutputDebugString(text.GetBuffer());
                }
                if (pAudioFilter != NULL) {
                    pAudioFilter->Release();
                }
            }
            pPropBag->Release();
        }
        pMoniker->Release();
        pMoniker = NULL;
        audio_dev_total++;
    }

    pEnumMoniker->Release();
    pCreateDevEnum->Release();

    return audio_dev_count;
}

// 枚举视频压缩格式
int CCameraCapture::ListVideoCompressFormat()
{
    return 0;
}

// 枚举音频压缩格式
int CCameraCapture::ListAudioCompressFormat()
{
    return 0;
}

// 根据选择的设备获取 Video Capture Filter
bool CCameraCapture::CreateVideoFilter(const char * videoDevice)
{
    if (videoDevice == NULL)
        return false;

    // 创建系统设备枚举
    ICreateDevEnum * pCreateDevEnum = NULL;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (FAILED(hr) || pCreateDevEnum == NULL) {
        return false;
    }

    // 创建一个指定视频采集设备的枚举
    IEnumMoniker * pEnumMoniker = NULL;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);
    if (FAILED(hr) || pEnumMoniker == NULL) {
        SAFE_COM_RELEASE(pCreateDevEnum);
        return false;
    }

    bool result = false;
    ULONG cFetched = 0;
    int nIndex = 0;
    IMoniker * pMoniker = NULL;

    // 枚举设备
    while (1) {
        hr = pEnumMoniker->Next(1, &pMoniker, &cFetched);
        if (hr == E_FAIL)
            break;
        else if (hr == S_FALSE) {
            CString text;
            text.Format(_T("Unable to access video capture device! index = %d\n"), nIndex);
            OutputDebugString(text.GetBuffer());
            break;
        }
        else if (hr != S_OK) {
            break;
        }
        // 设备属性信息
        IPropertyBag * pPropBag = NULL;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if (SUCCEEDED(hr)) {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pPropBag->Read(L"Description", &var, NULL);
            if (hr != NOERROR) {
                hr = pPropBag->Read(L"FriendlyName", &var, NULL);
            }
            if (hr == NOERROR) {
                char deviceName[256] = { '\0' };
                // 获取设备名称
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, deviceName, sizeof(deviceName), "", NULL);
                SysFreeString(var.bstrVal);
                if (videoDevice != NULL && strcmp(videoDevice, deviceName) == 0) {
                    SAFE_COM_RELEASE(pVideoFilter_);
                    // 尝试用当前设备绑定到 video filter
                    hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pVideoFilter_);
                    if (SUCCEEDED(hr) && pVideoFilter_ != NULL) {
                        result = true;
                    }
                    else {
                        CString text;
                        text.Format(_T("Video BindToObject Failed. index = %d\n"), nIndex);
                        OutputDebugString(text.GetBuffer());
                    }
                }
            }
            pPropBag->Release();
        }
        pMoniker->Release();
        pMoniker = NULL;
        nIndex++;
    }

    pEnumMoniker->Release();
    pCreateDevEnum->Release();
    return result;
}

// 根据选择的设备获取 Audio Capture Filter
bool CCameraCapture::CreateAudioFilter(const char * audioDevice)
{
    if (audioDevice == NULL)
        return false;

    // 创建系统设备枚举
    ICreateDevEnum * pCreateDevEnum = NULL;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (FAILED(hr) || pCreateDevEnum == NULL) {
        return false;
    }

    // 创建一个指定音频采集设备的枚举
    IEnumMoniker * pEnumMoniker = NULL;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pEnumMoniker, 0);
    if (FAILED(hr) || pEnumMoniker == NULL) {
        SAFE_COM_RELEASE(pCreateDevEnum);
        return false;
    }

    bool result = false;
    ULONG cFetched = 0;
    int nIndex = 0;
    IMoniker * pMoniker = NULL;

    // 枚举设备
    while (1) {
        hr = pEnumMoniker->Next(1, &pMoniker, &cFetched);
        if (hr == E_FAIL)
            break;
        else if (hr == S_FALSE) {
            CString text;
            text.Format(_T("Unable to access audio capture device! index = %d\n"), nIndex);
            OutputDebugString(text.GetBuffer());
            break;
        }
        else if (hr != S_OK) {
            break;
        }
        // 设备属性信息
        IPropertyBag * pPropBag = NULL;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if (SUCCEEDED(hr)) {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pPropBag->Read(L"Description", &var, NULL);
            if (hr != NOERROR) {
                hr = pPropBag->Read(L"FriendlyName", &var, NULL);
            }
            if (hr == NOERROR) {
                char deviceName[256] = { '\0' };
                // 获取设备名称
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, deviceName, sizeof(deviceName), "", NULL);
                SysFreeString(var.bstrVal);
                if (audioDevice != NULL && strcmp(audioDevice, deviceName) == 0) {
                    SAFE_COM_RELEASE(pAudioFilter_);
                    // 尝试用当前设备绑定到 audio filter
                    hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pAudioFilter_);
                    if (SUCCEEDED(hr) && pAudioFilter_ != NULL) {
                        result = true;
                    }
                    else {
                        CString text;
                        text.Format(_T("Audio BindToObject Failed. index = %d\n"), nIndex);
                        OutputDebugString(text.GetBuffer());
                    }
                }
            }
            pPropBag->Release();
        }
        pMoniker->Release();
        pMoniker = NULL;
        nIndex++;
    }

    pEnumMoniker->Release();
    pCreateDevEnum->Release();
    return result;
}

// 渲染摄像头预览视频
bool CCameraCapture::Render(int mode, TCHAR * videoPath,
                            const char * videoDevice,
                            const char * audioDevice)
{
    HRESULT hr;

    // 检查 Video capture Builder
    if (pCaptureBuilder_ == NULL)
        return false;

    // 检查 Video filter graph (管理器)
    if (pFilterGraph_ == NULL)
        return false;

    if (mode != MODE_LOCAL_VIDEO) {
        // 创建 Video filter
        bool result = CreateVideoFilter(videoDevice);
        if (!result)
            return false;

        // 将 Video filter 加入 filter graph
        hr = pFilterGraph_->AddFilter(pVideoFilter_, L"Video Filter");
        if (hr != NOERROR)
            return false;

        if (mode == MODE_RECORD_VIDEO) {
            // 创建 Audio filter
            result = CreateAudioFilter(audioDevice);
            if (result) {
                // 将 Audio filter 加入 filter graph
                hr = pFilterGraph_->AddFilter(pAudioFilter_, L"Audio Filter");
                if (hr != NOERROR)
                    return false;
            }
        }
    }

    if (mode == MODE_PREVIEW_VIDEO) {     // 预览视频
        hr = pCaptureBuilder_->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pVideoFilter_, NULL, NULL);
        if (hr == VFW_S_NOPREVIEWPIN) {
            // preview was faked up for us using the (only) capture pin
        }
        else if (hr != S_OK) {
            OutputDebugString(_T("This graph cannot preview!\n"));
            return false;
        }
    }
    else if (mode == MODE_RECORD_VIDEO) { // 录制视频
        USES_CONVERSION;
        LPCOLESTR OLE_PathName = T2OLE(videoPath); // 类型转化

        // 设置输出文件
        SAFE_COM_RELEASE(pVideoMux_);
        hr = pCaptureBuilder_->SetOutputFileName(&MEDIASUBTYPE_Avi, OLE_PathName, &pVideoMux_, NULL);
        if (hr != NOERROR)
            return false;

        // 录制的时候也需要预览
        hr = pCaptureBuilder_->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pVideoFilter_, NULL, NULL);
        if (hr == VFW_S_NOPREVIEWPIN) {
            // preview was faked up for us using the (only) capture pin
        }
        else if (hr != S_OK) {
            OutputDebugString(_T("This graph cannot preview!\n"));
            return false;
        }

        // 视频流写入 avi 文件
        if (pVideoMux_ != NULL) {
            hr = pCaptureBuilder_->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVideoFilter_, NULL, pVideoMux_);
            if (hr != NOERROR)
                return false;

            if (pAudioFilter_ != NULL) {
                hr = pCaptureBuilder_->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, pAudioFilter_, NULL, pVideoMux_);
                if (hr != NOERROR)
                    return false;
            }
        }
    }
    else if (mode == MODE_LOCAL_VIDEO) {   // 播放本地视频
        USES_CONVERSION;
        LPCOLESTR OLE_PathName = T2OLE(videoPath); // 类型转化
        hr = pFilterGraph_->RenderFile(OLE_PathName, NULL);
        if (hr != NOERROR) {
            return false;
        }
    }

    /******* 设置视频播放窗口 *******/
    if (!AttachToVideoWindow(hwndPreview_))
        return false;

    // 检查摄像头流媒体的控制开关
    if (pVideoMediaControl_ == NULL)
        return false;

    // 开始预览或录制
    hr = pVideoMediaControl_->Run();
    if (FAILED(hr))
        return false;

    return true;
}

HRESULT CCameraCapture::HandleGraphEvent(void)
{
    LONG evCode = -1;
	LONG_PTR evParam1 = NULL, evParam2 = NULL;
    HRESULT hr = S_OK;

    if (pVideoMediaEvent_ == NULL)
        return E_POINTER;

    while (SUCCEEDED(pVideoMediaEvent_->GetEvent(&evCode, &evParam1, &evParam2, 0))) {
        //
        // Insert event processing code here, if desired
        //

        //
        // Free event parameters to prevent memory leaks associated with
        // event parameter data.  While this application is not interested
        // in the received events, applications should always process them.
        //
        hr = pVideoMediaEvent_->FreeEventParams(evCode, evParam1, evParam2);
    }

    return hr;
}

HRESULT CCameraCapture::WindowStateChange(BOOL isShow)
{
    HRESULT hr;
    if (isShow)
        hr = ChangePreviewState(PLAY_STATE::Running);
    else
        hr = ChangePreviewState(PLAY_STATE::Stopped);
    return hr;
}

HRESULT CCameraCapture::ChangePreviewState(PLAY_STATE playState /* = PLAY_STATE::Running */)
{
    HRESULT hr = E_FAIL;

    // If the media control interface isn't ready, don't call it
    if (!pVideoMediaControl_)
        return E_FAIL;

    if (playState == PLAY_STATE::Running) {
        if (playState_ != PLAY_STATE::Running) {
            // Start previewing video data
            hr = pVideoMediaControl_->Run();
            playState_ = playState;
        }
    }
    else if (playState == PLAY_STATE::Paused) {
        // Pause previewing video data
        if (playState_ != PLAY_STATE::Paused) {
            hr = pVideoMediaControl_->Pause();
            playState_ = playState;
        }
    }
    else {
        // Stop previewing video data
        hr = pVideoMediaControl_->StopWhenReady();
        //hr = pVideoMediaControl_->Stop();
        playState_ = playState;
    }

    return hr;
}

// 关闭摄像头
bool CCameraCapture::StopCurrentOperating(int action_type)
{
    if (pVideoMediaControl_->Stop() < 0)
        return false;

    if (action_type != ACTION_STOP_LOCAL_VIDEO) {
        pVideoFilter_->Release();
        pCaptureBuilder_->Release();
    }

    pVideoWindow_->Release();
    pVideoMediaControl_->Release();
    pFilterGraph_->Release();
    return true;
}

// 暂停播放本地视频
bool CCameraCapture::PausePlayingLocalVideo()
{
    if (pVideoMediaControl_->Stop() < 0)
        return false;
    else
        return true;
}

// 继续播放本地视频
bool CCameraCapture::ContinuePlayingLocalVideo()
{
    if (pVideoMediaControl_->Run() < 0)
        return false;
    else
        return true;
}

int transform_video_format(AVFrame * src_frame, AVPixelFormat src_format,
                           AVFrame * dest_frame, AVPixelFormat dest_format)
{
    if (src_frame->format != src_format) {
        debug_print("Input frame is not in YUV422P format\n");
        return -1;
    }

    if (dest_frame->format != dest_format) {
        debug_print("Output frame is not in YUV420P format\n");
        return -1;
    }

    // 检查输入和输出帧的宽度和高度是否一致
    if (src_frame->width != dest_frame->width || src_frame->height != dest_frame->height) {
        debug_print("Input and output frame dimensions do not match\n");
        return -1;
    }

    // 创建 SwsContext 用于格式转换
    SwsContext * sws_ctx = sws_getContext(
        src_frame->width, src_frame->height, src_format,
        dest_frame->width, dest_frame->height, dest_format,
        SWS_BILINEAR, NULL, NULL, NULL);

    if (!sws_ctx) {
        debug_print("Could not initialize the conversion context\n");
        return -1;
    }

    // 执行格式转换
    int ret = sws_scale(sws_ctx,
                        (const uint8_t * const *)src_frame->data, src_frame->linesize,
                        0, src_frame->height,
                        dest_frame->data, dest_frame->linesize);

    // 释放 SwsContext
    sws_freeContext(sws_ctx);
    return ret;
}

int CCameraCapture::avcodec_encode_video_frame(AVFormatContext * outputFormatCtx,
                                               AVCodecContext * videoCodecCtx,
                                               AVStream * inputVideoStream,
                                               AVStream * outputVideoStream,
                                               AVFrame * srcFrame, int frameIndex,
                                               AVPixelFormat src_format,
                                               AVPixelFormat dest_format)
{
    int ret;
    // 创建一个 YUV420P 格式的 AVFrame
    fSmartPtr<AVFrame> destFrame = av_frame_alloc();
    if (destFrame == NULL) {
        debug_print("destFrame 创建失败\n");
        return -1;
    }

    ret = av_frame_copy_props(destFrame, srcFrame);

    destFrame->width  = srcFrame->width;
    destFrame->height = srcFrame->height;
    destFrame->pkt_duration = srcFrame->pkt_duration;
    destFrame->pkt_size = srcFrame->pkt_size;
    destFrame->format = (int)dest_format;
    destFrame->pict_type = AV_PICTURE_TYPE_NONE;

    // 为 YUV420P 格式的 AVFrame 分配内存
    ret = av_frame_get_buffer(destFrame, 32);
    if (ret < 0) {
        debug_print("Could not allocate destination frame data\n");
        return -1;
    }

    ret = av_frame_make_writable(destFrame);

    // 调用转换函数
    ret = transform_video_format(srcFrame, src_format, destFrame, dest_format);
    if (ret < 0) {
        debug_print("Failed to convert YUV422P to YUV420P\n");
        return -1;
    }

    // 输出视频编码
    ret = avcodec_send_frame(videoCodecCtx, destFrame);
    if (ret < 0) {
        debug_print("发送视频数据包到编码器时出错\n");
        return ret;
    }
    while (1) {
        fSmartPtr<AVPacket> videoPacket = av_packet_alloc();
        ret = avcodec_receive_packet(videoCodecCtx, videoPacket);
        if (ret == 0) {
            // See: https://www.cnblogs.com/leisure_chn/p/10584901.html
            // See: https://www.cnblogs.com/leisure_chn/p/10584925.html
            videoPacket->stream_index = outputVideoStream->index;
            if (videoPacket->pts == AV_NOPTS_VALUE) {
                // Write PTS
                AVRational time_base = inputVideoStream->time_base;
                // Duration between 2 frames (us)
                double duration = (double)AV_TIME_BASE / av_q2d(inputVideoStream->r_frame_rate);
                // Parameters
                videoPacket->pts = (int64_t)((double)(frameIndex * duration) / (double)(av_q2d(time_base) * AV_TIME_BASE));
                videoPacket->dts = videoPacket->pts;
                videoPacket->duration = (int64_t)((double)duration / (double)(av_q2d(time_base) * AV_TIME_BASE));
            }
            else {
                //if (videoPacket->duration == 0)
                //    videoPacket->duration = 1;
                av_packet_rescale_ts(videoPacket, videoCodecCtx->time_base, outputVideoStream->time_base);
                debug_print("videoPacket->duration = %d\n", videoPacket->duration);
            }
            //ret = av_interleaved_write_frame(outputFormatCtx, videoPacket);
            ret = av_write_frame(outputFormatCtx, videoPacket);
            if (ret < 0) {
                debug_print("保存编码器视频帧时出错\n");
                ret = 0;
            }
            break;
        }
        else if (ret == AVERROR(EAGAIN)) {
            break;
        }
        else if (ret == AVERROR_EOF) {
            debug_print("EOF\n");
            break;
        }
        else if (ret < 0) {
            debug_print("从编码器接收视频帧时出错\n");
            break;
        }
    }
    return ret;
}

int transform_audio_format(AVCodecContext * audioCodecCtx,
                           AVFrame * src_frame, AVSampleFormat src_format,
                           AVFrame * dest_frame, AVSampleFormat dest_format)
{
    int ret = 0;
#if 1
    SwrContext * swr_ctx = swr_alloc();
    if (swr_ctx == NULL) {
        debug_print("Could not allocate SwrContext\n");
        return -1;
    }

    // 配置 SwrContext
    ret = av_opt_set_int(swr_ctx, "in_channel_count",   src_frame->channels, 0);
    ret = av_opt_set_int(swr_ctx, "out_channel_count",  dest_frame->channels, 0);
    ret = av_opt_set_int(swr_ctx, "in_channel_layout",  AV_CH_LAYOUT_STEREO, 0);
    ret = av_opt_set_int(swr_ctx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    ret = av_opt_set_int(swr_ctx, "in_sample_rate",  44100, 0);
    ret = av_opt_set_int(swr_ctx, "out_sample_rate", 48000, 0);
    ret = av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",  AV_SAMPLE_FMT_S16,  0);
    ret = av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);

    ret = swr_init(swr_ctx);
    if (ret < 0) {
        debug_print("Could not initialize SwrContext\n");
        swr_free(&swr_ctx);
        return -1;
    }

    // 执行格式转换
    ret = swr_convert_frame(swr_ctx, dest_frame, src_frame);
    if (ret < 0) {
        debug_print("Error converting PCM_S16LE to FLTP\n");
        swr_free(&swr_ctx);
        return -1;
    }

    swr_free(&swr_ctx);
#else
    // 初始化 SwrContext 用于样本格式转换
    SwrContext * swr_ctx = swr_alloc_set_opts(
        NULL,
        audioCodecCtx->channel_layout,
        dest_format,
        audioCodecCtx->sample_rate,
        src_frame->channel_layout,
        src_format,
        src_frame->sample_rate,
        0, NULL);
    if (swr_ctx == NULL)
        return -1;

    // 转换 PCM 数据到 AAC 帧
    ret = swr_convert(swr_ctx, dest_frame->data,
                      dest_frame->nb_samples,
                      (const uint8_t **)src_frame->data,
                      src_frame->nb_samples);

    // 释放 SwrContext
    swr_free(&swr_ctx);
#endif
    return ret;
}

int CCameraCapture::avcodec_encode_audio_frame(AVFormatContext * outputFormatCtx,
                                               AVCodecContext * audioCodecCtx,
                                               AVStream * inputAudioStream,
                                               AVStream * outputAudioStream,
                                               AVFrame * srcFrame, int frameIndex,
                                               AVSampleFormat src_format,
                                               AVSampleFormat dest_format)
{
    int ret;
    // 创建一个 AAC 格式的 AVFrame
    fSmartPtr<AVFrame> destFrame = av_frame_alloc();
    if (destFrame == NULL) {
        debug_print("destFrame 创建失败\n");
        return -1;
    }

    ret = av_frame_copy_props(destFrame, srcFrame);

    destFrame->channels  = audioCodecCtx->channels;
    destFrame->sample_rate = audioCodecCtx->sample_rate;
    destFrame->channel_layout = audioCodecCtx->channel_layout;
    destFrame->nb_samples = audioCodecCtx->frame_size;
    destFrame->pkt_duration = srcFrame->pkt_duration;
    //destFrame->pkt_size = audioCodecCtx->frame_size;
    destFrame->format = (int)dest_format;

    // 为 AAC 格式的 AVFrame 分配内存
    ret = av_frame_get_buffer(destFrame, 32);
    if (ret < 0) {
        debug_print("Could not allocate destination frame data\n");
        return -1;
    }

    ret = av_frame_make_writable(destFrame);

    // 调用转换函数
    ret = transform_audio_format(audioCodecCtx, srcFrame, src_format, destFrame, dest_format);
    if (ret < 0) {
        debug_print("Failed to convert PCM_S16LE to AAC\n");
        return -1;
    }

    // 输出视频编码
    ret = avcodec_send_frame(audioCodecCtx, destFrame);
    if (ret < 0) {
        debug_print("发送音频数据包到编码器时出错\n");
        return ret;
    }
    while (1) {
        fSmartPtr<AVPacket> audioPacket = av_packet_alloc();
        ret = avcodec_receive_packet(audioCodecCtx, audioPacket);
        if (ret == 0) {
            // See: https://www.cnblogs.com/leisure_chn/p/10584901.html
            // See: https://www.cnblogs.com/leisure_chn/p/10584925.html
            audioPacket->stream_index = outputAudioStream->index;
            if (audioPacket->pts == AV_NOPTS_VALUE) {
                // Write PTS
                AVRational time_base = inputAudioStream->time_base;
                // Duration between 2 frames (us)
                double duration = (double)AV_TIME_BASE / av_q2d(inputAudioStream->r_frame_rate);
                // Parameters
                audioPacket->pts = (int64_t)((double)(frameIndex * duration) / (double)(av_q2d(time_base) * AV_TIME_BASE));
                audioPacket->dts = audioPacket->pts;
                audioPacket->duration = (int64_t)((double)duration / (double)(av_q2d(time_base) * AV_TIME_BASE));
            }
            else {
                //if (audioPacket->duration == 0)
                //    audioPacket->duration = 1;
                if (av_q2d_eq(audioCodecCtx->time_base, outputAudioStream->time_base)) {
                    av_packet_rescale_ts(audioPacket, audioCodecCtx->time_base, outputAudioStream->time_base);
                }
                debug_print("audioPacket->duration = %d\n", audioPacket->duration);
            }
            //ret = av_interleaved_write_frame(outputFormatCtx, audioPacket);
            ret = av_write_frame(outputFormatCtx, audioPacket);
            if (ret < 0) {
                debug_print("保存编码器音频帧时出错\n");
                ret = 0;
            }
            break;
        }
        else if (ret == AVERROR(EAGAIN)) {
            break;
        }
        else if (ret == AVERROR_EOF) {
            debug_print("EOF\n");
            break;
        }
        else if (ret < 0) {
            debug_print("从编码器接收音频帧时出错\n");
            break;
        }
    }
    return ret;
}

int CCameraCapture::ffmpeg_test()
{
    // 初始化 FFmpeg
    av_register_all();
    avdevice_register_all();
    avformat_network_init();

    av_log_set_level(AV_LOG_INFO);
    set_log_level(LOG_INFO);

    #define OUTPUT_FILENAME "output.mp4"

    int result = 0;

    // 打开视频设备
    AVInputFormat * videoInputFormat = av_find_input_format("dshow");
    if (videoInputFormat == NULL) {
        log_print(LOG_ERROR, "Could not find video input format\n");
        return -1;
    }

#if 0
    // 列出视频设备
    AVDeviceInfoList * videoDeviceList = NULL;
    result = avdevice_list_input_sources(videoInputFormat, NULL, NULL, &videoDeviceList);
    if (result < 0) {
        log_print(LOG_ERROR, "Could not list video devices\n");
        return -1;
    }

    // 选择视频设备
    const char * videoDeviceName = videoDeviceList->devices[0]->device_name;
    log_print(LOG_INFO, "Selected video device: %s\n", videoDeviceName);
#endif

    std::string vdDeviceName = Ansi2Utf8(videoDeviceList_[0]);
    const char * videoDeviceName = vdDeviceName.c_str();

    // 列出音频设备
    AVInputFormat * audioInputFormat = av_find_input_format("dshow");
    if (audioInputFormat == NULL) {
        log_print(LOG_ERROR, "Could not find audio input format\n");
        return -1;
    }

#if 0
    AVDeviceInfoList * audioDeviceList = NULL;
    result = avdevice_list_input_sources(audioInputFormat, NULL, NULL, &audioDeviceList);
    if (result == AVERROR(ENOSYS)) {
        // Not support
        result = -1;
    }
    if (result < 0) {
        log_print(LOG_ERROR, "Could not list audio devices\n");
        return -1;
    }

    // 选择音频设备
    const char * audioDeviceName = audioDeviceList->devices[0]->device_name;
    log_print(LOG_INFO, "Selected audio device: %s\n", audioDeviceName);
#endif

    std::string adDeviceName = Ansi2Utf8(audioDeviceList_[0]);
    const char * audioDeviceName = adDeviceName.c_str();

#if 0
    // 释放设备列表
    avdevice_free_list_devices(&videoDeviceList);
    avdevice_free_list_devices(&audioDeviceList);
#endif

    // 创建 AVFormatContext
    fSmartPtr<AVFormatContext> inputVideoFormatCtx = avformat_alloc_context();

    char video_url[256];
    snprintf(video_url, sizeof(video_url), "video=%s", videoDeviceName);

    result = avformat_open_input(&inputVideoFormatCtx, video_url, videoInputFormat, NULL);
    if (result != 0) {
        debug_print("无法打开输入视频设备\n");
        return -1;
    }

    // 查找视频流
    int videoStreamIndex = -1;
    for (unsigned int i = 0; i < inputVideoFormatCtx->nb_streams; i++) {
        if (inputVideoFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }
    if (videoStreamIndex == -1) {
        debug_print("找不到输入视频流\n");
        return -1;
    }

    // 创建 AVFormatContext
    fSmartPtr<AVFormatContext> inputAudioFormatCtx = avformat_alloc_context();

    char audio_url[256];
    snprintf(audio_url, sizeof(audio_url), "audio=%s", audioDeviceName);

    result = avformat_open_input(&inputAudioFormatCtx, audio_url, audioInputFormat, NULL);
    if (result != 0) {
        debug_print("无法打开输入音频设备\n");
        return -1;
    }

    // 查找音频流
    int audioStreamIndex = -1;
    for (unsigned int i = 0; i < inputAudioFormatCtx->nb_streams; i++) {
        if (inputAudioFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }
    if (audioStreamIndex == -1) {
        debug_print("找不到输入音频流\n");
        return -1;
    }

    // 获取视频和音频的编解码器参数
    AVStream * inputVideoStream = inputVideoFormatCtx->streams[videoStreamIndex];
    AVStream * inputAudioStream = inputAudioFormatCtx->streams[audioStreamIndex];

    AVCodecParameters * videoCodecParams = inputVideoFormatCtx->streams[videoStreamIndex]->codecpar;
    AVCodecParameters * audioCodecParams = inputAudioFormatCtx->streams[audioStreamIndex]->codecpar;

    // 查找视频的编解码器
    AVCodec * inputVideoCodec = avcodec_find_decoder(videoCodecParams->codec_id);
    if (inputVideoCodec == NULL) {
        debug_print("查找输入视频解码器 codec 失败\n");
        return -1;
    }

    fSmartPtr<AVCodecContext> inputVideoCodecCtx = avcodec_alloc_context3(inputVideoCodec);

    AVRational time_base = {333333, 10000000};
    AVRational framerate = {10000000, 333333};

    // AV_CODEC_ID_RAWVIDEO (13): "RawVideo"
    inputVideoCodecCtx->codec_id = videoCodecParams->codec_id;
    inputVideoCodecCtx->bit_rate = 2000000;
    inputVideoCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    inputVideoCodecCtx->width = videoCodecParams->width;
    inputVideoCodecCtx->height = videoCodecParams->height;
    inputVideoCodecCtx->time_base = av_inv_q(inputVideoStream->r_frame_rate);
    inputVideoCodecCtx->framerate = inputVideoStream->r_frame_rate;
    inputVideoCodecCtx->gop_size = 12;
    inputVideoCodecCtx->max_b_frames = 0;
    // AV_PIX_FMT_YUYV422 (1)
    inputVideoCodecCtx->pix_fmt = (AVPixelFormat)videoCodecParams->format;

    //inputVideoStream->time_base = inputVideoCodecCtx->time_base;

    result = avcodec_open2(inputVideoCodecCtx, inputVideoCodec, NULL);
    if (result == AVERROR(EINVAL)) {
        // EINVAL: Invalid argument
    }
    if (result < 0) {
        debug_print("无法打开输入视频编解码器\n");
        return -1;
    }

    // 查找音频的编解码器
    AVCodec * inputAudioCodec = avcodec_find_decoder(audioCodecParams->codec_id);
    if (inputAudioCodec == NULL) {
        debug_print("查找输入音频解码器 codec 失败\n");
        return -1;
    }
    fSmartPtr<AVCodecContext> inputAudioCodecCtx = avcodec_alloc_context3(inputAudioCodec);

    AVRational audio_time_base = { 1, audioCodecParams->sample_rate };

    // AV_CODEC_ID_PCM_S16LE (65536)
    inputAudioCodecCtx->codec_id = audioCodecParams->codec_id;
    inputAudioCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    inputAudioCodecCtx->channels = audioCodecParams->channels;
    inputAudioCodecCtx->sample_rate = audioCodecParams->sample_rate;
    // 麦克风应该是单声道的, 备选值：AV_CH_LAYOUT_STEREO
    //inputAudioCodecCtx->channel_layout = audioCodecParams->channel_layout;
    inputAudioCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    inputAudioCodecCtx->time_base = audio_time_base;
    // AV_SAMPLE_FMT_S16 (1)
    inputAudioCodecCtx->sample_fmt = (AVSampleFormat)audioCodecParams->format;

    //inputAudioStream->time_base = inputAudioCodecCtx->time_base;

    result = avcodec_open2(inputAudioCodecCtx, inputAudioCodec, NULL);
    if (result < 0) {
        debug_print("无法打开输入音频编解码器\n");
        return -1;
    }

    // 创建输出格式上下文
    fSmartPtr<AVFormatContext, 1> outputFormatCtx = NULL;
    result = avformat_alloc_output_context2(&outputFormatCtx, NULL, NULL, OUTPUT_FILENAME);
    if (result < 0) {
        debug_print("无法创建输出格式context\n");
        return -1;
    }

    // 打开视频的编解码器
    AVCodec * outputVideoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (outputVideoCodec == NULL) {
        debug_print("查找输出视频解码器 codec 失败\n");
        return -1;
    }
    fSmartPtr<AVCodecContext> outputVideoCodecCtx = avcodec_alloc_context3(outputVideoCodec);

    outputVideoCodecCtx->bit_rate = 2000000;
    //outputVideoCodecCtx->codec_id = AV_CODEC_ID_H263;
    outputVideoCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    outputVideoCodecCtx->width = videoCodecParams->width;
    outputVideoCodecCtx->height = videoCodecParams->height;
    outputVideoCodecCtx->time_base = time_base;
    outputVideoCodecCtx->framerate = framerate;
    outputVideoCodecCtx->gop_size = 30;
    outputVideoCodecCtx->max_b_frames = 3;
    outputVideoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    //outputVideoCodecCtx->pix_fmt = AV_PIX_FMT_NV12;

    if (outputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        outputVideoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    //
    // See: https://blog.csdn.net/weixin_50873490/article/details/141899535
    //
    if (outputVideoCodecCtx->codec_id == AV_CODEC_ID_H264) {
        result = av_opt_set(outputVideoCodecCtx->priv_data, "profile", "main", 0);
        // 0 latency
        //result = av_opt_set(outputVideoCodecCtx->priv_data, "tune", "zerolatency",0);
    }

    result = avcodec_open2(outputVideoCodecCtx.ptr(), outputVideoCodec, NULL);
    if (result == AVERROR(ENOSYS) || result == AVERROR(EINVAL)) {
        // EINVAL: Invalid argument
        //result = 0;
    }
    if (result < 0) {
        debug_print("无法打开输出视频编解码器\n");
        return -1;
    }

    // 打开音频的编解码器
    AVCodec * outputAudioCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (outputAudioCodec == NULL) {
        debug_print("查找输出音频解码器 codec 失败\n");
        return -1;
    }
    fSmartPtr<AVCodecContext> outputAudioCodecCtx = avcodec_alloc_context3(outputAudioCodec);

    audio_time_base.num = 1;
    audio_time_base.den = 48000;

    outputAudioCodecCtx->bit_rate = 160000;
    outputAudioCodecCtx->codec_id = AV_CODEC_ID_AAC;
    outputAudioCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    outputAudioCodecCtx->channels = 2;
    outputAudioCodecCtx->sample_rate = 48000;
    outputAudioCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    outputAudioCodecCtx->time_base = audio_time_base;
    outputAudioCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;

    if (outputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        outputAudioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    result = avcodec_open2(outputAudioCodecCtx, outputAudioCodec, NULL);
    if (result < 0) {
        debug_print("无法打开输出音频编解码器\n");
        return -1;
    }

    // 添加视频流
    AVStream * outputVideoStream = avformat_new_stream(outputFormatCtx, outputVideoCodec);

    outputVideoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    outputVideoStream->codecpar->codec_id = outputVideoCodec->id;

    // 复制视频编解码器参数
    //avcodec_parameters_copy(outputVideoStream->codecpar, videoCodecParams);
    avcodec_parameters_from_context(outputVideoStream->codecpar, outputVideoCodecCtx);
    // 注：读取视频文件时使用
    //avcodec_parameters_to_context(outputVideoCodecCtx, outputVideoStream->codecpar);

    //outputVideoStream->codec = outputVideoCodecCtx->codec;
    outputVideoStream->id    = outputVideoCodecCtx->codec_id;
    outputVideoStream->codecpar->codec_tag = 0;

    // 添加音频流
    AVStream * outputAudioStream = avformat_new_stream(outputFormatCtx, outputAudioCodec);

    outputAudioStream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    outputAudioStream->codecpar->codec_id = outputAudioCodec->id;

    // 复制音频的编解码器参数
    //avcodec_parameters_copy(outputAudioStream->codecpar, audioCodecParams);
    avcodec_parameters_from_context(outputAudioStream->codecpar, outputAudioCodecCtx);
    // 注：读取音频文件时使用
    //avcodec_parameters_to_context(outputAudioCodecCtx, outputAudioStream->codecpar);

    //outputAudioStream->codec = outputAudioCodecCtx->codec;
    outputAudioStream->id    = outputAudioCodecCtx->codec_id;
    outputAudioStream->codecpar->codec_tag = 0;

    //outputVideoStream->time_base = outputVideoCodecCtx->time_base;
    //outputAudioStream->time_base = outputAudioCodecCtx->time_base;

#if 0
    outputFormatCtx->video_codec = outputVideoCodec;
    outputFormatCtx->video_codec_id = outputVideoCodec->id;

    outputFormatCtx->audio_codec = outputAudioCodec;
    outputFormatCtx->audio_codec_id = outputAudioCodec->id;
#endif

    // 打开输出文件
    if (!(outputFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        result = avio_open(&outputFormatCtx->pb, OUTPUT_FILENAME, AVIO_FLAG_WRITE);
        if (result < 0) {
            debug_print("无法打开输出文件\n");
            return -1;
        }
    }

    // 写入文件头
    result = avformat_write_header(outputFormatCtx, NULL);
    if (result < 0) {
        debug_print("无法写入文件头\n");
        return -1;
    }

    // 读取和编码视频帧

    int ret, ret2;
    int nVideoFrameCnt = 0;
    int nAudioFrameCnt = 0;
    bool isExit = false;
    while (1) {
        // Video
        if (1) {
            fSmartPtr<AVPacket> videoPacket = av_packet_alloc();
            ret = av_read_frame(inputVideoFormatCtx, videoPacket);
            if (ret < 0) {
                isExit = true;
                break;
            }
            // See: https://www.cnblogs.com/leisure_chn/p/10584910.html
            av_packet_rescale_ts(videoPacket, inputVideoStream->time_base,
                                 inputVideoCodecCtx->time_base);
            if (videoPacket->stream_index == videoStreamIndex) {
                // 输入视频解码
                ret = avcodec_send_packet(inputVideoCodecCtx, videoPacket);
                if (ret < 0) {
                    debug_print("发送视频数据包到解码器时出错\n");
                    isExit = true;
                    break;
                }

                while (1) {
                    fSmartPtr<AVFrame> frame = av_frame_alloc();
                    ret = av_frame_is_writable(frame);
                    ret = avcodec_receive_frame(inputVideoCodecCtx, frame);
                    if (ret == 0) {
                        ret = av_frame_make_writable(frame);
                        // 将编码后的视频帧写入输出文件
                        //frame->pts = av_rescale_q(frame->pts, inputVideoCodecCtx->time_base, outputVideoStream->time_base);
                        //frame->pts = nFrameCount;
                        if (frame->pts == AV_NOPTS_VALUE) {
                            frame->pts = frame->best_effort_timestamp;
                            frame->pkt_dts = frame->pts;
                            frame->pkt_duration = av_rescale_q(1, inputVideoStream->time_base, outputVideoStream->time_base);
                        }

                        ret2 = avcodec_encode_video_frame(outputFormatCtx, outputVideoCodecCtx,
                                                          inputVideoStream, outputVideoStream,
                                                          frame, nVideoFrameCnt,
                                                          inputVideoCodecCtx->pix_fmt,
                                                          outputVideoCodecCtx->pix_fmt);
                        if (ret2 < 0 && ret2 != AVERROR(EAGAIN)) {
                            isExit = true;
                        }
                        else {
                            if (ret2 != AVERROR(EAGAIN)) {
                                nVideoFrameCnt++;
                                debug_print("nFrameCount = %d\n", nVideoFrameCnt + 1);
                                if (nVideoFrameCnt > 100) {
                                    isExit = true;
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    else if (ret == AVERROR(EAGAIN)) {
                        debug_print("avcodec_receive_frame(): AVERROR(EAGAIN)\n");
                        continue;
                    }
                    else if (ret == AVERROR_EOF) {
                        debug_print("EOF\n");
                        break;
                    } else if (ret < 0) {
                        debug_print("从解码器接收视频帧时出错\n");
                        isExit = true;
                        break;
                    }
                }
                if (isExit)
                    break;
            }
        }

        if (isExit)
            break;

        // Audio
        if (1) {
            fSmartPtr<AVPacket> audioPacket = av_packet_alloc();
            ret = av_read_frame(inputAudioFormatCtx, audioPacket);
            if (ret < 0) {
                isExit = true;
                break;
            }
            if (audioPacket->stream_index == audioStreamIndex) {
                // 输入音频解码
                // See: https://www.cnblogs.com/leisure_chn/p/10584910.html
                av_packet_rescale_ts(audioPacket, inputAudioStream->time_base,
                                     inputAudioCodecCtx->time_base);
                fSmartPtr<AVFrame> frame = av_frame_alloc();
                int ret = avcodec_send_packet(inputAudioCodecCtx, audioPacket);
                if (ret < 0) {
                    debug_print("发送音频数据包到解码器时出错\n");
                    break;
                }
                while (1) {
                    ret = avcodec_receive_frame(inputAudioCodecCtx, frame);
                    if (ret == 0) {
                        // 将编码后的音频帧写入输出文件
                        //frame->pts = av_rescale_q(frame->pts, inputAudioCodecCtx->time_base, outputAudioStream->time_base);
                        //frame->pkt_dts = frame->pts;
                        //frame->pkt_duration = av_rescale_q(1, inputAudioCodecCtx->time_base, outputAudioStream->time_base);
                        if (frame->pts == AV_NOPTS_VALUE) {
                            frame->pts = frame->best_effort_timestamp;
                            frame->pkt_dts = frame->pts;
                            frame->pkt_duration = av_rescale_q(1, inputVideoStream->time_base, outputVideoStream->time_base);
                        }

                        ret2 = avcodec_encode_audio_frame(outputFormatCtx, outputAudioCodecCtx,
                                                          inputAudioStream, outputAudioStream,
                                                          frame, nAudioFrameCnt,
                                                          inputAudioCodecCtx->sample_fmt,
                                                          outputAudioCodecCtx->sample_fmt);
                        if (ret2 < 0 && ret2 != AVERROR(EAGAIN)) {
                            isExit = true;
                        }
                        else {
                            if (ret2 != AVERROR(EAGAIN)) {
                                nAudioFrameCnt++;
                                debug_print("nAudioCount = %d\n", nAudioFrameCnt + 1);
                                if (nAudioFrameCnt > 50) {
                                    isExit = true;
                                    break;
                                }
                            }
                        }
                        break;
                    }
                    else if (ret == AVERROR(EAGAIN)) {
                        break;
                    }
                    else if (ret == AVERROR_EOF) {
                        break;
                    }
                    else if (ret < 0) {
                        debug_print("从解码器接收音频帧时出错\n");
                        isExit = true;
                        break;
                    }
                }
            }
        }
        if (isExit)
            break;
    }

    // 写入文件尾
    ret = av_write_trailer(outputFormatCtx);
    ret = avformat_flush(outputFormatCtx);

    if (outputFormatCtx && !(outputFormatCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_close(outputFormatCtx->pb);
    }

    // 释放资源
    //avformat_close_input(&inputVideoFormatCtx);
    //avformat_close_input(&inputAudioFormatCtx);
    //avcodec_free_context(&outputVideoCodecCtx);
    //avcodec_free_context(&outputAudioCodecCtx);
    //avformat_free_context(outputFormatCtx);

    avformat_network_deinit();
    return 0;
}

int CCameraCapture::merge_video()
{
    AVFormatContext *video_ctx = NULL, *audio_ctx = NULL, *output_ctx = NULL;
    AVStream *video_stream = NULL, *audio_stream = NULL;
    const char *video_filename = "input_video.h264";
    const char *audio_filename = "input_audio.aac";
    const char *output_filename = "output_h264.mp4";
    int ret, cnt = 0;
    int frameIndex;

    // 初始化 FFmpeg 库
    av_register_all();

    // 打开视频文件
    if ((ret = avformat_open_input(&video_ctx, video_filename, NULL, NULL)) < 0) {
        debug_print("Could not open video file: %d\n", ret);
        return -1;
    }
    if ((ret = avformat_find_stream_info(video_ctx, NULL)) < 0) {
        debug_print("Could not find video stream info: %d\n", ret);
        return -1;
    }

    // 打开音频文件
    if ((ret = avformat_open_input(&audio_ctx, audio_filename, NULL, NULL)) < 0) {
        debug_print("Could not open audio file: %d\n", ret);
        return -1;
    }
    if ((ret = avformat_find_stream_info(audio_ctx, NULL)) < 0) {
        debug_print("Could not find audio stream info: %d\n", ret);
        return -1;
    }

    // 创建输出文件
    if ((ret = avformat_alloc_output_context2(&output_ctx, NULL, NULL, output_filename)) < 0) {
        debug_print("Could not create output context: %d\n", ret);
        return -1;
    }

    // 添加视频流
    video_stream = avformat_new_stream(output_ctx, NULL);
    if (!video_stream) {
        debug_print("Failed to create video stream\n");
        return -1;
    }
    avcodec_parameters_copy(video_stream->codecpar, video_ctx->streams[0]->codecpar);
    video_stream->codecpar->codec_tag = 0;

    // 添加音频流
    audio_stream = avformat_new_stream(output_ctx, NULL);
    if (!audio_stream) {
        debug_print("Failed to create audio stream\n");
        return -1;
    }
    avcodec_parameters_copy(audio_stream->codecpar, audio_ctx->streams[0]->codecpar);
    audio_stream->codecpar->codec_tag = 0;

    // 打开输出文件
    if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&output_ctx->pb, output_filename, AVIO_FLAG_WRITE)) < 0) {
            debug_print("Could not open output file: %d\n", ret);
            return -1;
        }
    }

    // 写入文件头
    if ((ret = avformat_write_header(output_ctx, NULL)) < 0) {
        debug_print("Error occurred when writing header to output file: %d\n", ret);
        return -1;
    }

    AVStream * i_video_stream = video_ctx->streams[0];
    video_stream->r_frame_rate = i_video_stream->r_frame_rate;

    AVStream * i_audio_stream = audio_ctx->streams[0];

    // 读取并写入视频帧
    cnt = 0;
    frameIndex = 0;

    // Parameters
    double av_time_base = (av_q2d(video_stream->time_base) * AV_TIME_BASE);
    // Duration between 2 frames (us)
    double duration = ((double)AV_TIME_BASE / av_q2d(video_stream->r_frame_rate));
    int64_t duration_64 = (int64_t)(duration / av_time_base);

    while (1) {
        AVPacket pkt;
        ret = av_read_frame(video_ctx, &pkt);
        if (ret == 0) {
            if (pkt.pts == AV_NOPTS_VALUE) {
                // Write PTS
                pkt.pts = (int64_t)((duration * frameIndex) / av_time_base);
                pkt.dts = pkt.pts;
                pkt.duration = duration_64;
            }
            frameIndex++;
            pkt.stream_index = video_stream->index;
            ret = av_interleaved_write_frame(output_ctx, &pkt);
            if (ret < 0) {
                debug_print("Error occurred when writing video packet to output file: %d\n", ret);
                break;
            }
            av_packet_unref(&pkt);
        }
        else if (ret == AVERROR(EAGAIN)) {
            cnt++;
            if (cnt > 100)
                break;
            continue;;
        }
        else if (ret == AVERROR_EOF) {
            break;
        }
        else if (ret < 0) {
            debug_print("Error occurred when writing video to output file: %d\n", ret);
            break;
        }
    }

    // 读取并写入音频帧
    cnt = 0;
    while (1) {
        AVPacket pkt;
        ret = av_read_frame(audio_ctx, &pkt);
        if (ret == 0) {
            pkt.stream_index = audio_stream->index;
            av_packet_rescale_ts(&pkt, i_audio_stream->time_base, audio_stream->time_base);
            ret = av_interleaved_write_frame(output_ctx, &pkt);
            if (ret < 0) {
                debug_print("Error occurred when writing audio packet to output file: %d\n", ret);
            }
            av_packet_unref(&pkt);
        }
        else if (ret == AVERROR(EAGAIN)) {
            cnt++;
            if (cnt > 100)
                break;
            continue;;
        }
        else if (ret == AVERROR_EOF) {
            break;
        }
        else if (ret < 0) {
            debug_print("Error occurred when writing audio to output file: %d\n", ret);
            break;
        }
    }

    // 写入文件尾
    av_write_trailer(output_ctx);

    // 释放资源
    avformat_close_input(&video_ctx);
    avformat_close_input(&audio_ctx);
    if (output_ctx && !(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&output_ctx->pb);
    }
    avformat_free_context(output_ctx);

    return 0;
}