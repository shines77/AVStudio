#include "stdafx.h"
#include "CameraCapture.h"

#include <stdio.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>

#include <atlcomcli.h>      // For CComPtr<T>
#include <atlconv.h>
#include <assert.h>

#include "DShowUtil.h"
#include "SmartPtr.h"       // For SmartPtr<T>

#include "macros.h"
#include "global.h"
#include "utils.h"

#include "string_utils.h"
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

//
// From: https://blog.csdn.net/ett_qin/article/details/86691479
// See: https://www.cnblogs.com/linuxAndMcu/p/12068978.html
//

const TCHAR * CameraCapture::GetDeviceType(bool isVideo)
{
    return (isVideo) ? _T("Video") : _T("Audio");
}

const wchar_t * CameraCapture::GetDeviceFilterName(bool isVideo)
{
    return (isVideo) ? L"Video Filter" : L"Audio Filter";
}

REFCLSID CameraCapture::GetDeviceCategory(bool isVideo)
{
    return (isVideo) ? CLSID_VideoInputDeviceCategory : CLSID_AudioInputDeviceCategory;;
}

CameraCapture::CameraCapture(HWND hwndPreview /* = NULL */)
{
    hwndPreview_ = hwndPreview;

    InitEnv();
}

CameraCapture::~CameraCapture(void)
{
    HRESULT hr = Stop();
    Release();
}

void CameraCapture::InitEnv()
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

    pVideoMoniker_ = NULL;
    pAudioMoniker_ = NULL;

    pVideoGrabber_ = NULL;
    pAudioGrabber_ = NULL;

    pVideoCompression_ = NULL;

    pVideoStreamConfig_ = NULL;
    pAudioStreamConfig_ = NULL;

    pDroppedFrames_ = NULL;

    previewGraphBuilt_ = false;
    captureGraphBuilt_ = false;

    wantPreview_ = false;
    wantCapture_ = false;
    isPreviewing_ = false;
    isCapturing_ = false;

    isPreviewFaked_ = false;
}

void CameraCapture::Release()
{
    SAFE_COM_RELEASE(pVideoMux_);
    SAFE_COM_RELEASE(pVideoWindow_);
    SAFE_COM_RELEASE(pVideoMediaControl_);
    SAFE_COM_RELEASE(pVideoMediaEvent_);

    SAFE_COM_RELEASE(pVideoFilter_);
    SAFE_COM_RELEASE(pAudioFilter_);

    SAFE_COM_RELEASE(pVideoMoniker_);
    SAFE_COM_RELEASE(pAudioMoniker_);

    SAFE_COM_RELEASE(pVideoGrabber_);
    SAFE_COM_RELEASE(pAudioGrabber_);

    SAFE_COM_RELEASE(pCaptureBuilder_);
    SAFE_COM_RELEASE(pFilterGraph_);
}

HRESULT CameraCapture::CreateEnv()
{
    HRESULT hr;

    // 创建 filter graph manager
    SAFE_COM_RELEASE(pFilterGraph_);
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                          IID_IGraphBuilder, (void**)&pFilterGraph_);
    if (FAILED(hr))
        return hr;

    // 创建 capture graph builder
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

HRESULT CameraCapture::InitCaptureFilters()
{
    return 0;
}

void CameraCapture::FreeCaptureFilters()
{
    //
}

//
// Tear down everything downstream of a given filter
//
void CameraCapture::RemoveDownstream(IBaseFilter * pFilter)
{
    if (pFilter == nullptr)
        return;

    IEnumPins * pEnumPins = NULL;
    HRESULT hr = pFilter->EnumPins(&pEnumPins);
    if (hr == NOERROR && pEnumPins != nullptr) {
        hr = pEnumPins->Reset();
    }

    IPin *pPins = NULL, *pInputPin = NULL;
    PIN_INFO pin_info;
    ULONG uFetched = 0;
    while (hr == NOERROR) {
        hr = pEnumPins->Next(1, &pPins, &uFetched);
        if (hr == S_OK && pPins != nullptr) {
            pInputPin = nullptr;
            hr = pPins->ConnectedTo(&pInputPin);
            if (pInputPin != nullptr) {
                hr = pInputPin->QueryPinInfo(&pin_info);
                if (hr == NOERROR && pin_info.pFilter != nullptr) {
                    if (pin_info.dir == PINDIR_INPUT) {
                        // Recur call
                        RemoveDownstream(pin_info.pFilter);
                        pFilterGraph_->Disconnect(pInputPin);
                        pFilterGraph_->Disconnect(pPins);
                        pFilterGraph_->RemoveFilter(pin_info.pFilter);
                    }
                    pin_info.pFilter->Release();
                }
                pInputPin->Release();
            }
            pPins->Release();
        }
    }

    if (pEnumPins != nullptr) {
        pEnumPins->Release();
    }
}

//
// Tear down everything downstream of the capture filters, so we can build
// a different capture graph.  Notice that we never destroy the capture filters
// and WDM filters upstream of them, because then all the capture settings
// we've set would be lost.
//
void CameraCapture::TearDownGraph()
{
    SAFE_COM_RELEASE(pVideoMediaEvent_);
    SAFE_COM_RELEASE(pVideoMediaControl_);
    SAFE_COM_RELEASE(pDroppedFrames_);

    if (pVideoWindow_ != nullptr) {
        // Stop drawing in our window, or we may get wierd repaint effects
        pVideoWindow_->put_Owner(NULL);
        pVideoWindow_->put_Visible(OAFALSE);
        pVideoWindow_->Release();
        pVideoWindow_ = NULL;
    }

    // Destroy the graph downstream of our capture filters
    if (pVideoFilter_ != nullptr) {
        RemoveDownstream(pVideoFilter_);
    }
    if (pAudioFilter_ != nullptr) {
        RemoveDownstream(pAudioFilter_);
    }
    if (pVideoFilter_ != nullptr) {
        //pCaptureBuilder_->ReleaseFilters();
    }
}

void CameraCapture::OnClose()
{
    // Destroy the filter graph and cleanup
    StopPreview();
    StopDShowCapture();
    TearDownGraph();
    FreeCaptureFilters();
}

int GetStatusHeight()
{
    return 0;
}

// build the preview graph!
HRESULT CameraCapture::BuildPreviewGraph()
{
    HRESULT hr;

    // We have one already
    if (previewGraphBuilt_)
        return S_FALSE;

    // No rebuilding while we're running
    if (isCapturing_ || isPreviewing_)
        return S_FALSE;

    // We don't have the necessary capture filters
    if (pVideoFilter_ == NULL)
        return S_FALSE;

    if (pAudioFilter_ == NULL)
        return S_FALSE;
    
    // We already have another graph built ... tear down the old one
    if (captureGraphBuilt_)
        TearDownGraph();

    if (pCaptureBuilder_) {
        hr = pCaptureBuilder_->RenderStream(&PIN_CATEGORY_PREVIEW,
                                            &MEDIATYPE_Interleaved, pVideoFilter_, NULL, NULL);
        if (hr == VFW_S_NOPREVIEWPIN) {
            // Preview was faked up for us using the (only) capture pin
            isPreviewFaked_ = true;
        }
        else if (hr != S_OK) {
            // Maybe it's DV?
            hr = pCaptureBuilder_->RenderStream(&PIN_CATEGORY_PREVIEW,
                                                &MEDIATYPE_Video, pVideoFilter_, NULL, NULL);
            if (hr == VFW_S_NOPREVIEWPIN) {
                // Preview was faked up for us using the (only) capture pin
                isPreviewFaked_ = true;
            }
            else if (hr != S_OK) {
                console.error(_T("This graph cannot preview!"));
                previewGraphBuilt_ = false;
                return S_FALSE;
            }
        }
    }

    //
    // Get the preview window to be a child of our app's window
    //

    // This will find the IVideoWindow interface on the renderer.  It is
    // important to ask the filter graph for this interface... do NOT use
    // ICaptureGraphBuilder2::FindInterface, because the filter graph needs to
    // know we own the window so it can give us display changed messages, etc.

    if (pFilterGraph_) {
        hr = pFilterGraph_->QueryInterface(IID_IVideoWindow, (void **)&pVideoWindow_);
        if (hr != NOERROR) {
            console.error(_T("This graph cannot preview properly"));
        }
        else {
            // If we got here, pVideoWindow_ is not NULL
            ASSERT(pVideoWindow_ != NULL);
            hr = E_FAIL;
            if (hwndPreview_ != NULL && ::IsWindow(hwndPreview_)) {
                // Find out if this is a DV stream
                AM_MEDIA_TYPE * pmtDV = NULL;
                if (pVideoStreamConfig_ && SUCCEEDED(pVideoStreamConfig_->GetFormat(&pmtDV))) {
                    if (pmtDV->formattype == FORMAT_DvInfo) {
                        // In this case we want to set the size of the parent window to that of
                        // current DV resolution.
                        // We get that resolution from the IVideoWindow.
                        SmartPtr<IBasicVideo> pBasicVideo;

				        hr = pVideoWindow_->QueryInterface(IID_IBasicVideo, (void**)&pBasicVideo);
                        if (SUCCEEDED(hr)) {
                            HRESULT hr1, hr2;
                            long lWidth, lHeight;

                            hr1 = pBasicVideo->get_VideoHeight(&lHeight);
                            hr2 = pBasicVideo->get_VideoWidth(&lWidth);
                            if (SUCCEEDED(hr1) && SUCCEEDED(hr2)) {
                                ResizeVideoWindow(lWidth, abs(lHeight));
                            }
                        }
                    }
                }

                CRect rc;
                // We own the window now
                hr = pVideoWindow_->put_Owner((OAHWND)hwndPreview_);
                // you are now a child
                hr = pVideoWindow_->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN);

                // Give the preview window all our space but where the status bar is
                GetClientRect(hwndPreview_, &rc);
                int cyBorder = GetSystemMetrics(SM_CYBORDER);
                int cy = GetStatusHeight() + cyBorder;
                //rc.bottom -= cy;

                hr = pVideoWindow_->SetWindowPosition(0, 0, rc.right, rc.bottom); // be this big
                hr = pVideoWindow_->put_Visible(OATRUE);

                // Make sure we process events while we're previewing!
                hr = pFilterGraph_->QueryInterface(IID_IMediaEventEx, (void **)&pVideoMediaEvent_);
                if (hr == NOERROR) {
                    hr = pVideoMediaEvent_->SetNotifyWindow((OAHWND)hwndPreview_, WM_GRAPH_NOTIFY, 0);
                }
            }
        }
    }

    previewGraphBuilt_ = true;
    return hr;
}

HWND CameraCapture::SetPreviewHwnd(HWND hwndPreview, bool bAttachTo /* = false */)
{
    HWND oldHwnd = hwndPreview_;
    hwndPreview_ = hwndPreview;
    if (bAttachTo) {
        AttachToVideoWindow(hwndPreview);
    }
    return oldHwnd;
}

bool CameraCapture::AttachToVideoWindow(HWND hwndPreview)
{
    // 检查视频播放窗口
    if (pVideoWindow_ == NULL)
        return false;

    if (hwndPreview != NULL && ::IsWindow(hwndPreview)) {
        HRESULT hr;

        // Set the video window to be a child of the main window
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

void CameraCapture::ResizeVideoWindow(HWND hwndPreview /* = NULL */)
{
    if (hwndPreview == NULL)
        hwndPreview = hwndPreview_;

    if (pVideoWindow_ != NULL) {
        if (hwndPreview != NULL && ::IsWindow(hwndPreview)) {
            CRect rc;
            ::GetClientRect(hwndPreview, &rc);
            pVideoWindow_->SetWindowPosition(0, 0, rc.right, rc.bottom);
        }
    }
}

void CameraCapture::ResizeVideoWindow(long nWidth, long nHeight)
{
    HWND hwndPreview = hwndPreview_;
    if (hwndPreview != NULL && ::IsWindow(hwndPreview)) {
        BOOL result = ::MoveWindow(hwndPreview, 0, 0, nWidth, nHeight, TRUE);
        if (result) {
            ResizeVideoWindow(hwndPreview);
        }
    }
}

size_t CameraCapture::EnumVideoConfigures()
{
    size_t nConfigCount = 0;
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

size_t CameraCapture::EnumAudioConfigures()
{
    size_t nConfigCount = 0;
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

size_t CameraCapture::EnumAVDevices(std::vector<std::tstring> & deviceList, bool isVideo)
{
    // 创建系统设备枚举
    ICreateDevEnum * pCreateDevEnum = NULL;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum, (void **)&pCreateDevEnum);
    if (FAILED(hr) || pCreateDevEnum == NULL) {
        return hr;
    }

    REFCLSID CLSID_InputDeviceCategory = GetDeviceCategory(isVideo);

    // 创建一个指定系统采集设备的枚举
    IEnumMoniker * pEnumMoniker = NULL;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_InputDeviceCategory, &pEnumMoniker, 0);
    if (FAILED(hr) || pEnumMoniker == NULL) {
        SAFE_COM_RELEASE(pCreateDevEnum);
        return hr;
    }

    ULONG cFetched = 0;
    IMoniker * pMoniker = NULL;
    int nIndex = 0;
    deviceList.clear();

    // 枚举设备
    while (1) {
        hr = pEnumMoniker->Next(1, &pMoniker, &cFetched);
        if (hr == E_FAIL) {
            break;
        }
        else if (hr == S_FALSE) {
            console.error(_T("Unable to access %s capture device! index = %d"),
                          GetDeviceType(isVideo), nIndex);
            break;
        }
        else if (hr != S_OK) {
            console.error(_T("Unable to access %s capture device! Error = %x"),
                          GetDeviceType(isVideo), hr);
            break;
        }
        // 设备属性信息
        IPropertyBag * pPropBag = NULL;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if (SUCCEEDED(hr)) {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pPropBag->Read(L"FriendlyName", &var, NULL);
            if (hr != NOERROR) {
                hr = pPropBag->Read(L"Description", &var, NULL);
            }
            if (hr == NOERROR) {
                TCHAR deviceName[256] = { '\0' };
                // 获取设备名称
                int ret = string_utils::unicode_to_tchar(deviceName, _countof(deviceName), var.bstrVal);
                ::SysFreeString(var.bstrVal);
                // 尝试用当前设备绑定到 device filter
                IBaseFilter * pDeviceFilter = NULL;
                hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pDeviceFilter);
                if (SUCCEEDED(hr) && pDeviceFilter != NULL) {
                    // 绑定成功则添加到设备列表
                    deviceList.push_back(deviceName);
                }
                else {
                    console.error(_T("%s BindToObject Failed. index = %d"),
                                  GetDeviceType(isVideo), nIndex);
                }
                if (pDeviceFilter != NULL) {
                    pDeviceFilter->Release();
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

    return deviceList.size();
}

void CameraCapture::ReleaseDeviceFilter(IBaseFilter ** ppFilter)
{
    assert(ppFilter != nullptr);
    IBaseFilter * pFilter = *ppFilter;
    if (pFilter != nullptr) {
        RemoveDownstream(pFilter);
        pFilter->Release();
        //pFilter = nullptr;
        ppFilter = nullptr;
    }
}

HRESULT CameraCapture::BindDeviceFilter(IBaseFilter ** ppDeviceFilter, IMoniker ** ppDeviceMoniker,
                                        int nIndex, IMoniker * pMoniker,
                                        const TCHAR * inDeviceName, bool isVideo)
{
    assert(ppDeviceFilter != nullptr);
    assert(pMoniker != nullptr);
    assert(inDeviceName != nullptr);

    // 设备属性信息
    IPropertyBag * pPropBag = NULL;
    HRESULT hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
    if (SUCCEEDED(hr)) {
        VARIANT var;
        var.vt = VT_BSTR;
        hr = pPropBag->Read(L"FriendlyName", &var, NULL);
        if (hr != NOERROR) {
            hr = pPropBag->Read(L"Description", &var, NULL);
        }
        if (hr == NOERROR) {
            TCHAR deviceName[256] = { '\0' };
            // 获取设备名称
            int ret = string_utils::unicode_to_tchar(deviceName, _countof(deviceName), var.bstrVal);
            ::SysFreeString(var.bstrVal);
            hr = E_FAIL;
            if (inDeviceName != NULL && _tcscmp(inDeviceName, deviceName) == 0) {
                ReleaseDeviceFilter(ppDeviceFilter);
                // 尝试用当前设备绑定到 device filter
                hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)ppDeviceFilter);
                if (SUCCEEDED(hr) && ((ppDeviceFilter != NULL) && (*ppDeviceFilter != NULL))) {
                    if (ppDeviceMoniker != nullptr) {
                        *ppDeviceMoniker = pMoniker;
                        pMoniker->AddRef();
                    }
                    hr = S_OK;
                    if (pFilterGraph_ != nullptr) {
                        // 将 Device filter 加入 filter graph
                        IBaseFilter * pDeviceFilter = *ppDeviceFilter;
                        hr = pFilterGraph_->AddFilter(pDeviceFilter, GetDeviceFilterName(isVideo));
                        if (FAILED(hr)) {
                            console.error(_T("Failed to add %s device filter to filter graph: %x"),
                                          GetDeviceType(isVideo), hr);
                        }
                    }
                }
                else {
                    if (inDeviceName)
                        console.error(_T("%s BindToObject Failed. index = %d, name = %s"),
                                      GetDeviceType(isVideo), nIndex, inDeviceName);
                    else
                        console.error(_T("%s BindToObject Failed. index = %d, Moniker = %x"),
                                      GetDeviceType(isVideo), nIndex, pMoniker);
                    hr = E_FAIL;
                }
            }
        }
        pPropBag->Release();
    }
    return hr;
}

HRESULT CameraCapture::BindDeviceFilter(IBaseFilter ** ppDeviceFilter, IMoniker ** ppDeviceMoniker,
                                        const TCHAR * inDeviceName, bool isVideo)
{
    if (inDeviceName == NULL)
        return E_INVALIDARG;

    REFCLSID CLSID_InputDeviceCategory = GetDeviceCategory(isVideo);

    // 创建系统设备枚举
    ICreateDevEnum * pCreateDevEnum = NULL;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (FAILED(hr) || pCreateDevEnum == NULL) {
        return E_FAIL;
    }

    // 创建一个指定视频采集设备的枚举
    IEnumMoniker * pEnumMoniker = NULL;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_InputDeviceCategory, &pEnumMoniker, 0);
    if (FAILED(hr) || pEnumMoniker == NULL) {
        SAFE_COM_RELEASE(pCreateDevEnum);
        return E_FAIL;
    }

    ULONG cFetched = 0;
    int nIndex = 0;
    IMoniker * pMoniker = NULL;

    // 枚举设备
    while (1) {
        hr = pEnumMoniker->Next(1, &pMoniker, &cFetched);
        if (hr == E_FAIL) {
            break;
        }
        else if (hr == S_FALSE) {
            console.error(_T("Unable to access %s capture device! index = %d"),
                          GetDeviceType(isVideo), nIndex);
            break;
        }
        else if (hr != S_OK) {
            console.error(_T("Unable to access %s capture device! Error = %x"),
                          GetDeviceType(isVideo), hr);
            break;
        }
        if (pMoniker != nullptr) {
            hr = BindDeviceFilter(ppDeviceFilter, ppDeviceMoniker, nIndex, pMoniker, inDeviceName, isVideo);
            pMoniker->Release();
            pMoniker = NULL;
        }
        nIndex++;
    }

    pEnumMoniker->Release();
    pCreateDevEnum->Release();
    return hr;
}

// 枚举视频采集设备
size_t CameraCapture::EnumVideoDevices()
{
    return EnumAVDevices(videoDeviceList_, true);
}

// 枚举音频采集设备
size_t CameraCapture::EnumAudioDevices()
{
    return EnumAVDevices(audioDeviceList_, false);
}

// 根据选择的设备绑定 Video Capture Filter
HRESULT CameraCapture::BindVideoFilter(const TCHAR * videoDevice)
{
    if (videoDevice == NULL)
        return E_INVALIDARG;

    HRESULT hr = BindDeviceFilter(&pVideoFilter_, &pVideoMoniker_, videoDevice, true);
    return hr;
}

// 根据选择的设备绑定 Audio Capture Filter
HRESULT CameraCapture::BindAudioFilter(const TCHAR * audioDevice)
{
    if (audioDevice == NULL)
        return E_INVALIDARG;

    HRESULT hr = BindDeviceFilter(&pAudioFilter_, &pAudioMoniker_, audioDevice, false);
    return hr;
}

// 枚举视频压缩格式
size_t CameraCapture::EnumVideoCompressFormat()
{
    return 0;
}

// 枚举音频压缩格式
size_t CameraCapture::EnumAudioCompressFormat()
{
    return 0;
}

HRESULT CameraCapture::StartPreview()
{
    HRESULT hr = E_FAIL;
    if (isPreviewing_)
        return S_OK;

    if (!captureGraphBuilt_)
        return E_FAIL;

    // Run the filter graph
    if (pFilterGraph_) {
        IMediaControl * pMediaControl = NULL;
        HRESULT hr = pFilterGraph_->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
        if (SUCCEEDED(hr)) {
            hr = pMediaControl->Run();
            if (FAILED(hr)) {
                // Stop parts that ran
                pMediaControl->Stop();
            }
            pMediaControl->Release();
        }
        if (FAILED(hr)) {
            console.error(_T("Error %x: Cannot run preview graph"), hr);
            return hr;
        }
        isPreviewing_ = true;
    }    
    return hr;
}

HRESULT CameraCapture::StopPreview()
{
    HRESULT hr = E_FAIL;
    if (!isPreviewing_)
        return S_FALSE;

    if (!captureGraphBuilt_)
        return E_FAIL;

    // Stop the filter graph
    if (pFilterGraph_) {
        IMediaControl * pMediaControl = NULL;
        HRESULT hr = pFilterGraph_->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
        if (SUCCEEDED(hr)) {
            hr = pMediaControl->Stop();
            pMediaControl->Release();
        }
        if (FAILED(hr)) {
            console.error(_T("Error %x: Cannot stop preview graph"), hr);
            return hr;
        }
        if (hwndPreview_ != NULL && ::IsWindow(hwndPreview_)) {
            ::InvalidateRect(hwndPreview_, NULL, TRUE);
        }
        isPreviewing_ = false;
    }
    return hr;
}

HRESULT CameraCapture::Stop()
{
    HRESULT hr = E_FAIL;

    // Stop previewing data
    if (pVideoMediaControl_ != nullptr) {
        hr = pVideoMediaControl_->StopWhenReady();
    }

    playState_ = PLAY_STATE::Stopped;

    // Stop receiving events
    if (pVideoMediaEvent_ != nullptr) {
        hr = pVideoMediaEvent_->SetNotifyWindow(NULL, WM_GRAPH_NOTIFY, 0);
    }

    // Relinquish ownership (IMPORTANT!) of the video window.
    // Failing to call put_Owner can lead to assert failures within
    // the video renderer, as it still assumes that it has a valid
    // parent window.
    if (pVideoWindow_ != nullptr) {
        hr = pVideoWindow_->put_Visible(OAFALSE);
        hr = pVideoWindow_->put_Owner(NULL);
    }

    hwndPreview_ = NULL;
    return hr;
}

HRESULT CameraCapture::StartDShowCapture()
{
    HRESULT hr = E_FAIL;
    if (isCapturing_)
        return S_OK;

    if (isPreviewing_)
        StopPreview();

    if (!captureGraphBuilt_)
        return E_FAIL;

    nDroppedBase_ = 0;
    nNotDroppedBase_ = 0;

    REFERENCE_TIME start = MAXLONGLONG, stop = MAXLONGLONG;
    bool hasStreamControl = false;

    // Don't capture quite yet...
    if (pCaptureBuilder_) {
        hr = pCaptureBuilder_->ControlStream(&PIN_CATEGORY_CAPTURE, NULL,
                                             NULL, &start, NULL, 0, 0);
        // Do we have the ability to control capture and preview separately?
        hasStreamControl = SUCCEEDED(hr);

        // Prepare to run the graph
        if (pFilterGraph_) {
            IMediaControl * pMediaControl = NULL;
            hr = pFilterGraph_->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
            if (FAILED(hr)) {
                console.error(_T("Error %x: Cannot get capture IMediaControl"), hr);
                return E_FAIL;
            }

            if (hasStreamControl)
                hr = pMediaControl->Run();
            else
                hr = pMediaControl->Pause();
            if (FAILED(hr)) {
                // Stop parts that started
                pMediaControl->Stop();
                pMediaControl->Release();
                console.error(_T("Error %x: Cannot start capture graph"), hr);
                return E_FAIL;
            }

            // Start capture now !
            if (hasStreamControl) {
                // We may not have this yet
                if (!pVideoFilter_ && !pDroppedFrames_) {
                    hr = pCaptureBuilder_->FindInterface(&PIN_CATEGORY_CAPTURE,
                                                         &MEDIATYPE_Interleaved, pVideoFilter_,
                                                         IID_IAMDroppedFrames, (void **)&pDroppedFrames_);
                    if (hr != NOERROR) {
                        hr = pCaptureBuilder_->FindInterface(&PIN_CATEGORY_CAPTURE,
                                                             &MEDIATYPE_Video, pVideoFilter_,
                                                             IID_IAMDroppedFrames, (void **)&pDroppedFrames_);
                    }
                }

                // Turn the capture pin on now!
                hr = pCaptureBuilder_->ControlStream(&PIN_CATEGORY_CAPTURE, NULL,
                                                     NULL, NULL, &stop, 0, 0);
                // Make note of the current dropped frame counts
                if (pDroppedFrames_) {
                    pDroppedFrames_->GetNumDropped(&nDroppedBase_);
                    pDroppedFrames_->GetNumNotDropped(&nNotDroppedBase_);
                }
            }
            else {
                hr = pMediaControl->Run();
                if (FAILED(hr)) {
                    // Stop parts that started
                    pMediaControl->Stop();
                    pMediaControl->Release();
                    console.error(_T("Error %x: Cannot run capture graph"), hr);
                    return E_FAIL;
                }
            }
            pMediaControl->Release();

            if (wantPreview_)
                isPreviewing_ = true;
            isCapturing_ = true;
        }
    }
    return hr;
}

HRESULT CameraCapture::StopDShowCapture()
{
    HRESULT hr = E_FAIL;
    if (!isCapturing_)
        return S_FALSE;

    if (!captureGraphBuilt_)
        return E_FAIL;

    // Stop the filter graph
    if (pFilterGraph_) {
        IMediaControl * pMediaControl = NULL;
        HRESULT hr = pFilterGraph_->QueryInterface(IID_IMediaControl, (void **)&pMediaControl);
        if (SUCCEEDED(hr)) {
            hr = pMediaControl->Stop();
            pMediaControl->Release();
        }
        if (FAILED(hr)) {
            console.error(_T("Error %x: Cannot stop capture graph"), hr);
            return hr;
        }
        if (hwndPreview_ != NULL && ::IsWindow(hwndPreview_)) {
            ::InvalidateRect(hwndPreview_, NULL, TRUE);
        }
        isCapturing_ = false;
    }
    return hr;
}

// 渲染摄像头预览视频
bool CameraCapture::Render(int mode, TCHAR * videoPath,
                           const TCHAR * videoDevice,
                           const TCHAR * audioDevice)
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
        hr = BindVideoFilter(videoDevice);

        if (mode == MODE_RECORD_VIDEO) {
            // 创建 Audio filter
            hr = BindAudioFilter(audioDevice);
        }
    }

    if (mode == MODE_PREVIEW_VIDEO) {     // 预览视频
        hr = pCaptureBuilder_->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pVideoFilter_, NULL, NULL);
        if (hr == VFW_S_NOPREVIEWPIN) {
            // preview was faked up for us using the (only) capture pin
        }
        else if (hr != S_OK) {
            console.error(_T("This graph cannot preview!"));
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
            console.error(_T("This graph cannot preview!"));
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

HRESULT CameraCapture::HandleGraphEvent(void)
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

HRESULT CameraCapture::WindowStateChange(BOOL isShow)
{
    HRESULT hr;
    if (isShow)
        hr = ChangePreviewState(PLAY_STATE::Running);
    else
        hr = ChangePreviewState(PLAY_STATE::Stopped);
    return hr;
}

HRESULT CameraCapture::ChangePreviewState(PLAY_STATE playState /* = PLAY_STATE::Running */)
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
bool CameraCapture::StopCurrentOperating(int action_type)
{
    if (pVideoMediaControl_ == nullptr)
        return false;

    HRESULT hr = pVideoMediaControl_->Stop();
    if (FAILED(hr))
        return false;

    if (action_type != ACTION_STOP_LOCAL_VIDEO) {
        if (pVideoFilter_)
            pVideoFilter_->Release();
        if (pCaptureBuilder_)
            pCaptureBuilder_->Release();
    }

    if (pVideoWindow_)
        pVideoWindow_->Release();
    if (pVideoMediaControl_)
        pVideoMediaControl_->Release();
    if (pFilterGraph_)
        pFilterGraph_->Release();
    return true;
}

// 暂停播放本地视频
bool CameraCapture::PausePlayingLocalVideo()
{
    if (pVideoMediaControl_ == nullptr)
        return false;

    HRESULT hr = pVideoMediaControl_->Stop();
    if (hr < 0)
        return false;
    else
        return true;
}

// 继续播放本地视频
bool CameraCapture::ContinuePlayingLocalVideo()
{
    if (pVideoMediaControl_ == nullptr)
        return false;

    HRESULT hr = pVideoMediaControl_->Run();
    if (FAILED(hr))
        return false;
    else
        return true;
}
