#include "stdafx.h"
#include <fstream>
#include <Nb30.h>
#include <chrono>
#include <regex>
#include "common_function.h"

#pragma comment(lib,"Netapi32.lib")
#pragma comment(lib,"../SharedModule/mysql/lib/mysqlcppconn.lib")
#pragma comment(lib,"../SharedModule/mysql/lib/mysqlcppconn-static.lib")
#pragma comment(lib,"../SharedModule/NetworkZ/NetworkZ.lib")
#pragma comment(lib,"../SharedModule/LoggerZ/LoggerZ.lib")

#ifdef _DEBUG
#pragma comment(lib, "../SharedModule/ProtobufLib/protobuf_debug.lib")
#else
#pragma comment(lib, "../SharedModule/ProtobufLib/protobuf_release.lib")
#endif


std::string GetDataFromFile(const char* path)
{
	std::ifstream ifs(path, std::ios::binary | std::ios::in);

	if (!ifs.is_open())
		throw std::exception(FormatString("Î´ÕÒµ½ÎÄ¼þ:", path).c_str());

	std::istream::pos_type current_pos = ifs.tellg();
	ifs.seekg(0, std::ios_base::end);
	std::istream::pos_type file_size = ifs.tellg();
	ifs.seekg(current_pos);
	size_t nBufLen{ (size_t)file_size };

	char* szBuffer = new char[nBufLen];
	ifs.read(szBuffer, nBufLen);
	std::string strPto(szBuffer, (size_t)ifs.gcount());

	ifs.close();
	delete[]szBuffer;

	return move(strPto);
}

std::wstring ANSI_to_UNICODE(std::string src)
{
	int length{ MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, NULL, 0) };

	wchar_t *destination{ new wchar_t[length + 1] };

	ZeroMemory(destination, (length + 1) * sizeof(*destination));

	MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, destination, length);

	std::wstring strDest(destination);

	delete[] destination;

	return move(strDest);
}

std::string  ANSI_to_UTF8(std::string src)
{
	return move(UNICODE_to_UTF8(ANSI_to_UNICODE(src)));
}

std::string  UNICODE_to_ANSI(std::wstring src)
{
	int length = WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, NULL, 0, NULL, NULL);

	char *destination = new char[length + 1];

	ZeroMemory(destination, (length + 1) * sizeof(*destination));

	WideCharToMultiByte(CP_ACP, 0, src.c_str(), -1, destination, length, NULL, NULL);

	std::string strDest(destination);

	delete[] destination;

	return move(strDest);
}

std::string  UNICODE_to_UTF8(std::wstring src)
{
	int length{ WideCharToMultiByte(CP_UTF8, 0, src.c_str(), -1, NULL, 0, NULL, NULL) };

	char *destination{ new char[length + 1]{0} };

	ZeroMemory(destination, (length + 1) * sizeof(*destination));

	WideCharToMultiByte(CP_UTF8, 0, src.c_str(), -1, destination, length, NULL, NULL);

	std::string strDest(destination);

	delete[] destination;

	return move(strDest);
}

std::wstring UTF8_to_UNICODE(std::string src)
{
	int length = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, NULL, 0);

	wchar_t *destination = new wchar_t[length + 1];

	ZeroMemory(destination, (length + 1) * sizeof(*destination));

	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, destination, length);

	std::wstring strDest(destination);

	delete[] destination;

	return move(strDest);
}

std::string  UTF8_to_ANSI(std::string src)
{
	return move(UNICODE_to_ANSI(UTF8_to_UNICODE(src)));
}

int	GetNameStandardLength(const std::string& name)
{
	std::wregex reg_base(L"[0-9A-Za-z_]");
	std::wregex reg_chinese(L"[\\u4e00-\\u9fa5]");
	std::wregex reg_japanese(L"[\\u3040-\\u309F\\u30A0-\\u30FF]");

	int length{ 0 };

	std::wstring wname = UTF8_to_UNICODE(name);

	for (size_t i = 0; i < wname.length(); i++)
	{
		if (regex_match(wname.substr(i, 1), reg_chinese))
			length += 2;
		else if (regex_match(wname.substr(i, 1), reg_japanese))
			length += 2;
		else if (regex_match(wname.substr(i, 1), reg_base))
			length += 1;
		else
			return -1;
	}

	return length;
}

std::string MacAddress()
{
	NCB ncb;
	typedef struct _ASTAT_
	{
		ADAPTER_STATUS  adapt;
		NAME_BUFFER  NameBuff[30];
	}ASTAT, *PASTAT;
	ASTAT Adapter;

	typedef struct _LANA_ENUM
	{
		UCHAR  length;
		UCHAR  lana[MAX_LANA];
	}LANA_ENUM;

	LANA_ENUM lana_enum;

	UCHAR uRetCode;
	memset(&ncb, 0, sizeof(ncb));
	memset(&lana_enum, 0, sizeof(lana_enum));

	ncb.ncb_command = NCBENUM;
	ncb.ncb_buffer = (unsigned char *)&lana_enum;
	ncb.ncb_length = sizeof(LANA_ENUM);
	uRetCode = Netbios(&ncb);
	if (uRetCode != NRC_GOODRET)
		return "";

	for (int lana = 0; lana < lana_enum.length; lana++)
	{
		ncb.ncb_command = NCBRESET;
		ncb.ncb_lana_num = lana_enum.lana[lana];
		uRetCode = Netbios(&ncb);
		if (uRetCode == NRC_GOODRET)
			break;
	}
	if (uRetCode != NRC_GOODRET)
		return "";

	memset(&ncb, 0, sizeof(ncb));
	ncb.ncb_command = NCBASTAT;
	ncb.ncb_lana_num = lana_enum.lana[0];
	strcpy_s((char*)ncb.ncb_callname, sizeof(ncb.ncb_callname), "*");
	ncb.ncb_buffer = (unsigned char *)&Adapter;
	ncb.ncb_length = sizeof(Adapter);
	uRetCode = Netbios(&ncb);

	if (uRetCode != NRC_GOODRET)
		return "";

	char mac[32]{0};

	sprintf_s(mac, "%02X-%02X-%02X-%02X-%02X-%02X",
		Adapter.adapt.adapter_address[0],
		Adapter.adapt.adapter_address[1],
		Adapter.adapt.adapter_address[2],
		Adapter.adapt.adapter_address[3],
		Adapter.adapt.adapter_address[4],
		Adapter.adapt.adapter_address[5]);

	return mac;
}

int	GetTimeValue(TIME_TYPE type, time_t timez)
{
	tm local{ 0 };
	localtime_s(&local, &timez);

	int value{ 0 };

	switch (type)
	{
	case TIME_TYPE::SECOND:
		value = local.tm_sec;
		break;
	case TIME_TYPE::MINUTE:
		value = local.tm_min;
		break;
	case TIME_TYPE::HOUR:
		value = local.tm_hour;
		break;
	case TIME_TYPE::DAY:
		value = local.tm_mday;
		break;
	case TIME_TYPE::WEEK:
		value = local.tm_wday;
		break;
	case TIME_TYPE::MONTH:
		value = local.tm_mon + 1;
		break;
	case TIME_TYPE::YEAR:
		value = local.tm_year + 1900;
		break;
	case TIME_TYPE::DATE:
		value = local.tm_yday + 1;
		break;
	default:
		value = -1;
		break;
	}

	return value;
}

__int64	GetDurationCount(time_t after, time_t before, DURATION_TYPE enType)
{
	auto duration = std::chrono::steady_clock::from_time_t(after) - std::chrono::steady_clock::from_time_t(before);

	__int64 value{ 0 };

	switch (enType)
	{
	case DURATION_TYPE::MICROSECOND:
		value = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
		break;
	case DURATION_TYPE::MILLISECOND:
		value = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
		break;
	case DURATION_TYPE::SECOND:
		value = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
		break;
	case DURATION_TYPE::MINUTE:
		value = std::chrono::duration_cast<std::chrono::minutes>(duration).count();
		break;
	case DURATION_TYPE::HALF_HOUR:
		value = std::chrono::duration_cast<std::chrono::duration<int, std::ratio<3600 / 2>>>(duration).count();
		break;
	case DURATION_TYPE::HOUR:
		value = std::chrono::duration_cast<std::chrono::hours>(duration).count();
		break;
	case DURATION_TYPE::DAY:
		value = std::chrono::duration_cast<std::chrono::duration<int, std::ratio<3600 * 24>>>(duration).count();
		break;
	case DURATION_TYPE::WEEK:
		value = std::chrono::duration_cast<std::chrono::duration<int, std::ratio<3600 * 24 * 7>>>(duration).count();
		break;
	default:
		value = -1;
		break;
	}

	return value;
}