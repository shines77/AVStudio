#pragma once

// CPreviewWnd

class CPreviewWnd : public CWnd
{
	DECLARE_DYNAMIC(CPreviewWnd)

public:
	CPreviewWnd();
	virtual ~CPreviewWnd();

    BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT & rect,
                CWnd* pParentWnd, UINT nID);

protected:
	DECLARE_MESSAGE_MAP()
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

private:
    //
};
