//
// AVStreamingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "AVStreaming.h"
#include "AVStreamingDlg.h"

#include <AfxDialogEx.h>

#include "utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CAVStreamingDlg 对话框

CAVStreamingDlg::CAVStreamingDlg(CWnd* pParent /* = NULL */)
	: CDialogEx(IDD_AVSTREAMING_DIALOG, pParent)
{
    CoInitialize(NULL);   // 初始化 COM 库

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    dShowCapture_ = NULL;
}

CAVStreamingDlg::~CAVStreamingDlg()
{
    SAFE_OBJECT_DELETE(dShowCapture_);

    CoUninitialize();
}

void CAVStreamingDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_CAMERA_PREVIEW, wndCameraPreview_);
    DDX_Control(pDX, IDC_VIDEO_DEVICE_LIST, cbxVideoDeviceList_);
    DDX_Control(pDX, IDC_AUDIO_DEVICE_LIST, cbxAudioDeviceList_);
}

BEGIN_MESSAGE_MAP(CAVStreamingDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_CBN_SELCHANGE(IDC_VIDEO_DEVICE_LIST, &CAVStreamingDlg::OnCbnSelChangeVideoDeviceList)
    ON_CBN_SELCHANGE(IDC_AUDIO_DEVICE_LIST, &CAVStreamingDlg::OnCbnSelChangeAudioDeviceList)
END_MESSAGE_MAP()

// CAVStreamingDlg 消息处理程序

BOOL CAVStreamingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	// 执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	if (dShowCapture_ == NULL) {
        dShowCapture_ = new DShowCapture;
        if (dShowCapture_ != NULL) {
            dShowCapture_->Init();
            dShowCapture_->SetPreviewHwnd(wndCameraPreview_.GetSafeHwnd());

            EnumVideoDeviceList();
            EnumAudioDeviceList();

            dShowCapture_->ListVideoConfigures();
            dShowCapture_->ListAudioConfigures();
        }
    }

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

int CAVStreamingDlg::EnumVideoDeviceList()
{
    int nDeviceCount = 0;
    int default_selected = -1;
    std::string selected_device;

    if (dShowCapture_ != NULL) {
        nDeviceCount = dShowCapture_->ListVideoDevices();
        if (nDeviceCount > 0) {
            cbxVideoDeviceList_.Clear();
        }
        for (int i = 0; i < nDeviceCount; i++) {
            const std::string & deviceName = dShowCapture_->videoDeviceList_[i];
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

int CAVStreamingDlg::EnumAudioDeviceList()
{
    int nDeviceCount = 0;
    int default_selected = -1;

    if (dShowCapture_ != NULL) {
        nDeviceCount = dShowCapture_->ListAudioDevices();
        if (nDeviceCount > 0) {
            cbxAudioDeviceList_.Clear();
        }
        for (int i = 0; i < nDeviceCount; i++) {
            const std::string & deviceName = dShowCapture_->audioDeviceList_[i];
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

void CAVStreamingDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CAVStreamingDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 当用户拖动最小化窗口时系统调用此函数取得光标显示。
HCURSOR CAVStreamingDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CAVStreamingDlg::OnCbnSelChangeVideoDeviceList()
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

    if (dShowCapture_ != NULL) {
        bool result = dShowCapture_->Render(UVC_PREVIEW_VIDEO, NULL, selectedDevice.c_str());
    }
}

void CAVStreamingDlg::OnCbnSelChangeAudioDeviceList()
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
    if (dShowCapture_ != NULL) {
        bool result = dShowCapture_->CreateAudioFilter(selectedDevice.c_str());
    }
}
