#include "stdafx.h"
#include "PlayerCache.h"
#include "HeroCard.h"
#include "Equip.h"
#include "Knapsack.h"
#include "Guild.h"

CPlayerCache::CPlayerCache(int pid) :
pid_{ pid }
{
	__LoadBaseInfoFromDatabase();

	heroes_.fill(nullptr);
}

CPlayerCache::~CPlayerCache()
{
	for (auto &it : heroes_)
		delete it;
	for (auto &it : equips_)
		delete it.first;
}

void CPlayerCache::guild_id(int guild_id)
{
	std::ostringstream sql;
	sql << "update tb_player set guild_id = " << guild_id << " where pid = " << pid_;
	MYSQL_UPDATE(sql.str());
}

int	CPlayerCache::exercise_time()
{
	if (false == has_detail_)
		__LoadDetailInfoFromDatabase();

	return exercise_time_;
}

int	CPlayerCache::bodygurand()
{
	if (false == has_detail_)
		__LoadDetailInfoFromDatabase();

	return bodygurand_;
}

int	CPlayerCache::offline_battle_win_streak()
{
	if (false == has_detail_)
		__LoadDetailInfoFromDatabase();

	return offline_battle_win_streak_;
}

time_t CPlayerCache::offline_last_time()
{
	if (false == has_detail_)
		__LoadDetailInfoFromDatabase();

	return offline_battle_win_streak_;
}

const CHeroCard* CPlayerCache::hero(int index)
{
	if (false == has_detail_)
		__LoadDetailInfoFromDatabase();

	if ((size_t)index >= heroes_.size())
		return nullptr;

	return heroes_[index];
}

int	CPlayerCache::clothes()
{
	if (false == has_detail_)
		__LoadDetailInfoFromDatabase();

	return clothes_id_;
}

std::vector<const CEquip*>	CPlayerCache::hero_equip(int hero_id)
{
	if (false == has_detail_)
		__LoadDetailInfoFromDatabase();

	std::vector<const CEquip*> temp;

	for (auto &it : equips_)
	{
		if (hero_id == -it.second)
			temp.push_back(it.first);
	}

	return std::move(temp);
}

int	CPlayerCache::technology(int index)
{
	if (false == has_detail_)
		__LoadDetailInfoFromDatabase();

	if (size_t(index) >= technology_.size())
		return 0;

	return technology_[index];
}

int	CPlayerCache::speed_best_rank()
{
	if (false == has_detail_)
		__LoadDetailInfoFromDatabase();

	return speed_best_rank_;
}

const BattleRecord& CPlayerCache::battle_record(BattleRecordType type)
{
	if (false == has_detail_)
		__LoadDetailInfoFromDatabase();

	if (size_t(type) >= battle_record_.size())
		throw std::exception("战斗记录类型错误");

	return battle_record_[type];
}

int	CPlayerCache::battle_week_times(BattleRecordType type)
{
	if (false == has_detail_)
		__LoadDetailInfoFromDatabase();

	switch (type)
	{
	case kBattleRecordWar:return week_war_times_;
	case kBattleRecord1v1:return week_1v1_times_;
	case kBattleRecord3v3:return week_3v3_times_;
	}
	return 0;
}

std::string	CPlayerCache::guild_name()
{
	auto guild = GAME_WORLD()->FindGuild(guild_id_);

	if (guild)
	{
		return guild->name();
	}
	else
	{
		return "";
	}
}

int	CPlayerCache::guild_postion()
{
	auto guild = GAME_WORLD()->FindGuild(guild_id_);

	if (guild)
		return (int)guild->GetPosition(pid_);
	else
		return 0;
}

void CPlayerCache::__LoadBaseInfoFromDatabase()
{
	std::ostringstream sql;
	sql << "select sid,name,sex,level,guild_id,reputation,using_title,unix_timestamp(offline_time) as ott from tb_player where pid = " << pid_;
	ResultSetPtr result{ MYSQL_QUERY(sql.str()) };
	if (result && result->next())
	{
		sid_ = result->getInt("sid");
		name_ = result->getString("name").c_str();
		sex_ = result->getBoolean("sex");
		level_ = result->getInt("level");
		guild_id_ = result->getInt("guild_id");
		using_title_ = result->getInt("using_title");
		reputatuin_ = result->getInt("reputation");
		offline_time_ = result->getInt64("ott");
	}
	else
	{
		throw std::exception(FormatString("数据库找不到玩家，pid:", pid_).c_str());
	}
}

void CPlayerCache::__LoadDetailInfoFromDatabase()
{
	std::ostringstream sql;
	sql << "select * from tb_player_army where pid = " << pid();
	ResultSetPtr result_army{ MYSQL_QUERY(sql.str()) };

	std::string heroes_id;

	if (result_army && result_army->next())
	{
		technology_[kSoldierTechAtk] = result_army->getInt("tech_atk");
		technology_[kSoldierTechMatk] = result_army->getInt("tech_matk");
		technology_[kSoldierTechDef] = result_army->getInt("tech_def");
		technology_[kSoldierTechMdef] = result_army->getInt("tech_mdef");
		technology_[kSoldierTechHP] = result_army->getInt("tech_hp");

		sql.str("");
		sql << "(" << result_army->getInt("hero1") << "," << result_army->getInt("hero2") << "," << result_army->getInt("hero3") << ")";
		heroes_id = sql.str();
	}

	sql.str("");
	sql << "select * from tb_player_hero where pid = " << pid() << " and hid in " << heroes_id.c_str();
	ResultSetPtr result_heroes{ MYSQL_QUERY(sql.str()) };
	if (result_heroes)
	{
		size_t index{ 0 };

		while (result_heroes->next())
		{
			CHeroCard* cards{ new CHeroCard(result_heroes->getInt("hid")) };

			if (cards && cards->hero_id())
			{
				cards->level_ = result_heroes->getInt("level");
				cards->exp_ = result_heroes->getInt("exp");
				cards->quality_ = result_heroes->getInt("quality");
				cards->safety_times_ = result_heroes->getInt("safety_times");
				cards->trains_[kTrainStr] = result_heroes->getInt("train_str");
				cards->trains_[kTrainCmd] = result_heroes->getInt("train_cmd");
				cards->trains_[kTrainInt] = result_heroes->getInt("train_int");
				cards->set_today_is_internal_affair(result_heroes->getBoolean("today_affair"));
				auto it_runes = CSerializer<int>::ParseFromString(result_heroes->getString("runes").c_str());
				for (size_t i = 0; i < it_runes.size(); i++)
					cards->runes_[i] = it_runes[i];
				heroes_[index] = cards;

				index++;

				sql.str("");
				sql << "select * from tb_player_equip where pid =" << pid() << " and loaction = " << -cards->hero_id();
				ResultSetPtr result_equip{ MYSQL_QUERY(sql.str()) };
				if (result_equip)
				{
					while (result_equip->next())
					{
						time_t	get_time{ result_equip->getInt64("time") };
						int		location{ result_equip->getInt("loaction") };

						CEquip* equip{ new CEquip{ result_equip->getInt("id") } };
						if (0 == equip->GetID())
						{
							delete equip;
							continue;
						}

						equip->unique_id_ = 0;
						equip->level_ = result_equip->getInt("level");
						equip->quality_ = result_equip->getInt("quality");
						equip->suit_id_ = result_equip->getInt("suit_id");
						equip->enchanting1_.first = result_equip->getInt("enchanting1");
						equip->enchanting1_.second = (float)result_equip->getDouble("enchanting1_value");
						equip->enchanting2_.first = result_equip->getInt("enchanting2");
						equip->enchanting2_.second = (float)result_equip->getDouble("enchanting2_value");
						equip->enchanting2_is_active_ = result_equip->getBoolean("enchanting2_is_active");
						equip->get_time_ = get_time;

						auto it_gems = CSerializer<int>::ParseFromString(result_equip->getString("gem").c_str());
						for (size_t i = 0; i < it_gems.size(); ++i)
							equip->gems_[i] = it_gems[i];

						equips_[equip] = static_cast<ItemLocation>(location);
					}
				}
			}
			else
			{
				delete cards;
			}
		}
	}

	sql.str("");
	sql << "select id from tb_player_fashion where pid = " << pid() << " and is_on_body = true";
	ResultSetPtr result_fashion{ MYSQL_QUERY(sql.str()) };
	if (result_fashion && result_fashion->next())
		clothes_id_ = result_fashion->getInt("id");

	sql.str("");
	sql << "select speed_stage_record from tb_player_award where pid = " << pid();
	ResultSetPtr result_award{ MYSQL_QUERY(sql.str()) };
	if (result_award && result_award->next())
		speed_best_rank_ = result_award->getInt("speed_stage_record");

	sql.str("");
	sql << "select * from tb_player_record where pid = " << pid();
	ResultSetPtr result_record{ MYSQL_QUERY(sql.str()) };
	if (!result_record)
		throw 2;
	while (result_record->next())
	{
		int type = result_record->getInt("type");
		battle_record_[type].point = result_record->getInt("point");
		battle_record_[type].S = result_record->getInt("S");
		battle_record_[type].A = result_record->getInt("A");
		battle_record_[type].B = result_record->getInt("B");
		battle_record_[type].C = result_record->getInt("C");
		battle_record_[type].win = result_record->getInt("win");
		battle_record_[type].lose = result_record->getInt("lose");
	}

	sql.str("");
	sql << "select win_streak,bodyguard,last_time from tb_player_offline where pid = " << pid();
	ResultSetPtr result_offline{ MYSQL_QUERY(sql.str()) };
	if (result_offline && result_offline->next())
	{
		bodygurand_ = result_offline->getInt("bodyguard");
		offline_battle_win_streak_ = result_offline->getInt("win_streak");
		offline_battle_last_time_ = result_offline->getInt64("last_time");
	}

	sql.str("");
	sql << "select today_exercise_time from tb_player_exercise where pid = " << pid();
	ResultSetPtr result_exercise{ MYSQL_QUERY(sql.str()) };
	if (result_exercise && result_exercise->next())
		exercise_time_ = result_exercise->getInt("today_exercise_time");

	sql.str("");
	sql << "select week_war_times,week_1v1_times,week_3v3_times from tb_player_need_reset where pid = " << pid();
	ResultSetPtr result_reset{ MYSQL_QUERY(sql.str()) };
	if (result_reset && result_reset->next())
	{
		week_war_times_ = result_reset->getInt("week_war_times");
		week_1v1_times_ = result_reset->getInt("week_1v1_times");
		week_3v3_times_ = result_reset->getInt("week_3v3_times");
	}

	has_detail_ = true;
}