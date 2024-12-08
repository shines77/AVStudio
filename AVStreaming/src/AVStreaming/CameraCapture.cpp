#include "stdafx.h"
#include "CameraCapture.h"

#include <atlconv.h>

#include "Mtype.h"

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

    // ���� filter graph manager
    SAFE_COM_RELEASE(pFilterGraph_);
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pFilterGraph_);
	if (FAILED(hr))
		return hr;

    // ���� capture graph manager
    SAFE_COM_RELEASE(pCaptureBuilder_);
    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
                          IID_ICaptureGraphBuilder2, (void**)&pCaptureBuilder_);
	if (FAILED(hr))
		return hr;

    // ������Ƶ���Ŵ���
    SAFE_COM_RELEASE(pVideoWindow_);
    hr = pFilterGraph_->QueryInterface(IID_IVideoWindow, (void **)&pVideoWindow_);
	if (FAILED(hr))
		return hr;

    playState_ = PLAY_STATE::Unknown;

    // ��������ͷ��ý��Ŀ��ƿ���
    SAFE_COM_RELEASE(pVideoMediaControl_);
    hr = pFilterGraph_->QueryInterface(IID_IMediaControl, (void **)&pVideoMediaControl_);
	if (FAILED(hr))
		return hr;

    // ��������ͷ��ý��Ŀ����¼�
    SAFE_COM_RELEASE(pVideoMediaEvent_);
    hr = pFilterGraph_->QueryInterface(IID_IMediaEventEx, (void **)&pVideoMediaEvent_);
	if (FAILED(hr))
		return hr;

    // Ϊ capture graph ָ��Ҫʹ�õ� filter graph
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
    // �����Ƶ���Ŵ���
    if (pVideoWindow_ == NULL)
        return false;

    if (hwndPreview != NULL && ::IsWindow(hwndPreview)) {
        HRESULT hr;

        // Set the video window to be a child of the main window
        // ��Ƶ������ "Picture�ؼ�" ����Ԥ����Ƶ
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
            // ��ͼ�������������
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
		// &MEDIATYPE_Video, �����������ý������, �ڶ�����������Ϊ NULL, ��ʾ Any media type.
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
		// &MEDIATYPE_Video, �����������ý������, �ڶ�����������Ϊ NULL, ��ʾ Any media type.
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

// ö����Ƶ�ɼ��豸
int CCameraCapture::ListVideoDevices()
{
    HRESULT hr;

    // ����ϵͳ�豸ö��
    ICreateDevEnum * pCreateDevEnum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (FAILED(hr) || pCreateDevEnum == NULL) {
        return hr;
    }

    // ����һ��ָ����Ƶ�ɼ��豸��ö��
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

    // ö���豸
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
        // �豸������Ϣ
        IPropertyBag * pPropBag = NULL;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if (SUCCEEDED(hr)) {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pPropBag->Read(L"Description", &var, NULL);
            if (FAILED(hr)) {
                hr = pPropBag->Read(L"FriendlyName", &var, NULL);
            }
            if (SUCCEEDED(hr)) {
                char deviceName[256] = { '\0' };
                // ��ȡ�豸����
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, deviceName, sizeof(deviceName), "", NULL);
                SysFreeString(var.bstrVal);
                // �����õ�ǰ�豸�󶨵� video filter
                IBaseFilter * pVideoFilter = NULL;
                hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pVideoFilter);
                if (SUCCEEDED(hr)) {
                    // �󶨳ɹ������ӵ��豸�б�
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

// ö����Ƶ�ɼ��豸
int CCameraCapture::ListAudioDevices()
{
    HRESULT hr;

    // ����ϵͳ�豸ö��
    ICreateDevEnum * pCreateDevEnum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (FAILED(hr) || pCreateDevEnum == NULL) {
        return hr;
    }

    // ����һ��ָ����Ƶ�ɼ��豸��ö��
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

    // ö���豸
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
        // �豸������Ϣ
        IPropertyBag * pPropBag = NULL;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if (SUCCEEDED(hr)) {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pPropBag->Read(L"Description", &var, NULL);
            if (FAILED(hr)) {
                hr = pPropBag->Read(L"FriendlyName", &var, NULL);
            }
            if (SUCCEEDED(hr)) {
                char deviceName[256] = { '\0' };
                // ��ȡ�豸����
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, deviceName, sizeof(deviceName), "", NULL);
                SysFreeString(var.bstrVal);
                // �����õ�ǰ�豸�󶨵� audio filter
                IBaseFilter * pAudioFilter = NULL;
                hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void **)&pAudioFilter);
                if (SUCCEEDED(hr)) {
                    // �󶨳ɹ������ӵ��豸�б�
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

// ö����Ƶѹ����ʽ
int CCameraCapture::ListVideoCompressFormat()
{
    return 0;
}

// ö����Ƶѹ����ʽ
int CCameraCapture::ListAudioCompressFormat()
{
    return 0;
}

// ����ѡ����豸��ȡ Video Capture Filter
bool CCameraCapture::CreateVideoFilter(const char * selectedDevice)
{
    // ����ϵͳ�豸ö��
    ICreateDevEnum * pCreateDevEnum = NULL;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (FAILED(hr) || pCreateDevEnum == NULL) {
        return false;
    }

    // ����һ��ָ����Ƶ�ɼ��豸��ö��
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

    // ö���豸
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
        // �豸������Ϣ
        IPropertyBag * pPropBag = NULL;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if (SUCCEEDED(hr)) {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pPropBag->Read(L"Description", &var, NULL);
            if (FAILED(hr)) {
                hr = pPropBag->Read(L"FriendlyName", &var, NULL);
            }
            if (SUCCEEDED(hr)) {
                char deviceName[256] = { '\0' };
                // ��ȡ�豸����
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, deviceName, sizeof(deviceName), "", NULL);
                SysFreeString(var.bstrVal);
                if (selectedDevice != NULL && strcmp(selectedDevice, deviceName) == 0) {
                    SAFE_COM_RELEASE(pVideoFilter_);
                    // �����õ�ǰ�豸�󶨵� video filter
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

// ����ѡ����豸��ȡ Audio Capture Filter
bool CCameraCapture::CreateAudioFilter(const char * selectedDevice)
{
    // ����ϵͳ�豸ö��
    ICreateDevEnum * pCreateDevEnum = NULL;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (FAILED(hr) || pCreateDevEnum == NULL) {
        return false;
    }

    // ����һ��ָ����Ƶ�ɼ��豸��ö��
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

    // ö���豸
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
        // �豸������Ϣ
        IPropertyBag * pPropBag = NULL;
        hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
        if (SUCCEEDED(hr)) {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = pPropBag->Read(L"Description", &var, NULL);
            if (FAILED(hr)) {
                hr = pPropBag->Read(L"FriendlyName", &var, NULL);
            }
            if (SUCCEEDED(hr)) {
                char deviceName[256] = { '\0' };
                // ��ȡ�豸����
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, deviceName, sizeof(deviceName), "", NULL);
                SysFreeString(var.bstrVal);
                if (selectedDevice != NULL && strcmp(selectedDevice, deviceName) == 0) {
                    SAFE_COM_RELEASE(pAudioFilter_);
                    // �����õ�ǰ�豸�󶨵� audio filter
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

// ��Ⱦ����ͷԤ����Ƶ
bool CCameraCapture::Render(int mode, TCHAR * videoPath, const char * selectedDevice)
{
    HRESULT hr;

    // ��� Video filter graph (������)
    if (pFilterGraph_ == NULL)
        return false;

    if (mode != MODE_LOCAL_VIDEO) {
        // ���� Video filter
        bool result = CreateVideoFilter(selectedDevice);
        if (!result)
            return false;

        // ��� Video capture graph (������)
        if (pCaptureBuilder_ == NULL)
            return false;

        // ����ȡ���� Video filter ���� Video filter graph
        if (hr = pFilterGraph_->AddFilter(pVideoFilter_, L"Video Filter"), FAILED(hr))
            return false;
    }

    if (mode == MODE_PREVIEW_VIDEO) {     // Ԥ����Ƶ
        if (pCaptureBuilder_->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pVideoFilter_, NULL, NULL) < 0)
            return false;
    }
    else if (mode == MODE_RECORD_VIDEO) { // ¼����Ƶ
        USES_CONVERSION;
        LPCOLESTR OLE_PathName = T2OLE(videoPath); // ����ת��

        // ��������ļ�
        SAFE_COM_RELEASE(pVideoMux_);
        if (pCaptureBuilder_->SetOutputFileName(&MEDIASUBTYPE_Avi, OLE_PathName, &pVideoMux_, NULL) < 0)
            return false;

        // ¼�Ƶ�ʱ��Ҳ��ҪԤ��
        if (pCaptureBuilder_->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pVideoFilter_, NULL, NULL) < 0)
            return false;

        // ��Ƶ��д�� avi �ļ�
        if (pVideoMux_ != NULL && pCaptureBuilder_->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVideoFilter_, NULL, pVideoMux_) < 0)
            return false;
    }
    else if (mode == MODE_LOCAL_VIDEO) {   // ���ű�����Ƶ
        USES_CONVERSION;
        LPCOLESTR OLE_PathName = T2OLE(videoPath); // ����ת��
        pFilterGraph_->RenderFile(OLE_PathName, NULL);
    }

    /******* ������Ƶ���Ŵ��� *******/
    if (!AttachToVideoWindow(hwndPreview_))
        return false;

    // �������ͷ��ý��Ŀ��ƿ���
    if (pVideoMediaControl_ == NULL)
        return false;

    // ��ʼԤ����¼��
    if (pVideoMediaControl_->Run() < 0)
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
        playState_ = playState;
    }

    return hr;
}

// �ر�����ͷ
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

// ��ͣ���ű�����Ƶ
bool CCameraCapture::PausePlayingLocalVideo()
{
    if (pVideoMediaControl_->Stop() < 0)
        return false;
    else
        return true;
}

// �������ű�����Ƶ
bool CCameraCapture::ContinuePlayingLocalVideo()
{
    if (pVideoMediaControl_->Run() < 0)
        return false;
    else
        return true;
}