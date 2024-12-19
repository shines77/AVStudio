//
// PreviewWnd.cpp : 实现文件
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
    // 调用基类的 CreateEx 方法来创建窗口
    return CWnd::CreateEx(0,                    // 扩展样式
                    AfxRegisterWndClass(CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW),  // 窗口类名
                    lpszWindowName,             // 窗口标题
                    dwStyle,                    // 窗口样式
                    rect,                       // 窗口位置和大小
                    pParentWnd,                 // 父窗口
                    nID,                        // 控件ID
                    NULL);                      // 创建参数
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

// CPreviewWnd 消息处理程序

int CPreviewWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    return 0;
}

void CPreviewWnd::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    // TODO:
}

void CPreviewWnd::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
    CWnd::OnWindowPosChanged(lpwndpos);

    // TODO:
}

LRESULT CPreviewWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    CCameraCapture * pCameraCapture = NULL;
    switch (message)
    {
        case WM_GRAPH_NOTIFY:
            pCameraCapture = GetSafeCapture();
            if (pCameraCapture != NULL) {
                pCameraCapture->HandleGraphEvent();
            }
            break;

        case WM_SIZE:
            pCameraCapture = GetSafeCapture();
            if (pCameraCapture != NULL) {
                pCameraCapture->ResizeVideoWindow();
            }
            break;

        case WM_WINDOWPOSCHANGED:
            pCameraCapture = GetSafeCapture();
            if (pCameraCapture != NULL) {
                pCameraCapture->WindowStateChange(!this->IsIconic());
            }
            break;
    }

    // Pass this message to the video window for notification of system changes
    pCameraCapture = GetSafeCapture();
    if (pCameraCapture != NULL) {
        IVideoWindow * pVideoWindow = pCameraCapture->GetVideoWindow();
        if (pVideoWindow != NULL) {
            pVideoWindow->NotifyOwnerMessage((LONG_PTR)this->GetSafeHwnd(), message, wParam, lParam);
        }
    }

    return CWnd::WindowProc(message, wParam, lParam);
}
