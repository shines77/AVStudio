//
// AVStreamingDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "AVStreaming.h"
#include "AVStreamingDlg.h"

#include "PreviewWnd.h"
#include "CameraHostDlg.h"
#include "CameraCapture.h"
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

IMPLEMENT_DYNAMIC(CAVStreamingDlg, CDialogEx)

CAVStreamingDlg::CAVStreamingDlg(CWnd* pParent /* = NULL */)
	: CDialogEx(IDD_AVSTREAMING_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    pPreviewWnd_ = NULL;
    pCameraHostDlg_ = NULL;
}

CAVStreamingDlg::~CAVStreamingDlg()
{
    SAFE_OBJECT_DELETE(pCameraHostDlg_);
    SAFE_OBJECT_DELETE(pPreviewWnd_);
}

void CAVStreamingDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAVStreamingDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()

// CAVStreamingDlg 消息处理程序

void CAVStreamingDlg::SetScale(double scaleX, double scaleY)
{
    // 获取对话框的窗口矩形
    CRect rect;
    GetWindowRect(&rect);

    // 缩放对话框的大小
    rect.right = rect.left + (int)(rect.Width() * scaleX);
    rect.bottom = rect.top + (int)(rect.Height() * scaleY);
    MoveWindow(&rect);

    // 缩放对话框中的控件
    CWnd * pWnd = GetWindow(GW_CHILD);
    while (pWnd != NULL) {
        CRect controlRect;
        pWnd->GetWindowRect(&controlRect);
        ScreenToClient(&controlRect);

        controlRect.left = (int)(controlRect.left * scaleX);
        controlRect.top = (int)(controlRect.top * scaleY);
        controlRect.right = (int)(controlRect.right * scaleX);
        controlRect.bottom = (int)(controlRect.bottom * scaleY);

        pWnd->MoveWindow(&controlRect);

        pWnd = pWnd->GetNextWindow();
    }
}

BOOL CAVStreamingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

    // 获取屏幕的 DPI
    HDC hdc = ::GetDC(NULL);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
    ::ReleaseDC(NULL, hdc);

    // 计算缩放比例
    double scaleX = (double)dpiX / 96.0;
    double scaleY = (double)dpiY / 96.0;

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

    if (pPreviewWnd_ == NULL) {
        CRect rcWnd(10, 10, 10 + kDefaultVideoPreviewWidth, 10 + kDefaultVideoPreviewHeight);
        CPreviewWnd * pPreviewWnd = new CPreviewWnd;
        if (pPreviewWnd != NULL) {
            pPreviewWnd->Create(_T("摄像头预览窗口"),
                                 WS_CHILD | WS_CLIPCHILDREN, rcWnd,
                                 this, IDC_CAMERA_PREVIEW);
            pPreviewWnd->ShowWindow(SW_SHOWNORMAL);

            CRect rcClient;
            pPreviewWnd->GetClientRect(rcClient);

            CString text;
            text.Format(_T("r: %ul, l: %ul, width: %d, height: %d\n"), rcClient.left, rcClient.left, rcClient.Width(), rcClient.Height());
            ::OutputDebugString(text.GetBuffer());

            pPreviewWnd_ = pPreviewWnd;
        }
    }

    if (pCameraHostDlg_ == NULL) {
        CCameraHostDlg * pCameraHostDlg = new CCameraHostDlg;
        if (pCameraHostDlg != NULL) {
            if (pPreviewWnd_ != NULL && pPreviewWnd_->GetSafeHwnd()) {
                pCameraHostDlg->Create(IDD_CAMERA_HOST_DLG, pPreviewWnd_->GetSafeHwnd(), this);
                pPreviewWnd_->SetHostWnd(pCameraHostDlg);
            }
            else {
                pCameraHostDlg->Create(IDD_CAMERA_HOST_DLG, NULL, this);
            }
        
            pCameraHostDlg->ShowWindow(SW_SHOWNORMAL);

            CRect rcClient;
            pCameraHostDlg->GetClientRect(rcClient);
            pCameraHostDlg->MoveWindow(10 + kDefaultVideoPreviewWidth + 10, 10,
                                       rcClient.Width(), rcClient.Height(), TRUE);

            pCameraHostDlg_ = pCameraHostDlg;
        }
    }

    // 设置对话框的缩放比例
    SetScale(scaleX, scaleY);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
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

BOOL CAVStreamingDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN) {
        if (pMsg->wParam == VK_ESCAPE) {
            // 按下 ESC 键时不做任何处理
            return TRUE; // 返回 TRUE 表示消息已被处理
        }
    }
    return CDialogEx::PreTranslateMessage(pMsg);
}

LRESULT CAVStreamingDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_WINDOWPOSCHANGED:
            if (pCameraHostDlg_ != NULL) {
                CCameraCapture * pCameraCapture = pCameraHostDlg_->GetSafeCapture();
                if (pCameraCapture != NULL) {
                    pCameraCapture->WindowStateChange(!this->IsIconic());
                }
            }
            break;

        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }

    return CDialogEx::WindowProc(message, wParam, lParam);
}
