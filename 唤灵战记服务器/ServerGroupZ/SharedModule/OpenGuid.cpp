#include "stdafx.h"
#include "OpenGuid.h"

std::map<int, const COpenGuid*> COpenGuid::ms_mapOpenGuid;

void COpenGuid::Load()
{
	Guid_Library libGuid;
	libGuid.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Guid.txt"));

	for (int i = 0; i < libGuid.open_guid_library_size(); i++)
	{
		COpenGuid* pOpenGuid = new COpenGuid;
		pOpenGuid->fun_ = libGuid.open_guid_library(i).fun();
		pOpenGuid->open_type_ = libGuid.open_guid_library(i).open_type();
		pOpenGuid->parameters_ = libGuid.open_guid_library(i).parameters();

		ms_mapOpenGuid.insert(std::make_pair(pOpenGuid->fun_, pOpenGuid));
	}
}

const COpenGuid* COpenGuid::GetOpenGuid(int type)
{
	auto it = ms_mapOpenGuid.find(type);
	if (ms_mapOpenGuid.cend() == it)
		return nullptr;
	else
		return it->second;
}
