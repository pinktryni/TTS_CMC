
// tuan4clientDlg.h : header file
//

#pragma once


// Ctuan4clientDlg dialog
class Ctuan4clientDlg : public CDialogEx
{
// Construction
public:
	Ctuan4clientDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TUAN4CLIENT_DIALOG };
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
	CString m_strIP;
	int m_nPort;
	afx_msg void OnDestroy();
	virtual void OnOK();
public:
	SOCKET m_sock = INVALID_SOCKET;
	CFont  m_fontName;
	static UINT ClientThread(LPVOID pParam);
	void SetPeer(const CStringW& peer);
protected:
	afx_msg LRESULT OnChatRecv(WPARAM, LPARAM);
	afx_msg LRESULT OnChatStatus(WPARAM, LPARAM);
	afx_msg LRESULT OnChatPeer(WPARAM, LPARAM);
public:
	afx_msg void OnBnClickedBtnConnect();
	afx_msg void OnBnClickedBtnSend();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	BOOL m_bAutorun;
	afx_msg void OnBnClickedChkAutorun();
};
