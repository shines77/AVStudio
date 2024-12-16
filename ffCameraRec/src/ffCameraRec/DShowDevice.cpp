
#include "DShowDevice.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>

#include "macros.h"
#include "global.h"

DShowDevice::DShowDevice()
{
    // 初始化 COM 库
    ::CoInitialize(NULL);

    InitInterfaces();
}

DShowDevice::~DShowDevice()
{
    StopAndReleaseInterfaces();

    ::CoUninitialize();
}

void DShowDevice::InitInterfaces()
{
    pFilterGraph_ = NULL;
    pCaptureBuilder_ = NULL;
}

void DShowDevice::ReleaseInterfaces()
{
    SAFE_COM_RELEASE(pCaptureBuilder_);
    SAFE_COM_RELEASE(pFilterGraph_);
}

HRESULT DShowDevice::CreateInterfaces()
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

    // 为 capture graph 指定要使用的 filter graph
    if (pCaptureBuilder_ != NULL && pFilterGraph_ != NULL) {
        hr = pCaptureBuilder_->SetFiltergraph(pFilterGraph_);
        if (FAILED(hr))
            return hr;
    }
    return hr;
}

HRESULT DShowDevice::StopAndReleaseInterfaces()
{
    ReleaseInterfaces();
    return S_OK;
}

void DShowDevice::init()
{
    ListVideoDevices();
    ListAudioDevices();
}

int DShowDevice::ListVideoDevices()
{
    HRESULT hr;

    // 创建系统设备枚举
    ICreateDevEnum * pCreateDevEnum = NULL;
    hr = ::CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
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
            console.error("Unable to access video capture device! index = %d", video_dev_total);
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
            hr = pPropBag->Read(L"FriendlyName", &var, NULL);
            if (hr != NOERROR) {
                hr = pPropBag->Read(L"Description", &var, NULL);
            }
            if (hr == NOERROR) {
                char deviceName[256] = { '\0' };
                // 获取设备名称
                ::WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, deviceName, sizeof(deviceName), "", NULL);
                ::SysFreeString(var.bstrVal);
                // 尝试用当前设备绑定到 video filter
                IBaseFilter * pVideoFilter = NULL;
                hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pVideoFilter);
                if (SUCCEEDED(hr) && pVideoFilter != NULL) {
                    // 绑定成功则添加到设备列表
                    videoDeviceList_.push_back(deviceName);
                    video_dev_count++;
                }
                else {
                    console.error("Video BindToObject Failed. index = %d", video_dev_total);
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

int DShowDevice::ListAudioDevices()
{
    HRESULT hr;

    // 创建系统设备枚举
    ICreateDevEnum * pCreateDevEnum = NULL;
    hr = ::CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
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
            console.error("Unable to access audio capture device! index = %d", audio_dev_total);
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
            hr = pPropBag->Read(L"FriendlyName", &var, NULL);
            if (hr != NOERROR) {
                hr = pPropBag->Read(L"Description", &var, NULL);
            }
            if (hr == NOERROR) {
                char deviceName[256] = { '\0' };
                // 获取设备名称
                ::WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, deviceName, sizeof(deviceName), "", NULL);
                ::SysFreeString(var.bstrVal);
                // 尝试用当前设备绑定到 audio filter
                IBaseFilter * pAudioFilter = NULL;
                hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pAudioFilter);
                if (SUCCEEDED(hr) && pAudioFilter != NULL) {
                    // 绑定成功则添加到设备列表
                    audioDeviceList_.push_back(deviceName);
                    audio_dev_count++;
                }
                else {
                    console.error("Audio BindToObject Failed. index = %d", audio_dev_total);
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
