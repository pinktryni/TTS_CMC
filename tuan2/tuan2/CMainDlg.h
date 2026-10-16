#pragma once


// CMainDlg dialog

class CMainDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CMainDlg)

public:
	CMainDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CMainDlg();
	CString m_strCurrent;
	void AppendChat(const CString& who, const CString& text, BOOL isMe);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CStatic m_staticChatName;
	CTreeCtrl m_treeContacts;
	CEdit m_editMsg;
	afx_msg void OnTvnSelchangedTreeContacts(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnSend();
	afx_msg void OnLvnItemchangedListChat(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	CRichEditCtrl m_richChat;
};
