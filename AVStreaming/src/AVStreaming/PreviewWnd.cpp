//
// PreviewWnd.cpp : ʵ���ļ�
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

BEGIN_MESSAGE_MAP(CPreviewWnd, CWnd)
    ON_WM_CREATE()
END_MESSAGE_MAP()

// CPreviewWnd ��Ϣ�������

int CPreviewWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    return 0;
}
