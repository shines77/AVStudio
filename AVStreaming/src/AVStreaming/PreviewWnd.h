#pragma once

// CPreviewWnd

class CCameraHostDlg;
class CCameraCapture;

class CPreviewWnd : public CWnd
{
	DECLARE_DYNAMIC(CPreviewWnd)

public:
	CPreviewWnd();
	virtual ~CPreviewWnd();

    BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle, const RECT & rect,
                CWnd* pParentWnd, UINT nID);

    CCameraHostDlg * GetHostWnd() const {
        return pCameraHostDlg_;
    }
    void SetHostWnd(CCameraHostDlg * pCameraHostDlg) {
        pCameraHostDlg_ = pCameraHostDlg;
    }

    CCameraHostDlg * GetSafeHostWnd() const;
    CCameraCapture * GetSafeCapture() const;

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	DECLARE_MESSAGE_MAP()

private:
    CCameraHostDlg * pCameraHostDlg_;
};
