//
// CCameraHostDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "AVStreaming.h"
#include "CameraHostDlg.h"

#include "macros.h"
#include "CameraCapture.h"
#include "utils.h"
#include "string_utils.h"

// CCameraHostDlg 对话框

IMPLEMENT_DYNAMIC(CCameraHostDlg, CDialogEx)

CCameraHostDlg::CCameraHostDlg(CWnd * pParent /*=NULL*/)
	: CDialogEx(IDD_CAMERA_HOST_DLG, pParent)
{
    hwndPreview_ = NULL;
    pCameraCapture_ = NULL;

    inited_ = false;
}

CCameraHostDlg::~CCameraHostDlg()
{
    inited_ = false;

    SAFE_OBJECT_DELETE(pCameraCapture_);
}

void CCameraHostDlg::DoDataExchange(CDataExchange * pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_VIDEO_DEVICE_LIST, cbxVideoDeviceList_);
    DDX_Control(pDX, IDC_AUDIO_DEVICE_LIST, cbxAudioDeviceList_);
    DDX_Control(pDX, IDC_CHECK_SAVE_TO_AVI, cbxSaveToAviFile_);
}

BEGIN_MESSAGE_MAP(CCameraHostDlg, CDialogEx)
    ON_CBN_SELCHANGE(IDC_VIDEO_DEVICE_LIST, &CCameraHostDlg::OnCbnSelChangeVideoDeviceList)
    ON_CBN_SELCHANGE(IDC_AUDIO_DEVICE_LIST, &CCameraHostDlg::OnCbnSelChangeAudioDeviceList)
    ON_WM_CREATE()
    ON_BN_CLICKED(IDC_BTN_START_CAPTURE, &CCameraHostDlg::OnClickedBtnStartCapture)
    ON_BN_CLICKED(IDC_BTN_STOP_CAPTURE, &CCameraHostDlg::OnClickedBtnStopCapture)
END_MESSAGE_MAP()

CameraCapture * CCameraHostDlg::GetSafeCapture() const {
    if (pCameraCapture_ != NULL && pCameraCapture_->GetPreviewHwnd() != NULL)
        return pCameraCapture_;
    else
        return NULL;
}

HWND CCameraHostDlg::GetPreviewHwnd() const
{
    if (pCameraCapture_ != NULL)
        return pCameraCapture_->GetPreviewHwnd();
    else
        return NULL;
}

HWND CCameraHostDlg::SetPreviewHwnd(HWND hwndPreview, bool bAttachTo /* = false */)
{
    if (pCameraCapture_ != NULL) {
        return pCameraCapture_->SetPreviewHwnd(hwndPreview, bAttachTo);
    }
    return NULL;
}

// CCameraHostDlg 消息处理程序

BOOL CCameraHostDlg::Create(UINT nIDTemplate, HWND previewHwnd, CWnd * pParentWnd /*= NULL*/)
{
	if (pCameraCapture_ == NULL) {
        pCameraCapture_ = new CameraCapture;
        if (pCameraCapture_ != NULL) {
            if (previewHwnd != NULL) {
                pCameraCapture_->SetPreviewHwnd(previewHwnd);
            }
        }
    }
    return CDialogEx::Create(nIDTemplate, pParentWnd);
}

int CCameraHostDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialogEx::OnCreate(lpCreateStruct) == -1)
        return -1;

    return 0;
}

BOOL CCameraHostDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

    UpdateData(FALSE);

    if (pCameraCapture_ != NULL) {
        size_t nVideoCount = EnumVideoDeviceList();
        size_t nAudioCount = EnumAudioDeviceList();

        size_t nVideoConfig = pCameraCapture_->EnumVideoConfigures();
        size_t nAudioConfig = pCameraCapture_->EnumAudioConfigures();

        inited_ = true;
        StartCapture();
    }

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

size_t CCameraHostDlg::EnumAudioDeviceList()
{
    size_t nDeviceCount = 0;
    if (pCameraCapture_ != NULL) {
        nDeviceCount = pCameraCapture_->EnumAudioDevices();
        cbxAudioDeviceList_.Clear();
        if (nDeviceCount > 0) {
            for (size_t i = 0; i < nDeviceCount; i++) {
                const std::tstring & deviceName = pCameraCapture_->audioDeviceList_[i];           
                cbxAudioDeviceList_.AddString(deviceName.c_str());
            }
            cbxAudioDeviceList_.SetCurSel(0);
        }
    }
    return nDeviceCount;
}

size_t CCameraHostDlg::EnumVideoDeviceList()
{
    size_t nDeviceCount = 0;
    if (pCameraCapture_ != NULL) {
        nDeviceCount = pCameraCapture_->EnumVideoDevices();
        cbxVideoDeviceList_.Clear();
        if (nDeviceCount > 0) {
            for (size_t i = 0; i < nDeviceCount; i++) {
                const std::tstring & deviceName = pCameraCapture_->videoDeviceList_[i];             
                cbxVideoDeviceList_.AddString(deviceName.c_str());
            }
            cbxVideoDeviceList_.SetCurSel(0);
        }
    }
    return nDeviceCount;
}

std::tstring CCameraHostDlg::GetSelectedVideoDevice() const
{
    std::tstring deviceName;
    if (cbxVideoDeviceList_.GetCount() <= 0)
        return deviceName;

    int selected_idx = cbxVideoDeviceList_.GetCurSel();
    if (selected_idx < 0)
        return deviceName;

    CString name;
    cbxVideoDeviceList_.GetWindowText(name);
    deviceName = name.GetBuffer();
    return deviceName;
}

std::tstring CCameraHostDlg::GetSelectedAudioDevice() const
{
    std::tstring deviceName;
    if (cbxAudioDeviceList_.GetCount() <= 0)
        return deviceName;

    int selected_idx = cbxAudioDeviceList_.GetCurSel();
    if (selected_idx < 0)
        return deviceName;

    CString name;
    cbxAudioDeviceList_.GetWindowText(name);
    deviceName = name.GetBuffer();   
    return deviceName;
}

bool CCameraHostDlg::StartCapture()
{
    if (!inited_) {
        return false;
    }

    if (pCameraCapture_ != NULL) {
        CameraCapture::PLAY_STATE playState = pCameraCapture_->GetPlayState();
        if (playState == CameraCapture::PLAY_STATE::Paused) {
            pCameraCapture_->ChangePreviewState(CameraCapture::PLAY_STATE::Running);
        }
        else if (playState == CameraCapture::PLAY_STATE::Running) {
            pCameraCapture_->ChangePreviewState(CameraCapture::PLAY_STATE::Paused);
        }
        else {
            pCameraCapture_->SetWantPreview(true);
            pCameraCapture_->SetWantCapture(true);

            std::tstring videoDevice = GetSelectedVideoDevice();
            std::tstring audioDevice = GetSelectedAudioDevice();
            if (videoDevice != selectedVideoDevice_ && audioDevice != selectedAudioDevice_) {
                // 选择视频和音频设备, 并开始预览画面
#if 1
                pCameraCapture_->CreateEnv();
                pCameraCapture_->ChooseDevices(videoDevice.c_str(), audioDevice.c_str());
#else
                pCameraCapture_->CreateEnv();
                pCameraCapture_->BindVideoFilter(videoDevice.c_str());
                pCameraCapture_->BindAudioFilter(audioDevice.c_str());
                pCameraCapture_->InitCaptureFilters();
                pCameraCapture_->BuildPreviewGraph();
                pCameraCapture_->StartPreview();
                //bool result = pCameraCapture_->Render(MODE_PREVIEW_VIDEO, NULL,
                //                                      videoDevice.c_str(), audioDevice.c_str());
#endif
                if (videoDevice != selectedVideoDevice_)
                    selectedVideoDevice_ = videoDevice;
                if (audioDevice != selectedAudioDevice_)
                    selectedAudioDevice_ = audioDevice;
            }
        }
        return true;
    }
    return false;
}

bool CCameraHostDlg::StartCaptureOld()
{
    if (!inited_) {
        return false;
    }

    if (pCameraCapture_ != NULL) {
        CameraCapture::PLAY_STATE playState = pCameraCapture_->GetPlayState();
        if (playState == CameraCapture::PLAY_STATE::Paused) {
            pCameraCapture_->ChangePreviewState(CameraCapture::PLAY_STATE::Running);
        }
        else if (playState == CameraCapture::PLAY_STATE::Running) {
            pCameraCapture_->ChangePreviewState(CameraCapture::PLAY_STATE::Paused);
        }
        else {
            std::tstring videoDevice = GetSelectedVideoDevice();
            std::tstring audioDevice = GetSelectedAudioDevice();
            if (videoDevice != selectedVideoDevice_ && audioDevice != selectedAudioDevice_) {
#if 1 || defined(_DEBUG)
                bool result = pCameraCapture_->Render(MODE_PREVIEW_VIDEO, NULL,
                                                      videoDevice.c_str(), audioDevice.c_str());
#elif 0
                bool result = pCameraCapture_->Render(MODE_RECORD_VIDEO, _T("test.avi"),
                                                      videoDevice.c_str(), audioDevice.c_str());
#else
                bool result = pCameraCapture_->Render(MODE_LOCAL_VIDEO, _T("local.avi"),
                                                      videoDevice.c_str(), audioDevice.c_str());
#endif
                if (videoDevice != selectedVideoDevice_)
                    selectedVideoDevice_ = videoDevice;
                if (audioDevice != selectedAudioDevice_)
                    selectedAudioDevice_ = audioDevice;
            }
        }
        return true;
    }
    return false;
}

bool CCameraHostDlg::StopCapture()
{
    if (!inited_) {
        return false;
    }

    if (pCameraCapture_ != NULL) {
        CameraCapture::PLAY_STATE playState = pCameraCapture_->GetPlayState();
        if (playState == CameraCapture::PLAY_STATE::Running) {
            pCameraCapture_->ChangePreviewState(CameraCapture::PLAY_STATE::Paused);
        }
        else if (playState == CameraCapture::PLAY_STATE::Paused) {
            pCameraCapture_->ChangePreviewState(CameraCapture::PLAY_STATE::Stopped);
        }
    }
    return true;
}

void CCameraHostDlg::OnCbnSelChangeVideoDeviceList()
{
    StartCapture();
}

void CCameraHostDlg::OnCbnSelChangeAudioDeviceList()
{
    StartCapture();
}

void CCameraHostDlg::OnClickedBtnStartCapture()
{
    StartCapture();
}

void CCameraHostDlg::OnClickedBtnStopCapture()
{
    StopCapture();
}

BOOL CCameraHostDlg::PreTranslateMessage(MSG * pMsg)
{
    if (pMsg->message == WM_KEYDOWN) {
        if (pMsg->wParam == VK_ESCAPE) {
            // 按下 ESC 键时不做任何处理
            return TRUE; // 返回 TRUE 表示消息已被处理
        }
    }
    return CDialogEx::PreTranslateMessage(pMsg);
}

LRESULT CCameraHostDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CLOSE:
            // Hide the main window while the graph is destroyed
            if (pCameraCapture_ != NULL) {
                HWND hwndPreview = pCameraCapture_->GetPreviewHwnd();
                if (hwndPreview != NULL && ::IsWindow(hwndPreview)) {
                    ::ShowWindow(hwndPreview, SW_HIDE);
                }
                // Stop capturing and release interfaces
                HRESULT hr = pCameraCapture_->Close();
                pCameraCapture_->Release();
            }
            break;
    }

    return CDialogEx::WindowProc(message, wParam, lParam);
}
