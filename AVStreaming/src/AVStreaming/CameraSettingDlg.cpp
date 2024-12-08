//
// CCameraSettingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "AVStreaming.h"
#include "CameraSettingDlg.h"

#include "utils.h"

// CCameraSettingDlg 对话框

IMPLEMENT_DYNAMIC(CCameraSettingDlg, CDialogEx)

CCameraSettingDlg::CCameraSettingDlg(CWnd * pParent /*=NULL*/)
	: CDialogEx(IDD_CAMERA_SETTING_DLG, pParent)
{
    hwndPreview_ = NULL;
    pDShowCapture_ = NULL;
}

CCameraSettingDlg::~CCameraSettingDlg()
{
    SAFE_OBJECT_DELETE(pDShowCapture_);
}

void CCameraSettingDlg::DoDataExchange(CDataExchange * pDX)
{
	CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_VIDEO_DEVICE_LIST, cbxVideoDeviceList_);
    DDX_Control(pDX, IDC_AUDIO_DEVICE_LIST, cbxAudioDeviceList_);
}

BEGIN_MESSAGE_MAP(CCameraSettingDlg, CDialogEx)
    ON_CBN_SELCHANGE(IDC_VIDEO_DEVICE_LIST, &CCameraSettingDlg::OnCbnSelChangeVideoDeviceList)
    ON_CBN_SELCHANGE(IDC_AUDIO_DEVICE_LIST, &CCameraSettingDlg::OnCbnSelChangeAudioDeviceList)
    ON_WM_CREATE()
END_MESSAGE_MAP()

HWND CCameraSettingDlg::GetPreviewHwnd() const
{
    if (pDShowCapture_ != NULL) {
        return pDShowCapture_->GetPreviewHwnd();
    }
    return NULL;
}

HWND CCameraSettingDlg::SetPreviewHwnd(HWND hwndPreview, bool bAttachTo /* = false */)
{
    if (pDShowCapture_ != NULL) {
        return pDShowCapture_->SetPreviewHwnd(hwndPreview, bAttachTo);
    }
    return NULL;
}

// CCameraSettingDlg 消息处理程序

BOOL CCameraSettingDlg::Create(UINT nIDTemplate, HWND previewHwnd, CWnd* pParentWnd /*= NULL*/)
{
	if (pDShowCapture_ == NULL) {
        pDShowCapture_ = new DShowCapture;
        if (pDShowCapture_ != NULL) {
            pDShowCapture_->CreateInterfaces();
            if (previewHwnd != NULL) {
                pDShowCapture_->SetPreviewHwnd(previewHwnd);
            }
        }
    }
    return CDialogEx::Create(nIDTemplate, pParentWnd);
}

int CCameraSettingDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CDialogEx::OnCreate(lpCreateStruct) == -1)
        return -1;

    return 0;
}

BOOL CCameraSettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

    UpdateData(FALSE);

    if (pDShowCapture_ != NULL) {
        EnumVideoDeviceList();
        EnumAudioDeviceList();

        pDShowCapture_->ListVideoConfigures();
        pDShowCapture_->ListAudioConfigures();
    }

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

int CCameraSettingDlg::EnumVideoDeviceList()
{
    int nDeviceCount = 0;
    int default_selected = -1;
    std::string selected_device;

    if (pDShowCapture_ != NULL) {
        nDeviceCount = pDShowCapture_->ListVideoDevices();
        if (nDeviceCount > 0) {
            cbxVideoDeviceList_.Clear();
        }
        for (int i = 0; i < nDeviceCount; i++) {
            const std::string & deviceName = pDShowCapture_->videoDeviceList_[i];
            std::tstring deviceNameW = Ansi2Unicode(deviceName);
            if (deviceName == "HD WebCam") {
                default_selected = i;
                selected_device = deviceName;
            }               
            cbxVideoDeviceList_.AddString(deviceNameW.c_str());
        }
    }

    if (default_selected == -1)
        default_selected = 0;

    cbxVideoDeviceList_.SetCurSel(default_selected);
    OnCbnSelChangeVideoDeviceList();

    return nDeviceCount;
}

int CCameraSettingDlg::EnumAudioDeviceList()
{
    int nDeviceCount = 0;
    int default_selected = -1;

    if (pDShowCapture_ != NULL) {
        nDeviceCount = pDShowCapture_->ListAudioDevices();
        if (nDeviceCount > 0) {
            cbxAudioDeviceList_.Clear();
        }
        for (int i = 0; i < nDeviceCount; i++) {
            const std::string & deviceName = pDShowCapture_->audioDeviceList_[i];
            std::tstring deviceNameW = Ansi2Unicode(deviceName);             
            cbxAudioDeviceList_.AddString(deviceNameW.c_str());
        }
    }

    if (default_selected == -1)
        default_selected = 0;

    cbxAudioDeviceList_.SetCurSel(default_selected);
    OnCbnSelChangeAudioDeviceList();

    return nDeviceCount;
}

void CCameraSettingDlg::OnCbnSelChangeVideoDeviceList()
{
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

    if (pDShowCapture_ != NULL) {
        bool result = pDShowCapture_->Render(MODE_PREVIEW_VIDEO, NULL, selectedDevice.c_str());
    }
}

void CCameraSettingDlg::OnCbnSelChangeAudioDeviceList()
{
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
    if (pDShowCapture_ != NULL) {
        bool result = pDShowCapture_->CreateAudioFilter(selectedDevice.c_str());
    }
}

LRESULT CCameraSettingDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_GRAPH_NOTIFY:
            if (pDShowCapture_ != NULL) {
                pDShowCapture_->HandleGraphEvent();
            }
            break;

        case WM_SIZE:
            if (pDShowCapture_ != NULL) {
                pDShowCapture_->ResizeVideoWindow();
            }
            break;

        case WM_WINDOWPOSCHANGED:
            if (pDShowCapture_ != NULL) {
                pDShowCapture_->WindowStateChange(!this->IsIconic());
            }
            break;

        case WM_CLOSE:
            // Hide the main window while the graph is destroyed
            if (pDShowCapture_ != NULL) {
                HWND hwndPreview = pDShowCapture_->GetPreviewHwnd();
                if (hwndPreview != NULL && ::IsWindow(hwndPreview)) {
                    ::ShowWindow(hwndPreview, SW_HIDE);
                }
            }
            // Stop capturing and release interfaces
            pDShowCapture_->StopAndReleaseInterfaces();
            break;

        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }

    // Pass this message to the video window for notification of system changes
    if (pDShowCapture_ != NULL) {
        IVideoWindow * pVideoWindow = pDShowCapture_->GetVideoWindow();
        if (pVideoWindow != NULL) {
            pVideoWindow->NotifyOwnerMessage((LONG_PTR)this->GetSafeHwnd(), message, wParam, lParam);
        }
    }
    return CDialogEx::WindowProc(message, wParam, lParam);
}

BOOL CCameraSettingDlg::PreTranslateMessage(MSG * pMsg)
{

    return CDialogEx::PreTranslateMessage(pMsg);
}
