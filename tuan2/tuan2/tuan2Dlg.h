
// tuan2Dlg.h : header file
//

#pragma once


// Ctuan2Dlg dialog
class Ctuan2Dlg : public CDialogEx
{
// Construction
public:
	Ctuan2Dlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TUAN2_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CString m_strUser;
	CString m_strPass;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedBtnRegister();
};
