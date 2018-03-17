#include "stdafx.h"
#include "NameLibrary.h"

bool CNameLibrary::is_load{ false };

std::vector<std::string> CNameLibrary::last_name;
std::vector<std::string> CNameLibrary::man_first_name;
std::vector<std::string> CNameLibrary::women_first_name;

std::string CNameLibrary::GetRandomName(bool sex)
{
	if (false == is_load)
		__Load();

	std::string name{ last_name[GetRandom(SIZE_0, last_name.size() - 1)] };

	if (sex)
		name += man_first_name[GetRandom(SIZE_0, man_first_name.size() - 1)];
	else
		name += women_first_name[GetRandom(SIZE_0, women_first_name.size() - 1)];

	return std::move(name);
}

void CNameLibrary::__Load()
{
	DAT_NAME datName;
	datName.ParseFromString(GetDataFromFile(GAME_DATA_PATH"GameName.txt"));

	for (int i = 0; i < datName.last_name_size(); i++)
		last_name.push_back(datName.last_name(i));
	for (int i = 0; i < datName.men_fist_name_size(); i++)
		man_first_name.push_back(datName.men_fist_name(i));
	for (int i = 0; i < datName.women_fitst_name_size(); i++)
		women_first_name.push_back(datName.women_fitst_name(i));

	is_load = true;
}
