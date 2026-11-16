#include "pch.h"
#include "tuan2.h"
#include "CRegisterDlg.h"
#include "afxdialogex.h"
#include "ChatClient.h"

IMPLEMENT_DYNAMIC(CRegisterDlg, CDialogEx)

CRegisterDlg::CRegisterDlg(CWnd* pParent)
	: CDialogEx(IDD_REGISTER, pParent)
	, m_strUser(_T(""))
	, m_strPass(_T(""))
	, m_strConfirm(_T(""))
{

}

CRegisterDlg::~CRegisterDlg()
{
}

void CRegisterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_USER, m_strUser);
	DDX_Text(pDX, IDC_EDIT_PASS, m_strPass);
	DDX_Text(pDX, IDC_EDIT_CONFIRM, m_strConfirm);
    DDX_Text(pDX, IDC_EDIT_FULLNAME, m_strFullname);
}


BEGIN_MESSAGE_MAP(CRegisterDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CRegisterDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &CRegisterDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CRegisterDlg message handlers


void CRegisterDlg::OnBnClickedOk()
{
    UpdateData(TRUE);

    if (m_strUser.IsEmpty() || m_strPass.IsEmpty())
    {
        AfxMessageBox(L"Vui lòng nhập đầy đủ thông tin!");
        return;
    }

    if (m_strPass != m_strConfirm)
    {
        AfxMessageBox(L"Mật khẩu nhập lại không khớp!");
        return;
    }

    if (!g_chatClient.IsConnected())
    {
        AfxMessageBox(L"Chưa kết nối được tới server.");
        return;
    }

    std::wstring username(m_strUser.GetString());
    std::wstring fullname(m_strFullname.GetString());
    std::wstring password(m_strPass.GetString());
    std::wstring registerError;

    if (!g_chatClient.Register(username, fullname, password, registerError))
    {
        AfxMessageBox(CString(registerError.c_str()));
        return;
    }

    AfxMessageBox(L"Đăng ký thành công!");
    CDialogEx::OnOK();
}


void CRegisterDlg::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    CDialogEx::OnCancel();
}
