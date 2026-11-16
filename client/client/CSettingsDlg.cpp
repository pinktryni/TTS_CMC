// CSettingsDlg.cpp : implementation file
//

#include "pch.h"
#include "tuan2.h"
#include "CSettingsDlg.h"
#include "afxdialogex.h"
#include "ChatClient.h" 
#include "CMainDlg.h"
#include <atlimage.h>
#include "Utils.h"


// CSettingsDlg dialog

IMPLEMENT_DYNAMIC(CSettingsDlg, CDialogEx)

CSettingsDlg::CSettingsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTINGS, pParent)
{

}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_CURRENT_USER, m_staticCurrentUser);
	DDX_Control(pDX, IDC_STATIC_FULLNAME, m_staticFullName);
    DDX_Control(pDX, IDC_STATIC_AVATAR, m_staticAvatar);
    DDX_Control(pDX, IDC_CHECK_STARTUP, m_checkStartup);
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_LOGOUT, &CSettingsDlg::OnBnClickedBtnLogout)
    ON_BN_CLICKED(IDC_BTN_CHANGE_AVATAR, &CSettingsDlg::OnBnClickedBtnChangeAvatar)
    ON_BN_CLICKED(IDC_CHECK_STARTUP, &CSettingsDlg::OnBnClickedCheckStartup)
END_MESSAGE_MAP()


// CSettingsDlg message handlers
BOOL CSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	std::wstring currentUser = g_chatClient.GetCurrentUsername();
	std::wstring fullname = g_chatClient.GetFullName(currentUser);
	m_staticFullName.SetWindowText(fullname.c_str());
	CString strUser;
	strUser.Format(L"User: %s", currentUser.c_str());
	m_staticCurrentUser.SetWindowText(strUser);
    std::vector<BYTE> avatarBytes = g_chatClient.GetAvatar(currentUser);
    if (!avatarBytes.empty()) {
        HBITMAP hBmp = BytesToHBITMAP(avatarBytes, 100, 100); // Ảnh 100x100
        if (hBmp) {
            HBITMAP hOld = m_staticAvatar.SetBitmap(hBmp);
            if (hOld) DeleteObject(hOld); // Xóa ảnh cũ tránh rò rỉ RAM
        }
    }
    LoadAvatar();
    m_checkStartup.SetCheck(IsStartupEnabled() ? BST_CHECKED : BST_UNCHECKED);
    return TRUE;
}


void CSettingsDlg::OnBnClickedBtnLogout()
{
	if (AfxMessageBox(L"Bạn có chắc muốn đăng xuất khỏi tài khoản này?", MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		EndDialog(IDLOGOUT);
	}
}

void CSettingsDlg::LoadAvatar()
{
    std::wstring currentUser = g_chatClient.GetCurrentUsername();
    std::vector<BYTE> avatarBytes = g_chatClient.GetAvatar(currentUser);
    if (!avatarBytes.empty()) {
        HBITMAP hBmp = BytesToHBITMAP(avatarBytes, 100, 100);
        SafeSetBitmap(m_staticAvatar, hBmp);
    }
    else {
        SafeSetBitmap(m_staticAvatar, NULL);
    }
}

void CSettingsDlg::OnBnClickedBtnChangeAvatar()
{
    CFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        L"Image Files (*.jpg;*.jpeg;*.png;*.bmp)|*.jpg;*.jpeg;*.png;*.bmp||", this);

    if (dlg.DoModal() == IDOK)
    {
        CString filePath = dlg.GetPathName();

        // Đọc file thành mảng byte
        CFile file;
        if (file.Open(filePath, CFile::modeRead)) {
            ULONGLONG length = file.GetLength();
            std::vector<BYTE> buffer(length);
            file.Read(buffer.data(), (UINT)length);
            file.Close();

            std::string b64 = Base64Encode(buffer);
            if (b64.size() > 1 * 1024 * 1024) {
                AfxMessageBox(L"Ảnh không hợp lệ");
                return; 
            }
            std::wstring currentUser = g_chatClient.GetCurrentUsername();

            if (g_chatClient.SetAvatar(currentUser, b64)) {
                LoadAvatar();
                CMainDlg* pMainDlg = dynamic_cast<CMainDlg*>(GetParent());
                if (pMainDlg) {
                    pMainDlg->UpdateChatAvatar();
                }
                AfxMessageBox(L"Cập nhật ảnh đại diện thành công!");
            }
            else {
                AfxMessageBox(L"Lỗi khi gửi ảnh lên server.");
            }
        }
    }
}

void CSettingsDlg::OnBnClickedCheckStartup()
{
    bool isChecked = (m_checkStartup.GetCheck() == BST_CHECKED);
    SetStartup(isChecked);
}