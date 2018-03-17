#include "stdafx.h"
#include "AgentServer.h"

int _tmain(int argc, _TCHAR* argv[])
{
	int server_id{ 0 };
	if (argc > 1)
		server_id = _wtoi(argv[1]);
	else
		server_id = CBasicServer::GenerateServerID();

	try
	{
		CAgentServer svr(server_id);
		svr.ConnectGameServer();
		svr.RunNetwork(8);
		Sleep(INFINITE);
	}
	catch (std::exception ex)
	{
		std::cout << ex.what() << std::endl;
	}

	system("pause");

	return 0;
}
