#include "stdafx.h"
#include "FlashPolicy.h"

CFlashPolicy::CFlashPolicy()
{
	__ShowServerInfo();

#ifdef _TEST
	InitTCPService(843, 100, true);
#else
	InitTCPService(843, 100, false);
#endif
}

CFlashPolicy::~CFlashPolicy()
{

}

void CFlashPolicy::_OnReceive(networkz::ISession* session, int error_code)
{
	if (error_code)
		return;

	size_t data_length{ session->data_length() };

	if (0 == strcmp(session->data(true), "<policy-file-request/>"))
		session->send("<?xml version=\"1.0\" encoding=\"UTF-8\"?><cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"*\"/></cross-domain-policy>", 126);
	else
		session->close();
}

void CFlashPolicy::__ShowServerInfo()
{
	_PrintLine("*****************************************");
	_Print("服务器名:\tFlashPolicy");

#ifdef _TEST
	_Print("(测试版)");
#endif 

	_Print("\n运行版本:\t");

#ifdef _DEBUG 
	_PrintLine("Debug");
#else
	_PrintLine("Release");
#endif

	char str_data[32]{0};
	time_t now = time(0);
	ctime_s(str_data, sizeof(str_data), &now);

	_Print("启动时间:\t");
	_Print(str_data);
	_PrintLine("*****************************************");
}
