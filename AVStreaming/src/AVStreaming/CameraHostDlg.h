//
// CCameraHostDlg.h : 头文件
//

#pragma once

#include <AfxWin.h>
#include <AfxDialogEx.h>

#include <stdint.h>
#include <string>

#include "string_utils.h"

// CCameraHostDlg 对话框

class CameraCapture;

class CCameraHostDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CCameraHostDlg)

public:
	CCameraHostDlg(CWnd * pParent = NULL);   // 标准构造函数
	virtual ~CCameraHostDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CAMERA_HOST_DLG };
#endif

    BOOL Create(UINT nIDTemplate, HWND previewHwnd, CWnd * pParentWnd = NULL);

    HWND GetPreviewHwnd() const;
    HWND SetPreviewHwnd(HWND hwndPreview, bool bAttachTo = false);

    CameraCapture * GetSafeCapture() const;

    bool StartCapture();
    bool StopCapture();

    std::tstring GetSelectedVideoDevice() const;
    std::tstring GetSelectedAudioDevice() const;

protected:
    virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange * pDX);    // DDX/DDV 支持
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnCbnSelChangeVideoDeviceList();
    afx_msg void OnCbnSelChangeAudioDeviceList();
    virtual BOOL PreTranslateMessage(MSG * pMsg);
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    afx_msg void OnClickedBtnStartCapture();
    afx_msg void OnClickedBtnStopCapture();
	DECLARE_MESSAGE_MAP()

protected:
    size_t EnumVideoDeviceList();
    size_t EnumAudioDeviceList();

private:
    HWND            hwndPreview_;
    CameraCapture * pCameraCapture_;

    bool            inited_;
    std::tstring    selectedVideoDevice_;
    std::tstring    selectedAudioDevice_;

    CComboBox       cbxVideoDeviceList_;
    CComboBox       cbxAudioDeviceList_;       
    CButton         cbxSaveToAviFile_;
};
