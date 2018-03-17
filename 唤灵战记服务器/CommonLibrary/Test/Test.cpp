#include "stdafx.h"
#include "../NetworkZ/NetworkZ.h"
#include "../LoggerZ/LoggerZ.h"
#pragma comment(lib,"../Debug/NetworkZ.lib")
#pragma comment(lib,"../Debug/LoggerZ.lib")

#include <thread>
#include <vector>
using namespace Concurrency;

int _tmain(int argc, _TCHAR* argv[])
{
	std::vector<std::thread> vct;


	std::thread skk([](){printf("Hello"); });

	//skk.detach();

	skk.join();

	vct.push_back(std::move(skk));


	system("pause");

	return 0;
}

