// CMainDlg.cpp : implementation file
//

#include "pch.h"
#include "tuan2.h"
#include "CMainDlg.h"
#include "afxdialogex.h"
#include <richedit.h>
#include <algorithm>
#include <ctime>
#include "CSettingsDlg.h"
#include <atlimage.h>
#include "Utils.h"

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
	DDX_Control(pDX, IDC_STATIC_CHAT_AVATAR, m_staticChatAvatar);
	DDX_Control(pDX, IDC_STATIC_STATUS, m_staticStatus);
	DDX_Control(pDX, IDC_STATIC_MY_AVATAR, m_staticMyAvatar);
}


BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_CONTACTS, &CMainDlg::OnTvnSelchangedTreeContacts)
	ON_BN_CLICKED(IDC_BTN_SEND, &CMainDlg::OnBnClickedBtnSend)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_CHAT, &CMainDlg::OnLvnItemchangedListChat)
	ON_MESSAGE(WM_CHAT_MESSAGE_PUSH, &CMainDlg::OnChatMessagePush)
	ON_MESSAGE(WM_CHAT_PENDING_LIST, &CMainDlg::OnChatPendingList)
	ON_MESSAGE(WM_CHAT_DISCONNECTED, &CMainDlg::OnChatDisconnected)
	ON_MESSAGE(WM_CHAT_HISTORY_RESULT, &CMainDlg::OnChatHistoryResult)
	ON_BN_CLICKED(IDC_BTN_SETTINGS, &CMainDlg::OnBnClickedBtnSettings)
	ON_MESSAGE(WM_CHAT_STATUS_PUSH, &CMainDlg::OnChatStatusPush) // Thêm dòng này
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CMainDlg message handlers

BOOL CMainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	std::wstring me = g_chatClient.GetCurrentUsername();
	CString strLoggedIn;


	CString strTitle;
	strTitle.Format(L"Chat - %s", me.c_str());
	SetWindowText(strTitle);

	m_editMsg.ModifyStyle(WS_HSCROLL, 0);               
	m_editMsg.ModifyStyle(0, ES_MULTILINE | ES_AUTOVSCROLL);

	m_richChat.ModifyStyle(WS_HSCROLL, 0);              
	m_richChat.ModifyStyle(0, ES_MULTILINE | ES_AUTOVSCROLL);

	RebuildContactTree();
	m_staticChatAvatar.ModifyStyle(0, SS_BITMAP);

	UpdateMyAvatar();

	g_chatClient.StartListening(GetSafeHwnd());
	g_chatClient.RequestPendingMessages();

	m_editMsg.SetFocus();
	return TRUE;
}

static CString FormatMsgTime(long long epoch) {
	if (epoch <= 0) epoch = (long long)time(nullptr);
	time_t t = (time_t)epoch;
	struct tm local;
	localtime_s(&local, &t);

	time_t now = time(nullptr);
	struct tm nowLocal;
	localtime_s(&nowLocal, &now);

	CString s;
	if (local.tm_year == nowLocal.tm_year && local.tm_yday == nowLocal.tm_yday) {
		s.Format(L"%02d:%02d", local.tm_hour, local.tm_min);
	}
	else {
		s.Format(L"%02d/%02d %02d:%02d",
			local.tm_mday, local.tm_mon + 1,
			local.tm_hour, local.tm_min);
	}
	return s;
}

void CMainDlg::RebuildContactTree()
{
	m_treeContacts.DeleteAllItems();
	m_strCurrent.Empty();
	m_hUsersRoot = m_treeContacts.InsertItem(L"Người dùng", TVI_ROOT);

	std::wstring me = g_chatClient.GetCurrentUsername();
	for (const auto& u : g_chatClient.GetCachedUsers()) {
		if (u == me) continue;

		std::wstring fullname = g_chatClient.GetFullName(u);
		HTREEITEM hItem = m_treeContacts.InsertItem(fullname.c_str(), m_hUsersRoot);

		m_treeContacts.SetItemData(hItem, (DWORD_PTR)new CString(u.c_str()));
	}
	m_treeContacts.Expand(m_hUsersRoot, TVE_EXPAND);
}
HTREEITEM CMainDlg::EnsureContactInTree(const CString& username)
{
	if (!m_hUsersRoot) {
		m_hUsersRoot = m_treeContacts.InsertItem(L"Người dùng", TVI_ROOT);
		m_treeContacts.Expand(m_hUsersRoot, TVE_EXPAND);
	}

	// Tìm xem user này đã có trong cây chưa (dựa vào ItemData là username)
	HTREEITEM hChild = m_treeContacts.GetChildItem(m_hUsersRoot);
	while (hChild) {
		CString* pUser = reinterpret_cast<CString*>(m_treeContacts.GetItemData(hChild));
		if (pUser && *pUser == username)
			return hChild;
		hChild = m_treeContacts.GetNextSiblingItem(hChild);
	}

	// Nếu chưa có, thêm mới
	std::wstring wUsername(username.GetString());
	std::wstring wFullname = g_chatClient.GetFullName(wUsername);
	CString displayName(wFullname.c_str());

	HTREEITEM hNew = m_treeContacts.InsertItem(displayName, m_hUsersRoot);
	m_treeContacts.SetItemData(hNew, (DWORD_PTR)new CString(username));
	m_treeContacts.Expand(m_hUsersRoot, TVE_EXPAND);
	return hNew;
}

void CMainDlg::OnTvnSelchangedTreeContacts(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	HTREEITEM hSelected = pNMTreeView->itemNew.hItem;

	if (hSelected && m_treeContacts.GetParentItem(hSelected) != nullptr) {
		CString fullname = m_treeContacts.GetItemText(hSelected);
		m_staticChatName.SetWindowText(fullname);

		// Lấy Username từ ItemData thay vì dùng m_mapFullToUser
		CString* pUsername = reinterpret_cast<CString*>(m_treeContacts.GetItemData(hSelected));
		if (pUsername) {
			m_strCurrent = *pUsername;
		}
		else {
			m_strCurrent = fullname; // Dự phòng
		}

		// Cập nhật ảnh đại diện
		UpdateChatAvatar();
		UpdateStatusUI();

		RenderCurrentContact();
		if (!m_historyLoaded[m_strCurrent]) {
			g_chatClient.RequestChatHistory(std::wstring(m_strCurrent.GetString()));
		}
	}
	else {
		m_strCurrent.Empty();
		m_staticChatName.SetWindowText(L"");
		m_richChat.SetWindowText(L"");
		UpdateChatAvatar();
		UpdateStatusUI();
	}
	*pResult = 0;
}

void CMainDlg::OnBnClickedBtnSend()
{
	CString msg;
	m_editMsg.GetWindowText(msg);
	if (msg.IsEmpty()) return;

	if (m_strCurrent.IsEmpty()) {
		AfxMessageBox(L"Hãy chọn một liên hệ trước khi gửi tin nhắn.");
		return;
	}
	if (m_strCurrent == CString(g_chatClient.GetCurrentUsername().c_str())) {
		AfxMessageBox(L"Không thể tự gửi tin nhắn cho chính mình.");
		return;
	}

	std::wstring to(m_strCurrent.GetString());
	std::wstring content(msg.GetString());
	if (!g_chatClient.SendChatMessage(to, content)) {
		AfxMessageBox(L"Gửi tin nhắn thất bại (mất kết nối tới server?).");
		return;
	}

	long long now = (long long)time(nullptr);
	AddLineToHistory(m_strCurrent, L"Tôi", msg, TRUE, now);
	AppendChat(L"Tôi", msg, TRUE, now);

	m_editMsg.SetWindowText(L"");
	m_editMsg.SetFocus();
}

void CMainDlg::OnBnClickedBtnSettings()
{
	CSettingsDlg dlg(this);
	if (dlg.DoModal() == IDLOGOUT)
	{
		g_chatClient.Disconnect(); 
		EndDialog(IDLOGOUT);       
	}
	else
	{
		UpdateMyAvatar();
		UpdateChatAvatar();
	}
}

void CMainDlg::OnLvnItemchangedListChat(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
}

LRESULT CMainDlg::OnChatMessagePush(WPARAM wParam, LPARAM lParam)
{
	ChatPushData* data = reinterpret_cast<ChatPushData*>(lParam);
	OutputDebugString(L"OnChatMessagePush called\n");
	if (!data) return 0;
	if (!::IsWindow(GetSafeHwnd()) || !::IsWindow(m_treeContacts.GetSafeHwnd())) {
		delete data;
		return 0;
	}
	CString from(data->from.c_str());
	CString content(data->content.c_str());

	EnsureContactInTree(from);
	AddLineToHistory(from, from, content, FALSE, data->time);

	if (m_strCurrent == from)
		AppendChat(from, content, FALSE, data->time);

	delete data;
	return 0;
}

LRESULT CMainDlg::OnChatPendingList(WPARAM wParam, LPARAM lParam)
{
	auto* list = reinterpret_cast<std::vector<ChatPushData>*>(lParam);
	OutputDebugString(L"OnChatPendingList called\n");
	if (!list) return 0;
	if (!::IsWindow(GetSafeHwnd()) || !::IsWindow(m_treeContacts.GetSafeHwnd())) {
		delete list;
		return 0;
	}
	bool currentChanged = false;
	for (const ChatPushData& item : *list) {
		CString from(item.from.c_str());
		CString content(item.content.c_str());
		EnsureContactInTree(from);
		AddLineToHistory(from, from, content, FALSE, item.time);
		if (m_strCurrent == from) currentChanged = true;
	}
	if (currentChanged) RenderCurrentContact();

	delete list;
	return 0;
}

LRESULT CMainDlg::OnChatDisconnected(WPARAM wParam, LPARAM lParam)
{
	AfxMessageBox(L"Mất kết nối tới server. Vui lòng khởi động lại ứng dụng.");
	return 0;
}

void CMainDlg::OnDestroy()
{
	g_chatClient.Disconnect();
	CDialogEx::OnDestroy();
}

void CMainDlg::AddLineToHistory(const CString& contact, const CString& who, const CString& text, BOOL isMe, long long serverTime)
{
	ChatLine line;
	line.who = who;
	line.text = text;
	line.isMe = isMe;
	line.time = serverTime > 0 ? serverTime : (long long)time(nullptr);
	m_chatHistory[contact].push_back(line);
}

void CMainDlg::RenderCurrentContact()
{
	m_richChat.SetWindowText(L"");
	auto it = m_chatHistory.find(m_strCurrent);
	if (it == m_chatHistory.end()) return;

	auto& lines = it->second;
	std::sort(lines.begin(), lines.end(),
		[](const ChatLine& a, const ChatLine& b) { return a.time < b.time; });

	for (const ChatLine& line : lines)
		AppendChat(line.who, line.text, line.isMe, line.time);
}

void CMainDlg::AppendChat(const CString& who, const CString& text, BOOL isMe, long long serverTime)
{
	int nStart = m_richChat.GetWindowTextLength();
	m_richChat.SetSel(nStart, nStart);

	PARAFORMAT2 pf; ZeroMemory(&pf, sizeof(pf));
	pf.cbSize = sizeof(pf);
	pf.dwMask = PFM_ALIGNMENT;
	pf.wAlignment = isMe ? PFA_RIGHT : PFA_LEFT;
	m_richChat.SetParaFormat(pf);

	CHARFORMAT2 cf; ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = isMe ? RGB(0, 102, 204) : RGB(0, 0, 0);
	m_richChat.SetSelectionCharFormat(cf);

	CString senderLine = who + L":\r\n";
	m_richChat.ReplaceSel(senderLine);
	CString contentLine = text + L"\r\n";
	m_richChat.ReplaceSel(contentLine);

	CHARFORMAT2 cfTime; ZeroMemory(&cfTime, sizeof(cfTime));
	cfTime.cbSize = sizeof(cfTime);
	cfTime.dwMask = CFM_COLOR | CFM_SIZE;
	cfTime.crTextColor = RGB(150, 150, 150);
	cfTime.yHeight = 160;
	m_richChat.SetSelectionCharFormat(cfTime);

	CString strTime = FormatMsgTime(serverTime) + L"\n\n";
	m_richChat.ReplaceSel(strTime);

	int nEnd = m_richChat.GetWindowTextLength();
	m_richChat.SetSel(nEnd, nEnd);

	m_richChat.SetSel(-1, -1); // Đưa con trỏ text về cuối
	m_richChat.SendMessage(WM_VSCROLL, SB_BOTTOM, 0);
}

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		if (::GetFocus() == m_editMsg.GetSafeHwnd()) {
			if (::GetKeyState(VK_SHIFT) & 0x8000) {
				// Nếu bấm Shift + Enter: Chèn ký tự xuống dòng trực tiếp vào vị trí con trỏ
				m_editMsg.ReplaceSel(L"\r\n");
				return TRUE; // Báo hiệu đã xử lý xong phím này
			}
			// Nếu chỉ bấm Enter: Gửi tin nhắn
			OnBnClickedBtnSend();
			return TRUE;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

LRESULT CMainDlg::OnChatHistoryResult(WPARAM wParam, LPARAM lParam)
{
	auto* hist = reinterpret_cast<ChatHistoryData*>(lParam);
	if (!hist) return 0;
	if (!::IsWindow(GetSafeHwnd()) || !::IsWindow(m_richChat.GetSafeHwnd())) {
		delete hist;
		return 0;
	}
	CString withUser(hist->withUser.c_str());
	auto& existingHistory = m_chatHistory[withUser];

	for (const ChatPushData& item : hist->messages) {
		bool isMe = (item.from == g_chatClient.GetCurrentUsername());
		CString displayWho = isMe ? L"Tôi" : CString(item.from.c_str());
		
		bool exists = false;
		for (const auto& old : existingHistory) {
			if (old.time == item.time && old.text == CString(item.content.c_str()) && (bool)old.isMe == isMe) {
				exists = true;
				break;
			}
		}
		if (!exists) {
			ChatLine line;
			line.who = displayWho;
			line.text = CString(item.content.c_str());
			line.isMe = isMe;
			line.time = item.time;
			existingHistory.push_back(line);
		}
	}

	m_historyLoaded[withUser] = true;

	if (m_strCurrent == withUser) {
		RenderCurrentContact();
	}

	delete hist;
	return 0;
}

void CMainDlg::UpdateMyAvatar()
{
	if (!::IsWindow(m_staticMyAvatar.GetSafeHwnd())) return;

	std::wstring me = g_chatClient.GetCurrentUsername();
	std::vector<BYTE> avatarBytes = g_chatClient.GetAvatar(me);

	if (!avatarBytes.empty()) {
		// Lấy kích thước thực tế của khung trên giao diện
		CRect rect;
		m_staticMyAvatar.GetClientRect(&rect);

		HBITMAP hBmp = BytesToHBITMAP(avatarBytes, rect.Width(), rect.Height());
		SafeSetBitmap(m_staticMyAvatar, hBmp);
	}
	else {
		SafeSetBitmap(m_staticMyAvatar, NULL);
	}
}

void CMainDlg::UpdateChatAvatar()
{
	if (m_strCurrent.IsEmpty() || !::IsWindow(m_staticChatAvatar.GetSafeHwnd())) {
		SafeSetBitmap(m_staticChatAvatar, NULL);
		return;
	}

	std::wstring targetUser(m_strCurrent.GetString());
	std::vector<BYTE> avatarBytes = g_chatClient.GetAvatar(targetUser);

	if (!avatarBytes.empty()) {
		// Lấy kích thước thực tế của khung trên giao diện
		CRect rect;
		m_staticChatAvatar.GetClientRect(&rect);
		int w = rect.Width();
		int h = rect.Height();

		HBITMAP hBmp = BytesToHBITMAP(avatarBytes, w, h);
		SafeSetBitmap(m_staticChatAvatar, hBmp);
	}
	else {
		SafeSetBitmap(m_staticChatAvatar, NULL);
	}
}

void CMainDlg::UpdateStatusUI()
{
	if (m_strCurrent.IsEmpty()) {
		m_staticStatus.SetWindowText(L"");
		return;
	}
	std::wstring targetUser(m_strCurrent.GetString());
	bool isOnline = g_chatClient.IsUserOnline(targetUser);

	m_staticStatus.SetWindowText(isOnline ? L"Online" : L"Offline");
	// Ép control vẽ lại để nhận màu mới từ OnCtlColor
	m_staticStatus.Invalidate();
}

HBRUSH CMainDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetDlgCtrlID() == IDC_STATIC_STATUS) {
		// Đổi màu chữ
		if (m_strCurrent.IsEmpty()) {
			pDC->SetTextColor(RGB(150, 150, 150)); // Xám
		}
		else {
			bool isOnline = g_chatClient.IsUserOnline(std::wstring(m_strCurrent.GetString()));
			pDC->SetTextColor(isOnline ? RGB(0, 153, 0) : RGB(150, 150, 150)); // Xanh lá / Xám
		}
		// Đổi màu nền cho giống màu Dialog
		pDC->SetBkMode(TRANSPARENT);
	}

	return hbr;
}

LRESULT CMainDlg::OnChatStatusPush(WPARAM wParam, LPARAM lParam)
{
	// Cập nhật lại trạng thái nếu người đang chat cùng thay đổi
	UpdateStatusUI();
	return 0;
}