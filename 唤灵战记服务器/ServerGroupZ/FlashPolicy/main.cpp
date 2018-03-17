#include "stdafx.h"
#include "FlashPolicy.h"

int _tmain(int argc, _TCHAR* argv[])
{
	CFlashPolicy svr;

	svr.RunNetwork(4);

	Sleep(INFINITE);

	return 0;
}
