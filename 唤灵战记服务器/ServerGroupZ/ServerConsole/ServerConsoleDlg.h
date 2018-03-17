#pragma once
#include "afxcmn.h"

class CServerConsoleDlg : public CDialogEx
{
public:
	CServerConsoleDlg(CWnd* pParent = NULL);

	enum { IDD = IDD_SERVERCONSOLE_DIALOG };

protected:
	HICON m_hIcon;
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);

	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnBnClickedButtonKey();
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonGift();

	DECLARE_MESSAGE_MAP()

private:
	CTreeCtrl		tree_servers_;
	CString			edit_key_value_;

	const CServerList* server_list_{ CServerList::GetInstance() };

	CBasicServer	svr;

	void __Initialization();				//初始化
	void __InitTreeServers();				//初始化Tree控件	

private:
	void __OpenServers(std::vector<int> server_id, const wchar_t* server_name);	
public:
	afx_msg void OnBnClickedButtonClose();
	afx_msg void OnBnClickedButtonBroadcast();
};
