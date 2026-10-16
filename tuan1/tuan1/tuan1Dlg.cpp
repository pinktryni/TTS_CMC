
// tuan1Dlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "tuan1.h"
#include "tuan1Dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <string>

// Chuyển CString (UTF-16) sang chuỗi UTF-8 (std::string) để ghi file
static std::string CStringToUtf8(const CString& str)
{
	int len = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
	std::string result(len - 1, 0);  
	WideCharToMultiByte(CP_UTF8, 0, str, -1, &result[0], len, nullptr, nullptr);
	return result;
}

static CString Utf8ToCString(const char* data, int dataLen)
{
	int wlen = MultiByteToWideChar(CP_UTF8, 0, data, dataLen, nullptr, 0);
	CString result;
	WCHAR* buffer = result.GetBuffer(wlen);
	MultiByteToWideChar(CP_UTF8, 0, data, dataLen, buffer, wlen);
	result.ReleaseBuffer(wlen);
	return result;
}


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


// Ctuan1Dlg dialog



Ctuan1Dlg::Ctuan1Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TUAN1_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Ctuan1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Ctuan1Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(open, &Ctuan1Dlg::OnBnClickedopen)
	ON_BN_CLICKED(folder, &Ctuan1Dlg::OnBnClickedButton1)
	ON_BN_CLICKED(save, &Ctuan1Dlg::OnBnClickedsave)
	ON_BN_CLICKED(open, &Ctuan1Dlg::OnBnClickedopen)
	ON_EN_CHANGE(content, &Ctuan1Dlg::OnEnChangecontent)
END_MESSAGE_MAP()


// Ctuan1Dlg message handlers

BOOL Ctuan1Dlg::OnInitDialog()
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

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void Ctuan1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void Ctuan1Dlg::OnPaint()
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
HCURSOR Ctuan1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void Ctuan1Dlg::OnEnChangeEdit1()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void Ctuan1Dlg::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here
}


void Ctuan1Dlg::OnBnClickedsave()
{
	TCHAR szFile[MAX_PATH] = _T("");

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = this->m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter =
		_T("Text Files (*.txt)\0*.txt\0")
		_T("All Files (*.*)\0*.*\0");
	ofn.lpstrDefExt = _T("txt");
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn))
	{
		CString Path = szFile;
		SetDlgItemText(path, Path);

		CString Content;
		GetDlgItemText(content, Content);

		HANDLE hFile = CreateFile(
			Path,                  
			GENERIC_WRITE,        
			0,                     
			nullptr,                
			CREATE_ALWAYS,          
			FILE_ATTRIBUTE_NORMAL,
			nullptr);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			AfxMessageBox(_T("Không thể tạo file!"));
			return;
		}

		DWORD bytesWritten = 0;

		BYTE bom[3] = { 0xEF, 0xBB, 0xBF };
		WriteFile(hFile, bom, 3, &bytesWritten, nullptr);

		std::string utf8Content = CStringToUtf8(Content);
		WriteFile(hFile, utf8Content.c_str(), (DWORD)utf8Content.length(), &bytesWritten, nullptr);

		CloseHandle(hFile);

		AfxMessageBox(_T("Lưu file thành công!"));
	}
}

void Ctuan1Dlg::OnBnClickedopen()
{
	// TODO: Add your control notification handler code here
	TCHAR szFile[MAX_PATH] = _T("");
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = this->m_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter =
		_T("Text Files (*.txt)\0*.txt\0")
		_T("All Files (*.*)\0*.*\0");
	ofn.lpstrDefExt = _T("txt");
	ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
		CString Path = szFile;
		SetDlgItemText(path, Path);

		HANDLE hFile = CreateFile(
			Path,
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			AfxMessageBox(_T("Không thể mở file!"));
			return;
		}

		DWORD fileSize = GetFileSize(hFile, nullptr);
		if (fileSize == INVALID_FILE_SIZE || fileSize == 0)
		{
			CloseHandle(hFile);
			SetDlgItemText(content, _T(""));
			return;
		}

		std::string buffer(fileSize, 0);
		DWORD bytesRead = 0;
		BOOL ok = ReadFile(hFile, &buffer[0], fileSize, &bytesRead, nullptr);
		CloseHandle(hFile);

		if (!ok)
		{
			AfxMessageBox(_T("Đọc file thất bại!"));
			return;
		}

		buffer.resize(bytesRead);

		int offset = 0;
		if (bytesRead >= 3 &&
			(BYTE)buffer[0] == 0xEF &&
			(BYTE)buffer[1] == 0xBB &&
			(BYTE)buffer[2] == 0xBF)
		{
			offset = 3;
		}

		CString Content = Utf8ToCString(buffer.c_str() + offset, (int)buffer.length() - offset);
		SetDlgItemText(content, Content);
	}
}


void Ctuan1Dlg::OnEnChangecontent()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}
