//
// AVStreamingDlg.h : 头文件
//

#pragma once

#include "DShowCapture.h"
#include "afxwin.h"

// CAVStreamingDlg 对话框
class CAVStreamingDlg : public CDialogEx
{
// 构造
public:
	CAVStreamingDlg(CWnd * pParent = NULL);
    ~CAVStreamingDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AVSTREAMING_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnCbnSelChangeVideoDeviceList();
	DECLARE_MESSAGE_MAP()

    int EnumVideoDeviceList();
    int EnumAudioDeviceList();

private:
    DShowCapture * camera_;

    CStatic     m_wndCameraPreview;
    CComboBox   m_cbxVideoDeviceList;
    CComboBox   m_cbxAudioDeviceList;
};
