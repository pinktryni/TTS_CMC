
// tuan4Dlg.cpp : implementation file
//

#include "pch.h"
#include "ChatProtocol.h"  
#include "framework.h"
#include "tuan4.h"
#include "tuan4Dlg.h"
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


// Ctuan4Dlg dialog



Ctuan4Dlg::Ctuan4Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TUAN4_DIALOG, pParent)
	, m_strMsg(_T(""))
	, m_nPort(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Ctuan4Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_CONTACT, m_tree);
	DDX_Control(pDX, IDC_STATIC_NAME, m_stName);
	DDX_Control(pDX, IDC_RICH_CHAT, m_richChat);
	DDX_Text(pDX, IDC_EDIT_MSG, m_strMsg);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
}

BEGIN_MESSAGE_MAP(Ctuan4Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_LISTEN, &Ctuan4Dlg::OnBnClickedBtnListen)
	ON_BN_CLICKED(IDC_BTN_SEND, &Ctuan4Dlg::OnBnClickedBtnSend)
	ON_WM_DESTROY()
	ON_MESSAGE(WM_CHAT_RECV, &Ctuan4Dlg::OnChatRecv)
	ON_MESSAGE(WM_CHAT_STATUS, &Ctuan4Dlg::OnChatStatus)
	ON_MESSAGE(WM_CHAT_PEER, &Ctuan4Dlg::OnChatPeer)
END_MESSAGE_MAP()


// Ctuan4Dlg message handlers

BOOL Ctuan4Dlg::OnInitDialog()
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
	m_fontName.CreatePointFont(70, L"Segoe UI");
	m_stName.SetFont(&m_fontName);
	SetPeer(L"");
	m_nPort = 5000;
	UpdateData(FALSE);
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

void Ctuan4Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void Ctuan4Dlg::OnPaint()
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
HCURSOR Ctuan4Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}





void Ctuan4Dlg::OnBnClickedBtnListen()
{
	// TODO: Add your control notification handler code here
	if (m_listen != INVALID_SOCKET) return;

	UpdateData(TRUE);                                  

	m_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((u_short)m_nPort);

	if (bind(m_listen, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR ||
		listen(m_listen, SOMAXCONN) == SOCKET_ERROR)
	{
		CStringW e;
		e.Format(L"bind/listen failed: %d", WSAGetLastError());
		AfxMessageBox(e);
		closesocket(m_listen);
		m_listen = INVALID_SOCKET;
		return;
	}

	CStringW s;
	s.Format(L"[Listening on port %d ...]", m_nPort);
	AppendRich(m_richChat, L"", s, 0);
	GetDlgItem(IDC_BTN_LISTEN)->EnableWindow(FALSE);

	AfxBeginThread(ServerThread, this);
}


void Ctuan4Dlg::OnBnClickedBtnSend()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	if (m_strMsg.IsEmpty()) return;

	if (m_client == INVALID_SOCKET)
	{
		AppendRich(m_richChat, L"", L"[No client connected]", 0);
		return;
	}
	if (SendMsg(m_client, m_strMsg))
	{
		AppendRich(m_richChat, L"Me", m_strMsg, RGB(0, 102, 204));
		m_strMsg.Empty();
		UpdateData(FALSE);                         
	}
	else
		AppendRich(m_richChat, L"", L"[Send failed]", 0);
}


void Ctuan4Dlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	if (m_client != INVALID_SOCKET) { closesocket(m_client); m_client = INVALID_SOCKET; }
	if (m_listen != INVALID_SOCKET) { closesocket(m_listen); m_listen = INVALID_SOCKET; }
	WSACleanup();
}


void Ctuan4Dlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class
}

void Ctuan4Dlg::SetPeer(const CStringW& peer)
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


UINT Ctuan4Dlg::ServerThread(LPVOID pParam)
{
	Ctuan4Dlg* dlg = (Ctuan4Dlg*)pParam;
	HWND hWnd = dlg->m_hWnd;

	while (true)
	{
		sockaddr_in peer = {};
		int plen = sizeof(peer);
		SOCKET c = accept(dlg->m_listen, (sockaddr*)&peer, &plen);
		if (c == INVALID_SOCKET) break;                
		wchar_t ip[INET_ADDRSTRLEN] = {};
		InetNtopW(AF_INET, &peer.sin_addr, ip, _countof(ip));
		CStringW name;
		name.Format(L"%s:%d", ip, ntohs(peer.sin_port));

		dlg->m_client = c;
		PostText(hWnd, WM_CHAT_PEER, name);
		PostText(hWnd, WM_CHAT_STATUS, L"[Client connected]");

		CStringW msg;
		while (RecvMsg(c, msg))
			PostText(hWnd, WM_CHAT_RECV, msg);

		dlg->m_client = INVALID_SOCKET;
		closesocket(c);
		PostText(hWnd, WM_CHAT_PEER, L"");           
		PostText(hWnd, WM_CHAT_STATUS, L"[Client disconnected]");
	}
	return 0;
}

LRESULT Ctuan4Dlg::OnChatRecv(WPARAM, LPARAM lp)
{
	CStringW* s = (CStringW*)lp;
	CStringW who;
	m_stName.GetWindowText(who);                       
	AppendRich(m_richChat, who, *s, RGB(200, 60, 0));
	delete s;
	return 0;
}

LRESULT Ctuan4Dlg::OnChatStatus(WPARAM, LPARAM lp)
{
	CStringW* s = (CStringW*)lp;
	AppendRich(m_richChat, L"", *s, 0);
	delete s;
	return 0;
}

LRESULT Ctuan4Dlg::OnChatPeer(WPARAM, LPARAM lp)
{
	CStringW* s = (CStringW*)lp;
	SetPeer(*s);
	delete s;
	return 0;
}

BOOL Ctuan4Dlg::PreTranslateMessage(MSG* pMsg)
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


