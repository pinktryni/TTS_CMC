
// tuan4clientDlg.cpp : implementation file
//

#include "pch.h"
#include "ChatProtocol.h"
#include "framework.h"
#include "tuan4client.h"
#include "tuan4clientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Ctuan4clientDlg dialog



Ctuan4clientDlg::Ctuan4clientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TUAN4CLIENT_DIALOG, pParent)
	, m_strMsg(_T(""))
	, m_strIP(_T(""))
	, m_nPort(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Ctuan4clientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_CONTACT, m_tree);
	DDX_Control(pDX, IDC_STATIC_NAME, m_stName);
	DDX_Control(pDX, IDC_RICH_CHAT, m_richChat);
	DDX_Text(pDX, IDC_EDIT_MSG, m_strMsg);
	DDX_Text(pDX, IDC_EDIT_IP, m_strIP);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
}

BEGIN_MESSAGE_MAP(Ctuan4clientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_CHAT_RECV, &Ctuan4clientDlg::OnChatRecv)
	ON_MESSAGE(WM_CHAT_STATUS, &Ctuan4clientDlg::OnChatStatus)
	ON_MESSAGE(WM_CHAT_PEER, &Ctuan4clientDlg::OnChatPeer)

	ON_BN_CLICKED(IDC_BTN_CONNECT, &Ctuan4clientDlg::OnBnClickedBtnConnect)
	ON_BN_CLICKED(IDC_BTN_SEND, &Ctuan4clientDlg::OnBnClickedBtnSend)
END_MESSAGE_MAP()


// Ctuan4clientDlg message handlers

BOOL Ctuan4clientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		AfxMessageBox(L"WSAStartup failed");
		EndDialog(IDCANCEL);
		return TRUE;
	}
	// Add "About..." menu item to system menu.
	m_fontName.CreatePointFont(100, L"Segoe UI");
	m_stName.SetFont(&m_fontName);
	SetPeer(L"");
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void Ctuan4clientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void Ctuan4clientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR Ctuan4clientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void Ctuan4clientDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: Add your message handler code here
	if (m_sock != INVALID_SOCKET) { closesocket(m_sock); m_sock = INVALID_SOCKET; }
	WSACleanup();
}


void Ctuan4clientDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

}

void Ctuan4clientDlg::SetPeer(const CStringW& peer)
{
	m_tree.DeleteAllItems();
	HTREEITEM root = m_tree.InsertItem(L"Contacts");
	if (peer.IsEmpty())
	{
		m_stName.SetWindowText(L"(not connected)");
	}
	else
	{
		HTREEITEM h = m_tree.InsertItem(peer, root);
		m_tree.Expand(root, TVE_EXPAND);
		m_tree.SelectItem(h);
		m_stName.SetWindowText(peer);
	}
}


UINT Ctuan4clientDlg::ClientThread(LPVOID pParam)
{
	Ctuan4clientDlg* dlg = (Ctuan4clientDlg*)pParam;
	HWND hWnd = dlg->m_hWnd;
	SOCKET s = dlg->m_sock;

	CStringW msg;
	while (RecvMsg(s, msg))
		PostText(hWnd, WM_CHAT_RECV, msg);

	dlg->m_sock = INVALID_SOCKET;
	closesocket(s);
	PostText(hWnd, WM_CHAT_PEER, L"");
	PostText(hWnd, WM_CHAT_STATUS, L"[Disconnected from server]");
	return 0;
}

LRESULT Ctuan4clientDlg::OnChatRecv(WPARAM, LPARAM lp)
{
	CStringW* s = (CStringW*)lp;
	CStringW who;
	m_stName.GetWindowText(who);
	AppendRich(m_richChat, who, *s, RGB(200, 60, 0));
	delete s;
	return 0;
}

LRESULT Ctuan4clientDlg::OnChatStatus(WPARAM, LPARAM lp)
{
	CStringW* s = (CStringW*)lp;
	AppendRich(m_richChat, L"", *s, 0);
	delete s;
	return 0;
}

LRESULT Ctuan4clientDlg::OnChatPeer(WPARAM, LPARAM lp)
{
	CStringW* s = (CStringW*)lp;
	SetPeer(*s);
	if (s->IsEmpty())
		GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(TRUE);
	delete s;
	return 0;
}


void Ctuan4clientDlg::OnBnClickedBtnConnect()
{
	// TODO: Add your control notification handler code here
	if (m_sock != INVALID_SOCKET) return;

	UpdateData(TRUE);                                   

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons((u_short)m_nPort);
	if (InetPtonW(AF_INET, m_strIP, &addr.sin_addr) != 1)
	{
		AfxMessageBox(L"Invalid IP address");
		return;
	}

	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect(m_sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		CStringW e;
		e.Format(L"connect failed: %d", WSAGetLastError());
		AfxMessageBox(e);
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return;
	}

	CStringW name;
	name.Format(L"%s:%d", (LPCWSTR)m_strIP, m_nPort);
	SetPeer(name);
	AppendRich(m_richChat, L"", L"[Connected to server]", 0);
	GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(FALSE);

	AfxBeginThread(ClientThread, this);
}


void Ctuan4clientDlg::OnBnClickedBtnSend()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	if (m_strMsg.IsEmpty()) return;

	if (m_sock == INVALID_SOCKET)
	{
		AppendRich(m_richChat, L"", L"[Not connected]", 0);
		return;
	}
	if (SendMsg(m_sock, m_strMsg))
	{
		AppendRich(m_richChat, L"Me", m_strMsg, RGB(0, 102, 204));
		m_strMsg.Empty();
		UpdateData(FALSE);
	}
	else
		AppendRich(m_richChat, L"", L"[Send failed]", 0);
}


BOOL Ctuan4clientDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN &&
		GetFocus() == GetDlgItem(IDC_EDIT_MSG))
	{
		if (!(GetKeyState(VK_SHIFT) & 0x8000))   
		{
			OnBnClickedBtnSend();
			return TRUE;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}
