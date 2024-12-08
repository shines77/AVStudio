//
// PreviewWnd.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "AVStreaming.h"
#include "PreviewWnd.h"
#include "CameraHostDlg.h"
#include "CameraCapture.h"

// CPreviewWnd

IMPLEMENT_DYNAMIC(CPreviewWnd, CWnd)

CPreviewWnd::CPreviewWnd()
{
    pCameraHostDlg_ = NULL;
}

CPreviewWnd::~CPreviewWnd()
{
    pCameraHostDlg_ = NULL;
}

BOOL CPreviewWnd::Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT & rect,
                         CWnd * pParentWnd, UINT nID)
{
    // ���û���� CreateEx ��������������
    return CWnd::CreateEx(0,                    // ��չ��ʽ
                    AfxRegisterWndClass(CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW),  // ��������
                    lpszWindowName,             // ���ڱ���
                    dwStyle,                    // ������ʽ
                    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, // ����λ�úʹ�С
                    pParentWnd->GetSafeHwnd(),  // �����ھ��
                    (HMENU)nID,                 // �ؼ�ID
                    NULL);                      // ��������
}

CCameraHostDlg * CPreviewWnd::GetSafeHostWnd() const {
    if (pCameraHostDlg_ != NULL && pCameraHostDlg_->GetPreviewHwnd() != NULL)
        return pCameraHostDlg_;
    else
        return NULL;
}

CCameraCapture * CPreviewWnd::GetSafeCapture() const {
    if (pCameraHostDlg_ != NULL && pCameraHostDlg_->GetSafeCapture())
        return pCameraHostDlg_->GetSafeCapture();
    else
        return NULL;
}

BEGIN_MESSAGE_MAP(CPreviewWnd, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()

// CPreviewWnd ��Ϣ�������

int CPreviewWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    return 0;
}

void CPreviewWnd::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    CCameraCapture * pCameraCapture = GetSafeCapture();
    if (pCameraCapture != NULL) {
        pCameraCapture->ResizeVideoWindow();
    }
}

void CPreviewWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
    CWnd::OnWindowPosChanged(lpwndpos);

    CCameraCapture * pCameraCapture = GetSafeCapture();
    if (pCameraCapture != NULL) {
        pCameraCapture->WindowStateChange(!this->IsIconic());
    }
}
