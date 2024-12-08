//
// AVStreamingDlg.h : 头文件
//

#pragma once

#include <AfxWin.h>
#include <AfxDialogEx.h>

class CPreviewWnd;
class CCameraHostDlg;

// CAVStreamingDlg 对话框
class CAVStreamingDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CAVStreamingDlg)

    // 构造
public:
	CAVStreamingDlg(CWnd * pParent = NULL);
    ~CAVStreamingDlg();

    // 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AVSTREAMING_DIALOG };
#endif

    static const int kDefaultVideoPreviewWidth = 640;
    static const int kDefaultVideoPreviewHeight = 480;

    // 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

    void SetScale(double scaleX, double scaleY);

private:
    CPreviewWnd *       pPreviewWnd_;
    CCameraHostDlg *    pCameraHostDlg_;
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
};
