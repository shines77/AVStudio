//
// AVStreamingDlg.h : ͷ�ļ�
//

#pragma once

#include <AfxWin.h>
#include <AfxDialogEx.h>

class CPreviewWnd;
class CCameraHostDlg;

// CAVStreamingDlg �Ի���
class CAVStreamingDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CAVStreamingDlg)

    // ����
public:
	CAVStreamingDlg(CWnd * pParent = NULL);
    ~CAVStreamingDlg();

    // �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AVSTREAMING_DIALOG };
#endif

    static const int kDefaultVideoPreviewWidth = 640;
    static const int kDefaultVideoPreviewHeight = 480;

    // ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��
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
