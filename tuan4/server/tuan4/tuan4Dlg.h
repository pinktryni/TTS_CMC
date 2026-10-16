
// tuan4Dlg.h : header file
//

#pragma once


// Ctuan4Dlg dialog
class Ctuan4Dlg : public CDialogEx
{
// Construction
public:
	Ctuan4Dlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TUAN4_DIALOG };
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
	CTreeCtrl m_tree;
	CStatic m_stName;
	CRichEditCtrl m_richChat;
	CString m_strMsg;
	int m_nPort;
	afx_msg void OnBnClickedBtnListen();
	afx_msg void OnBnClickedBtnSend();
	afx_msg void OnDestroy();
	virtual void OnOK();
public:
	SOCKET m_listen = INVALID_SOCKET;
	SOCKET m_client = INVALID_SOCKET;
	CFont  m_fontName;                          
	static UINT ServerThread(LPVOID pParam);
	void SetPeer(const CStringW& peer);       
protected:
	afx_msg LRESULT OnChatRecv(WPARAM, LPARAM);
	afx_msg LRESULT OnChatStatus(WPARAM, LPARAM);
	afx_msg LRESULT OnChatPeer(WPARAM, LPARAM);
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
