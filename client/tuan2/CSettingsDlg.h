#pragma once

class CSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingsDlg)

public:
	CSettingsDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CSettingsDlg();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTINGS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog(); 

	DECLARE_MESSAGE_MAP()

public:
	CStatic m_staticCurrentUser;
	afx_msg void OnBnClickedBtnLogout(); 
	CStatic m_staticFullName;
	afx_msg void OnBnClickedBtnChangeAvatar();
	CStatic m_staticAvatar;      
	void LoadAvatar();
	CButton m_checkStartup;
	afx_msg void OnBnClickedCheckStartup();
};