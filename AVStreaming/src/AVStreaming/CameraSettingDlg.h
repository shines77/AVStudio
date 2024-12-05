//
// CCameraSettingDlg.h : ͷ�ļ�
//

#pragma once

#include <AfxWin.h>
#include <AfxDialogEx.h>

#include "DShowCapture.h"

// CVideoSettingDlg �Ի���

class CCameraSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CCameraSettingDlg)

public:
	CCameraSettingDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CCameraSettingDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CAMERA_SETTING_DLG };
#endif

    BOOL Create(UINT nIDTemplate, HWND previewHwnd, CWnd* pParentWnd = NULL);

    HWND GetPreviewHwnd() const;
    HWND SetPreviewHwnd(HWND hwndPreview, bool bAttachTo = false);

protected:
    virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
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
