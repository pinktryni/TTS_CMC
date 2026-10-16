
// tuan3Dlg.h : header file
//
#include "IpcChat.h"

#pragma once


// Ctuan3Dlg dialog
class Ctuan3Dlg : public CDialogEx
{
// Construction
public:
	Ctuan3Dlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TUAN3_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CString m_strPeer;
	CString m_strMsg;
	CListBox m_lstChat;
	BOOL m_nMode;
	afx_msg void OnClickedBtnSend();
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void OnDestroy();
public:
	CString     m_myUser;
	CPipeServer m_pipe;
	void AppendToChatBox(const CString& s);

protected:
	// phần Class Wizard đã sinh sẵn: OnCopyData, OnDestroy, ...
	afx_msg void OnBnClickedBtnSend();
	afx_msg LRESULT OnPipeMsg(WPARAM wParam, LPARAM lParam);
	virtual void OnOK();
};
