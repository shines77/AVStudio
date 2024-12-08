//
// CCameraHostDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "AVStreaming.h"
#include "CameraHostDlg.h"

#include "CameraCapture.h"
#include "utils.h"

// CCameraHostDlg 对话框

IMPLEMENT_DYNAMIC(CCameraHostDlg, CDialogEx)

CCameraHostDlg::CCameraHostDlg(CWnd * pParent /*=NULL*/)
	: CDialogEx(IDD_CAMERA_SETTING_DLG, pParent)
{
    hwndPreview_ = NULL;
    pCameraCapture_ = NULL;
}

CCameraHostDlg::~CCameraHostDlg()
{
    SAFE_OBJECT_DELETE(pCameraCapture_);
}

void CCameraHostDlg::DoDataExchange(CDataExchange * pDX)
{
	CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_VIDEO_DEVICE_LIST, cbxVideoDeviceList_);
    DDX_Control(pDX, IDC_AUDIO_DEVICE_LIST, cbxAudioDeviceList_);
}

BEGIN_MESSAGE_MAP(CCameraHostDlg, CDialogEx)
    ON_CBN_SELCHANGE(IDC_VIDEO_DEVICE_LIST, &CCameraHostDlg::OnCbnSelChangeVideoDeviceList)
    ON_CBN_SELCHANGE(IDC_AUDIO_DEVICE_LIST, &CCameraHostDlg::OnCbnSelChangeAudioDeviceList)
    ON_WM_CREATE()
END_MESSAGE_MAP()

CCameraCapture * CCameraHostDlg::GetSafeCapture() const {
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
        pCameraCapture_ = new CCameraCapture;
        if (pCameraCapture_ != NULL) {
            pCameraCapture_->CreateInterfaces();
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
        EnumVideoDeviceList();
        EnumAudioDeviceList();

        pCameraCapture_->ListVideoConfigures();
        pCameraCapture_->ListAudioConfigures();
    }

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

int CCameraHostDlg::EnumVideoDeviceList()
{
    int nDeviceCount = 0;
    if (pCameraCapture_ != NULL) {
        nDeviceCount = pCameraCapture_->ListVideoDevices();
        cbxVideoDeviceList_.Clear();
        if (nDeviceCount > 0) {
            for (int i = 0; i < nDeviceCount; i++) {
                const std::string & deviceName = pCameraCapture_->videoDeviceList_[i];
                std::tstring deviceNameW = Ansi2Unicode(deviceName);              
                cbxVideoDeviceList_.AddString(deviceNameW.c_str());
            }
            cbxVideoDeviceList_.SetCurSel(0);
            OnCbnSelChangeVideoDeviceList();
        }
    }
    return nDeviceCount;
}

int CCameraHostDlg::EnumAudioDeviceList()
{
    int nDeviceCount = 0;
    if (pCameraCapture_ != NULL) {
        nDeviceCount = pCameraCapture_->ListAudioDevices();
        cbxAudioDeviceList_.Clear();
        if (nDeviceCount > 0) {
            for (int i = 0; i < nDeviceCount; i++) {
                const std::string & deviceName = pCameraCapture_->audioDeviceList_[i];
                std::tstring deviceNameW = Ansi2Unicode(deviceName);             
                cbxAudioDeviceList_.AddString(deviceNameW.c_str());
            }
            cbxAudioDeviceList_.SetCurSel(0);
            OnCbnSelChangeAudioDeviceList();
        }
    }
    return nDeviceCount;
}

void CCameraHostDlg::OnCbnSelChangeVideoDeviceList()
{
    if (cbxVideoDeviceList_.GetCount() <= 0)
        return;

    int selected_idx = cbxVideoDeviceList_.GetCurSel();
    if (selected_idx < 0)
        return;

    CString name;
    cbxVideoDeviceList_.GetWindowText(name);
    std::tstring deviceNameW = name.GetBuffer();
    if (deviceNameW.empty()) {
        return;
    }
    std::string selectedDevice = Unicode2Ansi(deviceNameW);

    if (pCameraCapture_ != NULL) {
        bool result = pCameraCapture_->Render(MODE_PREVIEW_VIDEO, NULL, selectedDevice.c_str());
    }
}

void CCameraHostDlg::OnCbnSelChangeAudioDeviceList()
{
    if (cbxAudioDeviceList_.GetCount() <= 0)
        return;

    int selected_idx = cbxAudioDeviceList_.GetCurSel();
    if (selected_idx < 0)
        return;

    CString name;
    cbxAudioDeviceList_.GetWindowText(name);
    std::tstring deviceNameW = name.GetBuffer();
    if (deviceNameW.empty()) {
        return;
    }

    std::string selectedDevice = Unicode2Ansi(deviceNameW);
    if (pCameraCapture_ != NULL) {
        bool result = pCameraCapture_->CreateAudioFilter(selectedDevice.c_str());
    }
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
        case WM_GRAPH_NOTIFY:
            if (pCameraCapture_ != NULL) {
                pCameraCapture_->HandleGraphEvent();
            }
            break;

        case WM_CLOSE:
            // Hide the main window while the graph is destroyed
            if (pCameraCapture_ != NULL) {
                HWND hwndPreview = pCameraCapture_->GetPreviewHwnd();
                if (hwndPreview != NULL && ::IsWindow(hwndPreview)) {
                    ::ShowWindow(hwndPreview, SW_HIDE);
                }
                // Stop capturing and release interfaces
                pCameraCapture_->StopAndReleaseInterfaces();
            }
            break;

        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }

    // Pass this message to the video window for notification of system changes
    if (pCameraCapture_ != NULL) {
        IVideoWindow * pVideoWindow = pCameraCapture_->GetVideoWindow();
        if (pVideoWindow != NULL) {
            pVideoWindow->NotifyOwnerMessage((LONG_PTR)this->GetSafeHwnd(), message, wParam, lParam);
        }
    }
    return CDialogEx::WindowProc(message, wParam, lParam);
}
