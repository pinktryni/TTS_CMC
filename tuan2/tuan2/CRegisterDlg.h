#pragma once


// CRegisterDlg dialog

class CRegisterDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRegisterDlg)

public:
	CRegisterDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CRegisterDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REGISTER };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString m_strUser;
	CString m_strPass;
	CString m_strConfirm;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
