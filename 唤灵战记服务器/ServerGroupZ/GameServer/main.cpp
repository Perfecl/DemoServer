#include "stdafx.h"
#include "GameServer.h"

int _tmain(int argc, _TCHAR* argv[])
{
	DeleteMenu(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_BYCOMMAND);

	int server_id{ 0 };
	if (argc > 1)
		server_id = _wtoi(argv[1]);
	else
		server_id = CBasicServer::GenerateServerID();

	try
	{
		CGameServer svr(server_id);
		svr.ConnectDatabase();
		svr.ConnectBattleServer();
		svr.CreateGameWorld();
		svr.RunNetwork(4);
		
		std::string order;
		while (true)
		{
			order.clear();
			std::cin >> order;

			if ("exit" == order)
				break;
		}
	}
	catch (std::exception ex)
	{
		std::cout << ex.what() << std::endl;
	}

	system("pause");

	return 0;
}
