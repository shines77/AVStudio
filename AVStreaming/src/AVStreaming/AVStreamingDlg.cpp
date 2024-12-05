//
// AVStreamingDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "AVStreaming.h"
#include "AVStreamingDlg.h"

#include "utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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

// CAVStreamingDlg �Ի���

IMPLEMENT_DYNAMIC(CAVStreamingDlg, CDialogEx)

CAVStreamingDlg::CAVStreamingDlg(CWnd* pParent /* = NULL */)
	: CDialogEx(IDD_AVSTREAMING_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    pCameraSettingDlg_ = NULL;
    pPreviewWnd_ = NULL;
}

CAVStreamingDlg::~CAVStreamingDlg()
{
    SAFE_OBJECT_DELETE(pCameraSettingDlg_);
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

// CAVStreamingDlg ��Ϣ��������

void CAVStreamingDlg::SetScale(double scaleX, double scaleY)
{
    // ��ȡ�Ի���Ĵ��ھ���
    CRect rect;
    GetWindowRect(&rect);

    // ���ŶԻ���Ĵ�С
    rect.right = rect.left + (int)(rect.Width() * scaleX);
    rect.bottom = rect.top + (int)(rect.Height() * scaleY);
    MoveWindow(&rect);

    // ���ŶԻ����еĿؼ�
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

	// ��������...���˵������ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

    // ��ȡ��Ļ�� DPI
    HDC hdc = ::GetDC(NULL);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
    ::ReleaseDC(NULL, hdc);

    // �������ű���
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

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	// ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

    if (pPreviewWnd_ == NULL) {
        CRect rcWnd(10, 10, 10 + kDefaultVideoPreviewWidth, 10 + kDefaultVideoPreviewHeight);
        pPreviewWnd_ = new CPreviewWnd;
        pPreviewWnd_->Create(_T("����ͷԤ������"),
                             WS_CHILD | WS_CLIPCHILDREN, rcWnd,
                             this, IDC_CAMERA_PREVIEW);
        pPreviewWnd_->ShowWindow(SW_SHOWNORMAL);

        CRect rcClient;
        pPreviewWnd_->GetClientRect(rcClient);

        CString text;
        text.Format(_T("r: %ul, l: %ul, width: %d, height: %d\n"), rcClient.left, rcClient.left, rcClient.Width(), rcClient.Height());
        ::OutputDebugString(text.GetBuffer());
    }

    if (pCameraSettingDlg_ == NULL) {
        pCameraSettingDlg_ = new CCameraSettingDlg;
        if (pPreviewWnd_ != NULL && pPreviewWnd_->GetSafeHwnd())
            pCameraSettingDlg_->Create(IDD_CAMERA_SETTING_DLG, pPreviewWnd_->GetSafeHwnd(), this);
        else
            pCameraSettingDlg_->Create(IDD_CAMERA_SETTING_DLG, NULL, this);
        
        pCameraSettingDlg_->ShowWindow(SW_SHOWNORMAL);

        CRect rcClient;
        pCameraSettingDlg_->GetClientRect(rcClient);
        pCameraSettingDlg_->MoveWindow(10 + kDefaultVideoPreviewWidth + 10, 10,
                                       rcClient.Width(), rcClient.Height(), TRUE);
    }

    // ���öԻ�������ű���
    SetScale(scaleX, scaleY);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի���������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CAVStreamingDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
HCURSOR CAVStreamingDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}