#include "stdafx.h"
#include "GuildLibrary.h"

std::map<int, SPGuildLibrary> CGuildLibrary::guild_library_;
CGuildLibrary::CGuildLibrary()
{
}


CGuildLibrary::~CGuildLibrary()
{
}

void CGuildLibrary::Load()
{
	dat_GUILD_STRUCT_GuildLibrary pto_guild_library;
	pto_guild_library.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Guild.txt"));

	for (int i = 0; i < pto_guild_library.guild_library_size(); i++)
	{
		SPGuildLibrary guild = std::make_shared<CGuildLibrary>();
		guild->level_ = pto_guild_library.guild_library(i).level();
		guild->max_member_ = pto_guild_library.guild_library(i).max_member();
		guild->exp_ = pto_guild_library.guild_library(i).exp();
		guild->times_ = pto_guild_library.guild_library(i).times();

		guild_library_.insert(std::make_pair(guild->level_, guild));
	}
}

SPGuildLibrary CGuildLibrary::GetGuildLibrary(int level)
{
	auto it = guild_library_.find(level);
	if (guild_library_.cend() == it)
		return nullptr;
	return it->second;
}