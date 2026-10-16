// CMainDlg.cpp : implementation file
//

#include "pch.h"
#include "tuan2.h"
#include "CMainDlg.h"
#include "afxdialogex.h"
#include <richedit.h>
#include "FakeData.h"


// CMainDlg dialog

IMPLEMENT_DYNAMIC(CMainDlg, CDialogEx)

CMainDlg::CMainDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MAIN, pParent)
{

}

CMainDlg::~CMainDlg()
{
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_CONTACTS, m_treeContacts);
	DDX_Control(pDX, IDC_EDIT_MSG, m_editMsg);
	DDX_Control(pDX, IDC_STATIC_CHATNAME, m_staticChatName);
	DDX_Control(pDX, IDC_RICH_CHAT, m_richChat);
}


BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_CONTACTS, &CMainDlg::OnTvnSelchangedTreeContacts)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_CONTACTS, &CMainDlg::OnTvnSelchangedTreeContacts)
	ON_BN_CLICKED(IDC_BTN_SEND, &CMainDlg::OnBnClickedBtnSend)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_CHAT, &CMainDlg::OnLvnItemchangedListChat)
END_MESSAGE_MAP()


// CMainDlg message handlers


void CMainDlg::OnTvnSelchangedTreeContacts(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	HTREEITEM hSelected = pNMTreeView->itemNew.hItem;

	if (m_treeContacts.GetParentItem(hSelected) != NULL)
	{
		m_strCurrent = m_treeContacts.GetItemText(hSelected);
		m_staticChatName.SetWindowText(m_strCurrent);

		m_richChat.SetWindowText(L"");   // xóa nội dung cũ

		AppendChat(m_strCurrent, L"Chào bạn!", FALSE);
		AppendChat(L"Tôi", L"Chào, khỏe không?", TRUE);
		AppendChat(m_strCurrent, L"Khỏe, cảm ơn bạn!", FALSE);
	}


	*pResult = 0;
}


void CMainDlg::OnBnClickedBtnSend()
{
	// TODO: Add your control notification handler code here
	CString msg;
	m_editMsg.GetWindowText(msg);

	if (msg.IsEmpty())
		return;

	AppendChat(L"Tôi", msg, TRUE);
	m_editMsg.SetWindowText(L"");
	m_editMsg.SetFocus();
}


void CMainDlg::OnLvnItemchangedListChat(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}


BOOL CMainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here

	HTREEITEM hFriends = m_treeContacts.InsertItem(L"Bạn bè", TVI_ROOT);
	HTREEITEM hGroups = m_treeContacts.InsertItem(L"Nhóm", TVI_ROOT);

	m_treeContacts.InsertItem(L"Minh", hFriends);
	m_treeContacts.InsertItem(L"Lan", hFriends);
	m_treeContacts.InsertItem(L"Hùng", hFriends);
	m_treeContacts.InsertItem(L"Nhóm lớp", hGroups);

	m_treeContacts.Expand(hFriends, TVE_EXPAND);
	m_treeContacts.Expand(hGroups, TVE_EXPAND);

	m_staticChatName.SetWindowText(L"Chọn một liên hệ để bắt đầu chat");

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}
void CMainDlg::AppendChat(const CString& who, const CString& text, BOOL isMe)
{
    int nStart = m_richChat.GetWindowTextLength();
    m_richChat.SetSel(nStart, nStart);

    PARAFORMAT2 pf;
    ZeroMemory(&pf, sizeof(pf));
    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_ALIGNMENT;
    pf.wAlignment = isMe ? PFA_RIGHT : PFA_LEFT;
    m_richChat.SetParaFormat(pf);

    CHARFORMAT2 cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = isMe ? RGB(0, 102, 204) : RGB(0, 0, 0);
    m_richChat.SetSelectionCharFormat(cf);

    CString line = who + L": " + text + L"\n";
    m_richChat.ReplaceSel(line);

    CHARFORMAT2 cfTime;
    ZeroMemory(&cfTime, sizeof(cfTime));
    cfTime.cbSize = sizeof(cfTime);
    cfTime.dwMask = CFM_COLOR | CFM_SIZE;
    cfTime.crTextColor = RGB(150, 150, 150);
    cfTime.yHeight = 160;
    m_richChat.SetSelectionCharFormat(cfTime);

    CString strTime = CTime::GetCurrentTime().Format(L"%H:%M") + L"\n\n";
    m_richChat.ReplaceSel(strTime);

    int nEnd = m_richChat.GetWindowTextLength();
    m_richChat.SetSel(nEnd, nEnd);
}