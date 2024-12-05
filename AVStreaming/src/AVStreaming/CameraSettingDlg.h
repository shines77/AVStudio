//
// CCameraSettingDlg.h : 头文件
//

#pragma once

#include <AfxWin.h>
#include <AfxDialogEx.h>

#include "DShowCapture.h"

// CVideoSettingDlg 对话框

class CCameraSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CCameraSettingDlg)

public:
	CCameraSettingDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CCameraSettingDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CAMERA_SETTING_DLG };
#endif

    BOOL Create(UINT nIDTemplate, HWND previewHwnd, CWnd* pParentWnd = NULL);

    HWND GetPreviewHwnd() const;
    HWND SetPreviewHwnd(HWND hwndPreview, bool bAttachTo = false);

protected:
    virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnCbnSelChangeVideoDeviceList();
    afx_msg void OnCbnSelChangeAudioDeviceList();
	DECLARE_MESSAGE_MAP()

protected:
    int EnumVideoDeviceList();
    int EnumAudioDeviceList();

private:
    HWND            hwndPreview_;
    DShowCapture *  pDShowCapture_;

    CComboBox   cbxVideoDeviceList_;
    CComboBox   cbxAudioDeviceList_;    
};
