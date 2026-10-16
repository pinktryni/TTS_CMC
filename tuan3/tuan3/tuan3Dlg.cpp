
// tuan3Dlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "tuan3.h"
#include "resource.h"
#include "tuan3Dlg.h"
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


// Ctuan3Dlg dialog



Ctuan3Dlg::Ctuan3Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TUAN3_DIALOG, pParent)
	, m_strPeer(_T(""))
	, m_strMsg(_T(""))
	, m_nMode(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Ctuan3Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_PEER, m_strPeer);
	DDX_Text(pDX, IDC_EDIT_MSG, m_strMsg);
	DDX_Control(pDX, IDC_LIST_CHAT, m_lstChat);
	DDX_Radio(pDX, IDC_RADIO_WINMSG, m_nMode);
}

BEGIN_MESSAGE_MAP(Ctuan3Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_SEND, &Ctuan3Dlg::OnClickedBtnSend)
	ON_WM_COPYDATA()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_PIPE_MSG, &Ctuan3Dlg::OnPipeMsg)
END_MESSAGE_MAP()


// Ctuan3Dlg message handlers

BOOL Ctuan3Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

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
	if (m_myUser.IsEmpty())
	{
		m_myUser = AfxGetApp()->m_lpCmdLine;
		m_myUser.Trim();
		if (m_myUser.IsEmpty())
			m_myUser = L"user1";
	}
	SetWindowText(IpcChat::MakeWindowTitle(m_myUser));
	m_nMode = 0;
	UpdateData(FALSE);
	m_pipe.Start(m_hWnd, m_myUser);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void Ctuan3Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void Ctuan3Dlg::OnPaint()
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
HCURSOR Ctuan3Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void Ctuan3Dlg::OnClickedBtnSend()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	if (m_strPeer.IsEmpty() || m_strMsg.IsEmpty())
	{
		AfxMessageBox(L"Nhập người nhận và nội dung tin nhắn");
		return;
	}

	CString full = m_myUser + L": " + m_strMsg;
	BOOL ok = (m_nMode == 0)
		? IpcChat::SendByWinMsg(m_hWnd, m_strPeer, full)
		: IpcChat::SendByPipe(m_strPeer, full);

	if (ok)
	{
		AppendToChatBox(CString(m_nMode == 0 ? L"[WinMsg] " : L"[Pipe] ") + L"Tôi: " + m_strMsg);
		m_strMsg.Empty();
		UpdateData(FALSE);
		GetDlgItem(IDC_EDIT_MSG)->SetFocus();
	}
	else
		AfxMessageBox(L"Không gửi được. App kia đã chạy chưa, tên user có đúng không?");
}


BOOL Ctuan3Dlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCDS)
{
	// TODO: Add your message handler code here and/or call default
	if (pCDS->dwData == CHAT_COPYDATA_ID)
	{
		CString msg((LPCWSTR)pCDS->lpData);   // copy ngay, không giữ con trỏ
		AppendToChatBox(L"[WinMsg] " + msg);
		return TRUE;
	}
	return CDialogEx::OnCopyData(pWnd, pCDS);
}


void Ctuan3Dlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: Add your message handler code here
}

LRESULT Ctuan3Dlg::OnPipeMsg(WPARAM wParam, LPARAM lParam)
{
	CString* p = (CString*)lParam;
	AppendToChatBox(L"[Pipe] " + *p);
	delete p;                      // thread pipe đã new, UI thread phải delete
	return 0;
}

void Ctuan3Dlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	CDialogEx::OnOK();
}

void Ctuan3Dlg::AppendToChatBox(const CString& s)
{
	m_lstChat.AddString(s);
	m_lstChat.SetTopIndex(m_lstChat.GetCount() - 1);
}

void Ctuan3Dlg::OnBnClickedBtnSend()
{
	UpdateData(TRUE);
	if (m_strPeer.IsEmpty() || m_strMsg.IsEmpty())
	{
		AfxMessageBox(L"Nhập người nhận và nội dung tin nhắn");
		return;
	}

	CString full = m_myUser + L": " + m_strMsg;
	BOOL ok = (m_nMode == 0)
		? IpcChat::SendByWinMsg(m_hWnd, m_strPeer, full)
		: IpcChat::SendByPipe(m_strPeer, full);

	if (ok)
	{
		AppendToChatBox(CString(m_nMode == 0 ? L"[WinMsg] " : L"[Pipe] ") + L"Tôi: " + m_strMsg);
		m_strMsg.Empty();
		UpdateData(FALSE);
		GetDlgItem(IDC_EDIT_MSG)->SetFocus();
	}
	else
		AfxMessageBox(L"Không gửi được. App kia đã chạy chưa, tên user có đúng không?");
}






