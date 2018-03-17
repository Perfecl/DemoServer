#include "stdafx.h"
#include "ServerConsole.h"
#include "ServerConsoleDlg.h"
#include "afxdialogex.h"
#include <regex>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{

}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

CServerConsoleDlg::CServerConsoleDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CServerConsoleDlg::IDD, pParent)
	, edit_key_value_(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CServerConsoleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_SERVER, tree_servers_);
	DDX_Text(pDX, IDC_EDIT_KEY, edit_key_value_);
}

BEGIN_MESSAGE_MAP(CServerConsoleDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_KEY, &CServerConsoleDlg::OnBnClickedButtonKey)
	ON_BN_CLICKED(IDC_BUTTON_START, &CServerConsoleDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_GIFT, &CServerConsoleDlg::OnBnClickedButtonGift)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CServerConsoleDlg::OnBnClickedButtonClose)
	ON_BN_CLICKED(IDC_BUTTON_BROADCAST, &CServerConsoleDlg::OnBnClickedButtonBroadcast)
END_MESSAGE_MAP()

BOOL CServerConsoleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	__Initialization();

	return TRUE;
}

void CServerConsoleDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CServerConsoleDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CServerConsoleDlg::OnQueryDragIcon()
{
	//当用户拖动最小化窗口时系统调用此函数取得光标
	//显示。
	return static_cast<HCURSOR>(m_hIcon);
}

void CServerConsoleDlg::OnBnClickedButtonKey()
{
	UpdateData();

	std::wregex reg_base(L"[0-9A-Z]{2}-[0-9A-Z]{2}-[0-9A-Z]{2}-[0-9A-Z]{2}-[0-9A-Z]{2}-[0-9A-Z]{2}");

	if (std::regex_match(edit_key_value_.GetBuffer(), reg_base))
	{
		std::string path("./");
		path += (ToMD5(UNICODE_to_ANSI(edit_key_value_.GetBuffer()) + SECURITY_KEY) + ".key");
		HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		CloseHandle(hFile);
		MessageBox(L"生成密钥成功", L"information", MB_OK);
	}
	else
	{
		MessageBox(L"mac地址格式错误,请按以下格式填写:\nXX-XX-XX-XX-XX-XX (字母大写)", L"warning", MB_OK);
	}
}

void CServerConsoleDlg::OnBnClickedButtonStart()
{
	ShellExecute(NULL, L"open", L"FlashPolicy.exe", NULL, NULL, SW_SHOWNORMAL);

	for (auto &it : CBasicServer::GetHostIPv4())
	{
		__OpenServers(server_list_->GetLoginServersByHostIP(it.c_str()), L"LoginServer.exe");
		__OpenServers(server_list_->GetAgentServersByHostIP(it.c_str()), L"AgentServer.exe");
		__OpenServers(server_list_->GetGameServersByHostIP(it.c_str()), L"GameServer.exe");
		__OpenServers(server_list_->GetBattleServersByHostIP(it.c_str()), L"BattleServer.exe");

		auto server_ids = server_list_->GetBattleServersByHostIP(it.c_str());

		if (!server_ids.empty())
		{
			wchar_t param[16]{0};
			_itow_s(server_list_->GetBattleServerInformation(server_ids[0])->platform_id, param, 10);
			ShellExecute(NULL, L"runas", L"RechargeServer.exe", param, NULL, SW_SHOWNORMAL);
		}
	}
}

void CServerConsoleDlg::OnBnClickedButtonGift()
{
	for (auto &it : CBasicServer::GetHostIPv4())
	{
		auto game_svr = server_list_->GetGameServersByHostIP(it.c_str());

		if (!game_svr.empty())
		{
			auto database_info = server_list_->GetDatabaseInformation(server_list_->GetGameServerInformation(game_svr[0])->database_id);
			if (false == svr.ConnectDatabase(SERVER_LIST()->GetHostIP(database_info->host_id), database_info->port, database_info->username.c_str(), database_info->password.c_str(), database_info->database_name.c_str()))
				MessageBox(L"连接数据库失败");
		}
	}

	std::ostringstream sql;
	sql << "select count(*) from tb_gift";

	ResultSetPtr result = svr.MySQLQuery(sql.str());
	if (result && result->next())
	{
		/*if (result->getInt(1))
		{
			MessageBox(L"数据库已经有礼包信息");
			return;
		}*/
		char key[32]{0};

		HANDLE hFile = CreateFile(L"GiftKey.txt", GENERIC_READ | GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);


		std::ostringstream sql;
		sql << "insert into tb_gift values";

		std::string log;

		for (size_t i = 1; i <= 5; i++)
		{
			for (size_t j = 2001; j <= 4000; j++)
			{
				sprintf_s(key, "HLZJ%d%05d", i, j);
				std::string md5 = ToMD5(key).substr(8, 8);
				strcat_s(key, md5.c_str());
				sql << "('" << key << "'," << i << ",default),";

				log.append(key);
				log.append("\r\n");
			}

			log.append("\r\n");
		}

		DWORD dw{ 0 };
		WriteFile(hFile, log.c_str(), log.length(), &dw, NULL);
		CloseHandle(hFile);

		std::string str_sql = std::move(sql.str());
		str_sql.erase(str_sql.length() - 1, 1);
		svr.MySQLUpdate(str_sql);

		MessageBox(L"礼包生成成功");
	}
}

void CServerConsoleDlg::__Initialization()
{
	__InitTreeServers();
}

void CServerConsoleDlg::__InitTreeServers()
{
	tree_servers_.ModifyStyle(NULL, TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT);

	HTREEITEM tree_root_ = tree_servers_.InsertItem(L"服务器列表", NULL, NULL);
}

void CServerConsoleDlg::__OpenServers(std::vector<int> server_id, const wchar_t* server_name)
{
	wchar_t param[16]{0};

	for (auto &it : server_id)
	{
		ZeroMemory(param, sizeof(param));

		_itow_s(it, param, 10);

		ShellExecute(NULL, L"open", server_name, param, NULL, SW_SHOWNORMAL);
	}
}


void CServerConsoleDlg::OnBnClickedButtonClose()
{
	// TODO:  在此添加控件通知处理程序代码

	//Socket连接
}


void CServerConsoleDlg::OnBnClickedButtonBroadcast()
{
	// TODO:  在此添加控件通知处理程序代码
}
