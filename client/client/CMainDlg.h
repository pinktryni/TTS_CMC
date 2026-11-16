#pragma once

#include <map>
#include <vector>
#include "ChatClient.h"

#define IDLOGOUT   100

struct ChatLine {
	CString   who;
	CString   text;
	BOOL      isMe;
	long long time;
};

class CMainDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CMainDlg)

public:
	CMainDlg(CWnd* pParent = nullptr);
	virtual ~CMainDlg();

	CString m_strCurrent;

	void AppendChat(const CString& who, const CString& text, BOOL isMe, long long serverTime);
	void AddLineToHistory(const CString& contact, const CString& who, const CString& text, BOOL isMe, long long serverTime);
	void RenderCurrentContact();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAIN };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	CStatic       m_staticChatName;
	CTreeCtrl     m_treeContacts;
	CEdit         m_editMsg;
	CRichEditCtrl m_richChat;

	std::map<CString, std::vector<ChatLine>> m_chatHistory;

	afx_msg void OnTvnSelchangedTreeContacts(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnSend();
	afx_msg void OnBnClickedBtnLogout();
	afx_msg void OnLvnItemchangedListChat(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnChatMessagePush(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnChatPendingList(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnChatDisconnected(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDestroy();
	std::map<CString, bool> m_historyLoaded; 
	afx_msg LRESULT OnChatHistoryResult(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBtnSettings();
	CStatic m_staticChatAvatar;
	void UpdateChatAvatar();
	CStatic m_staticMyAvatar;
	void UpdateMyAvatar();
	CStatic m_staticStatus;
	void UpdateStatusUI();
	afx_msg LRESULT OnChatStatusPush(WPARAM wParam, LPARAM lParam);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
private:
	HTREEITEM m_hUsersRoot = nullptr;
	std::map<CString, CString> m_mapFullToUser;
	HTREEITEM EnsureContactInTree(const CString& username);
	void      RebuildContactTree();
};