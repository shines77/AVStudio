//
// PreviewWnd.cpp : 实现文件
//

#include "stdafx.h"
#include "AVStreaming.h"
#include "PreviewWnd.h"

// CPreviewWnd

IMPLEMENT_DYNAMIC(CPreviewWnd, CWnd)

CPreviewWnd::CPreviewWnd()
{
    //
}

CPreviewWnd::~CPreviewWnd()
{
    //
}

BOOL CPreviewWnd::Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT & rect,
                         CWnd * pParentWnd, UINT nID)
{
    // 调用基类的 CreateEx 方法来创建窗口
    return CWnd::CreateEx(0,                    // 扩展样式
                    AfxRegisterWndClass(CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW),  // 窗口类名
                    lpszWindowName,             // 窗口标题
                    dwStyle,                    // 窗口样式
                    rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, // 窗口位置和大小
                    pParentWnd->GetSafeHwnd(),  // 父窗口句柄
                    (HMENU)nID,                 // 控件ID
                    NULL);                      // 创建参数
}

BEGIN_MESSAGE_MAP(CPreviewWnd, CWnd)
    ON_WM_CREATE()
END_MESSAGE_MAP()

// CPreviewWnd 消息处理程序

int CPreviewWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    return 0;
}
