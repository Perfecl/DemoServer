#include "stdafx.h"
#include "Army.h"
#include "GameServer.h"
#include "HeroCard.h"
#include "Mission.h"
#include "GameWorld.h"
#include "HeroCard.h"
#include "Knapsack.h"
#include "Player.h"
#include "Equip.h"
#include "NeedReset.h"

CArmy::CArmy(CPlayer& player) :
player_{ player }
{
	hero_train_temp_.fill(0);
	__LoadFromDatabase();
}

CArmy::~CArmy()
{
	for (auto &it : hero_cards_)
		delete it.second;
	for (auto &it : soldiers_)
	{
		if (it.second)
			delete it.second;
	}
}

void CArmy::ProcessMessage(const CMessage& msg)
{
	switch (msg.GetProtocolID())
	{
	case PLAYER_C2S_REQ_SaveFormation:__SaveFormation(msg);				break;
	case PLAYER_C2S_REQ_TrainHero:__TrainHero(msg);						break;
	case PLAYER_C2S_REQ_SaveTrainHero:__SaveTrainHero(msg);				break;
	case PLAYER_C2S_REQ_RecruitHero:__RecruitHero(msg);					break;
	case PLAYER_CS2_REQ_UnitTechnologyLvUp:	__UpgradeTechnology(msg);	break;
	case PLAYER_C2S_REQ_BuyRunePage:__BuyRunePage();					break;
	case PLAYER_C2S_REQ_LockRune:__LockRune(msg);						break;
	case PLAYER_C2S_REQ_FullRuneEnergy:	__FullRuneEnergy();				break;
	case PLAYER_C2S_REQ_WashRune:__WashRune(msg);						break;
	case PLAYER_C2S_REQ_IntimacyGame:__IntimacyGame(msg);				break;
	case PLAYER_C2S_REQ_ExchangeHeroExp:__ExchangeHeroExp(msg);			break;
	case PLAYER_C2S_REQ_ClearExchangeCD:__ClearExchangeCD();			break;
	case PLAYER_C2S_REQ_UpgradeSoldierTrainLevel:__UpgradeSoldierTrainLevel(msg);	break;
	case PLAYER_C2S_REQ_CultivateHero:__CultivateHero(msg);				break;
	default:RECORD_WARNING(FormatString("未知的Army协议号:", msg.GetProtocolID()).c_str()); break;
	}
}

void CArmy::SaveToDatabase()
{
	std::string str_sql;

	CSerializer<int> ser_intimacy;
	for (auto &it : heroes_intimacy_)
		ser_intimacy.AddPair(it.first, it.second);

	std::ostringstream sql;
	sql << "update tb_player_army set tech_atk = " << soldiers_technology_[kSoldierTechAtk]
		<< ",tech_matk = " << soldiers_technology_[kSoldierTechMatk]
		<< ",tech_def = " << soldiers_technology_[kSoldierTechDef]
		<< ",tech_mdef = " << soldiers_technology_[kSoldierTechMdef]
		<< ",tech_hp = " << soldiers_technology_[kSoldierTechHP]
		<< ",is_in_practice_cd = " << is_in_practice_cd_
		<< ",parctice_seconds = " << practice_seconds_
		<< ",last_parctice_time = " << last_practice_time_
		<< ",rune_page = " << rune_page_
		<< ",rune_energy = " << rune_energy_
		<< ",hero_intimacy = '" << ser_intimacy.SerializerPairToString().c_str()
		<< "',hero1 = " << formation_heroes[0]
		<< ",hero2 = " << formation_heroes[1]
		<< ",hero3 = " << formation_heroes[2]
		<< ",soldier1 = " << formation_soldiers[0]
		<< ",soldier2 = " << formation_soldiers[1]
		<< ",soldier3 = " << formation_soldiers[2]
		<< ",soldier4 = " << formation_soldiers[3]
		<< ",soldier5 = " << formation_soldiers[4]
		<< ",soldier6 = " << formation_soldiers[5]
		<< " where pid = " << player_.pid();
	MYSQL_UPDATE(sql.str());

	if (!soldiers_.empty())
	{
		sql.str("");
		sql << "replace into tb_player_soldier values";
		for (auto &it : soldiers_)
			sql << "(" << player_.pid() << ", " << it.first << ", " << it.second->soul_level_ << ", " << it.second->soul_exp_ << ", " << it.second->train_level_ << "),";
		str_sql = std::move(sql.str());
		str_sql.erase(str_sql.length() - 1, 1);
		MYSQL_UPDATE(str_sql);
	}

	if (!hero_cards_.empty())
	{
		sql.str("");
		sql << "replace into tb_player_hero values";

		for (auto &it : hero_cards_)
		{
			CSerializer<int> ser_runes;
			for (auto &rune : it.second->runes_)
				ser_runes.AddValue(rune);
			sql << "(" << player_.pid() << ", " << it.second->hero_id() << ", " << it.second->hero_level() << ", " << it.second->exp_ << ", " << it.second->quality_ << ", "
				<< it.second->safety_times_ << "," << it.second->train_str() << "," << it.second->train_cmd() << "," << it.second->train_int() << "," << it.second->today_is_internal_affair() << ",'" << ser_runes.SerializerToString().c_str() << "'),";
		}

		str_sql = std::move(sql.str());
		str_sql.erase(str_sql.length() - 1, 1);
		MYSQL_UPDATE(str_sql);
	}
}

void CArmy::SendInitialMessage()
{
	std::string str;

	//士兵
	pto_PLAYER_S2C_NTF_UpdatePlayeSoldierDepot pto_soldiers;
	auto it_soldiers = pto_soldiers.mutable_player_soldier_depot();
	for (auto &it : soldiers_)
	{
		auto soldier = it_soldiers->add_soldier_model();
		soldier->set_soldier_model_id(it.first);
		soldier->set_soul_level(it.second->soul_level_);
		soldier->set_soul_exp(it.second->soul_exp_);
		soldier->set_train_level(it.second->train_level_);
	}
	player_.SendToAgent(pto_soldiers, PLAYER_S2C_NTF_UpdatePlayerSoldierDepot);

	//英雄
	pto_PLAYER_S2C_NTF_UpdatePlayeHeroDepot pto_heroes;
	auto it_heroes = pto_heroes.mutable_player_hero_depot();
	for (auto &itHero : hero_cards_)
	{
		auto hero_model = it_heroes->add_player_hero_model();
		hero_model->set_hero_model_id(itHero.second->hero_id());
		hero_model->set_level(itHero.second->level_);
		hero_model->set_exp(itHero.second->exp_);
		hero_model->set_train_str(itHero.second->trains_[kTrainStr]);
		hero_model->set_train_cmd(itHero.second->trains_[kTrainCmd]);
		hero_model->set_train_int(itHero.second->trains_[kTrainInt]);
		hero_model->set_quality(itHero.second->quality());
		hero_model->set_safety_times(itHero.second->safety_times());

		for (auto &rune : itHero.second->runes_)
		{
			auto pto_rune = hero_model->add_rune();
			pto_rune->set_rune_id(CRuneLibrary::GetRuneID(rune));

			if (pto_rune->rune_id() != Broke_Rune_ID)
				pto_rune->set_colour(CRuneLibrary::GetRuneColor(rune));
			else
				pto_rune->set_colour(0);

			pto_rune->set_lock(CRuneLibrary::HasLock(rune));
		}

		hero_model->set_used(itHero.second->today_is_internal_affair());
	}

	for (auto &it : heroes_intimacy_)
	{
		auto intimacy = pto_heroes.add_hero_intimacy();
		intimacy->set_hero_id(it.first);
		intimacy->set_intimacy(it.second);
	}

	if (practice_seconds_ <= static_cast<int>(time(0) - last_practice_time_))
	{
		practice_seconds_ = 0;
		is_in_practice_cd_ = false;
	}
	else
	{
		practice_seconds_ -= static_cast<int>(time(0) - last_practice_time_);
	}
	pto_heroes.set_exchange_exp_cd(practice_seconds_);
	pto_heroes.set_in_exchange_cd(is_in_practice_cd_);

	pto_heroes.set_rune_page(rune_page_);
	pto_heroes.set_rune_energy(rune_energy_);

	player_.SendToAgent(pto_heroes, PLAYER_S2C_NTF_UpdatePlayeHeroDepot);

	//军阵
	pto_PLAYER_S2C_NTF_UpdatePlayerFormation pto_formations;
	pto_formations.set_using_formation(1);
	auto formation = pto_formations.add_formations();
	formation->set_id(1);
	formation->set_name("default");
	for (auto &it : formation_heroes)
		formation->add_hero_id(it);
	for (auto &it : formation_soldiers)
		formation->add_soldier_id(it);
	player_.SendToAgent(pto_formations, PLAYER_S2C_NTF_UpdatePlayerFormation);

	//科技
	pto_PLAYER_S2C_NTF_UpdateUnitTechnology pto_technology;
	pto_technology.add_unit_sciencd(soldiers_technology_[kSoldierTechAtk]);
	pto_technology.add_unit_sciencd(soldiers_technology_[kSoldierTechMatk]);
	pto_technology.add_unit_sciencd(soldiers_technology_[kSoldierTechDef]);
	pto_technology.add_unit_sciencd(soldiers_technology_[kSoldierTechMdef]);
	pto_technology.add_unit_sciencd(soldiers_technology_[kSoldierTechHP]);
	player_.SendToAgent(pto_technology, PLAYER_S2C_NTF_UpdateUnitTechnology);
}

void CArmy::SetAllHeroesLevel(int level)
{
	for (auto &it : hero_cards_)
		it.second->level_ = level;
}

void CArmy::AddSoldier(int soldier_id)
{
	const CSoldierTP* soldier_tp{ CSoldierTP::GetSoldierTP(soldier_id) };

	if (soldier_tp)
	{
		LOCK_BLOCK(lock_);

		SoldierData* soldier_data = new SoldierData;

		if (soldiers_.insert(std::make_pair(soldier_id, soldier_data)).second)
		{
			pto_PLAYER_S2C_NTF_GiveNewSoldier pto;
			pto.set_id(soldier_id);

			player_.SendToAgent(pto, PLAYER_S2C_NTF_GiveNewSoldier);
		}
	}
}

void CArmy::GiveAllSoldier()
{
	for (size_t i = 1; i <= 15; ++i)
		AddSoldier(i);

	AddSoldier(18);
	AddSoldier(20);
	AddSoldier(23);
	AddSoldier(24);
	AddSoldier(27);
	AddSoldier(28);
	AddSoldier(29);
	AddSoldier(30);
	AddSoldier(31);
}

CHeroCard*	CArmy::FindHero(int hero_id)
{
	LOCK_BLOCK(lock_);

	auto it = hero_cards_.find(hero_id);

	if (hero_cards_.cend() == it)
		return nullptr;
	else
		return it->second;
}

void CArmy::GiveAllHero()
{
	std::vector<int> temp{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 18, 19, 21, 22, 26, 41, 42, 43, 44 };

	LOCK_BLOCK(lock_);
	for (auto &it : temp)
		AddHero(it, true);
}

void CArmy::AddHero(int hero_id, bool send_msg)
{
	CHeroCard* hero{ new CHeroCard{ hero_id } };

	LOCK_BLOCK(lock_);
	if (nullptr == hero->hero_template_ || false == hero_cards_.insert(std::make_pair(hero_id, hero)).second)
	{
		delete hero;
		return;
	}

	if (send_msg)
	{
		pto_PLAYER_S2C_RES_RecruitHero pto;
		pto.set_res(0);
		pto.set_id(hero_id);
		player_.SendToAgent(pto, PLAYER_S2C_RES_RecruitHero);
	}
}

void CArmy::ResetHeroInternal()
{
	LOCK_BLOCK(lock_);
	for (auto &it : hero_cards_)
		it.second->today_is_internal_affair_ = false;
}

void CArmy::SetAllTechnology(int level)
{
	if (level > CPlayer::kMaxLevel)
		level = CPlayer::kMaxLevel;
	if (level < 0)
		level = 0;
	for (size_t i = 0; i < soldiers_technology_.size(); ++i)
	{
		soldiers_technology_[i] = level;
		pto_PLAYER_S2C_RES_UnitTechnologyLvUp pto_res;
		pto_res.set_technology_lv(soldiers_technology_[i]);
		pto_res.set_technology_type(static_cast<SoldierTechnology>(i));
		pto_res.set_res(0);
		player_.SendToAgent(pto_res, PLAYER_S2C_RES_UnitTechnologyLvUp);
	}
}

void CArmy::SetHeroesAndSoldiersProtocol(pto_BATTLE_STRUCT_Master_Ex *pto, bool is_fair)
{
	LOCK_BLOCK(lock_);

	for (auto &it : formation_heroes)
	{
		CHeroCard*	hero_card = FindHero(it);
		if (hero_card)
		{
			pto_BATTLE_STRUCT_Hero_Ex* pto_hero = pto->add_heroes();
			__SetHeroeProtocol(hero_card, pto_hero, is_fair);
		}
	}

	for (auto &it : formation_soldiers)
		if (it)
			pto->add_using_soldiers(it);

	if (player_.knapsack()->body_fashion())
	{
		const CFashionTP* fashion = CFashionTP::GetFashionTP(player_.knapsack()->body_fashion()->id);
		if (fashion)
		{
			pto->add_soldeir_ex(fashion->hero_atk());
			pto->add_soldeir_ex(fashion->hero_matk());
			pto->add_soldeir_ex(fashion->hero_def());
			pto->add_soldeir_ex(fashion->hero_mdef());
			pto->add_soldeir_ex(fashion->hero_hp());
		}
	}

	if (is_fair)
	{
		pto->add_technology(CAddition::fair_soldier_atk());
		pto->add_technology(CAddition::fair_soldier_matk());
		pto->add_technology(CAddition::fair_soldier_def());
		pto->add_technology(CAddition::fair_soldier_mdef());
		pto->add_technology(CAddition::fair_soldier_hp());
		pto->add_castle_upgrade(int((CInteriorLib::castle_atk_mul() * CUnitTP::GetBaseATK()) + (CInteriorLib::castle_atk_mul() * CAddition::GetSoldierAddition(50, AdditionType::kAdditionTypeUnitAtk))));
		pto->add_castle_upgrade(int((CInteriorLib::castle_matk_mul() * CUnitTP::GetBaseMATK()) + (CInteriorLib::castle_atk_mul() * CAddition::GetSoldierAddition(50, AdditionType::kAdditionTypeUnitMAtk))));
		pto->add_castle_upgrade(0);

		for (auto &it : soldiers_)
		{
			auto pto_soldier = pto->add_soldiers();

			pto_soldier->set_soldier_id(it.first);
			pto_soldier->set_soul_level(it.second->soul_level_);
			pto_soldier->set_soul_exp(it.second->soul_exp_);
			pto_soldier->set_train_level(100);
		}
	}
	else
	{
		pto->add_technology(CAddition::GetSoldierAddition(soldier_technology(SoldierTechnology::kSoldierTechAtk), AdditionType::kAdditionTypeUnitAtk));
		pto->add_technology(CAddition::GetSoldierAddition(soldier_technology(SoldierTechnology::kSoldierTechMatk), AdditionType::kAdditionTypeUnitMAtk));
		pto->add_technology(CAddition::GetSoldierAddition(soldier_technology(SoldierTechnology::kSoldierTechDef), AdditionType::kAdditionTypeUnitDef));
		pto->add_technology(CAddition::GetSoldierAddition(soldier_technology(SoldierTechnology::kSoldierTechMdef), AdditionType::kAdditionTypeUnitMDef));
		pto->add_technology(CAddition::GetSoldierAddition(soldier_technology(SoldierTechnology::kSoldierTechHP), AdditionType::kAdditionTypeUnitHP));

		pto->add_castle_upgrade(int((CInteriorLib::castle_atk_mul() * CUnitTP::GetBaseATK()) + (CInteriorLib::castle_atk_mul() * CAddition::GetSoldierAddition(player_.need_reset()->castle_upgrade(CastleUpgrade::kCastleUpgradeAtk), AdditionType::kAdditionTypeUnitAtk))));
		pto->add_castle_upgrade(int((CInteriorLib::castle_matk_mul() * CUnitTP::GetBaseMATK()) + (CInteriorLib::castle_atk_mul() * CAddition::GetSoldierAddition(player_.need_reset()->castle_upgrade(CastleUpgrade::kCastleUpgradeMAtk), AdditionType::kAdditionTypeUnitMAtk))));
		pto->add_castle_upgrade(int(player_.need_reset()->castle_upgrade(CastleUpgrade::kCastleUpgradeMAtk) * CInteriorLib::castle_hp_mul()));

		for (auto &it : soldiers_)
		{
			auto pto_soldier = pto->add_soldiers();

			pto_soldier->set_soldier_id(it.first);
			pto_soldier->set_soul_level(it.second->soul_level_);
			pto_soldier->set_soul_exp(it.second->soul_exp_);
			pto_soldier->set_train_level(it.second->train_level_ * 2);
		}
	}
}

int	 CArmy::CalculateHeroRank(const CHeroCard* hero)
{
	const CHeroTP* pHeroTP = CHeroTP::GetHeroTP(hero->hero_id());
	if (nullptr == pHeroTP)
		return 0;

	if (hero->train_str() == 0 && hero->train_int() == 0 && hero->train_cmd() == 0)
		return 0;

	int nMaxNum = hero->hero_level() * 2 + 20;
	int nTrainNum = (hero->train_str() + hero->train_int() + hero->train_cmd()) / 3;
	int nResult = nMaxNum - nTrainNum;

	if (nResult <= 0)
		return 6;
	if (nResult <= 5)
		return 5;
	else if (nResult <= 15)
		return 4;
	else if (nResult <= 25)
		return 3;
	else if (nResult <= 40)
		return 2;
	else
		return 1;
}

void CArmy::OpenRunePage(int level)
{
	bool is_open{ false };
	if (level >= 80 && rune_page_ < 3)
	{
		rune_page_ = 3;
		is_open = true;
	}

	else if (level >= 60 && rune_page_ < 2)
	{
		rune_page_ = 2;
		is_open = true;
	}

	else if (level >= 40 && rune_page_ < 1)
	{
		rune_page_ = 1;
		is_open = true;
	}

	if (is_open)
	{
		pto_PLAYER_S2C_RES_BuyRunePage pto;
		pto.set_res(0);
		pto.set_open_page(rune_page_);
		player_.SendToAgent(pto, PLAYER_S2C_RES_BuyRunePage);
	}
}

void CArmy::GiveAllRunes(int rune_id)
{
	if (rune_page_ < 3)
	{
		rune_page_ = 3;

		pto_PLAYER_S2C_RES_BuyRunePage pto;
		pto.set_res(0);
		pto.set_open_page(rune_page_);
		player_.SendToAgent(pto, PLAYER_S2C_RES_BuyRunePage);
	}

	for (auto &it : hero_cards_)
	{
		for (int i = 0; i < 4; i++)
		{
			pto_PLAYER_S2C_RES_WashRune pto;
			pto.set_hero_id(it.second->hero_id());
			pto.set_page(i);

			int start_pos = i * 4;

			for (int j = start_pos; j < start_pos + 4; j++)
			{
				unsigned old_rune = it.second->runes_[j];

				if (!CRuneLibrary::HasLock(old_rune))
				{
					unsigned new_rune = it.second->runes_[j];

					if (!CRuneLibrary::HasLock(new_rune))
					{
						int give_rune_id = rune_id;

						if (0 == give_rune_id)
						{
							switch (it.second->hero_template_->GetType())
							{
							case HT_ATK:give_rune_id = 40; break;
							case HT_DEF:give_rune_id = 40; break;
							case HT_INT:give_rune_id = 41; break;
							case HT_SKILL:give_rune_id = 41; break;
							}
						}

						new_rune = CRuneLibrary::TestWash(CRuneLibrary::GetRuneCellColour(j), give_rune_id);

						it.second->runes_[j] = new_rune;

						CRuneLibrary::SetLock(new_rune, false);
					}

					auto pto_rune = pto.add_runes();

					pto_rune->set_rune_id(CRuneLibrary::GetRuneID(new_rune));

					if (pto_rune->rune_id() != Broke_Rune_ID)
						pto_rune->set_colour(CRuneLibrary::GetRuneColor(new_rune));
					else
						pto_rune->set_colour(0);
					pto_rune->set_lock(CRuneLibrary::HasLock(new_rune));
				}
			}

			pto.set_rune_engery(rune_energy_);
			pto.set_res(0);
			player_.SendToAgent(pto, PLAYER_S2C_RES_WashRune);
		}
	}
}

void CArmy::__SetHeroeProtocol(CHeroCard* hero_card, pto_BATTLE_STRUCT_Hero_Ex* pto_hero, bool is_fair)
{
	if (!hero_card->hero_template())
		return;

	float hp_mul = CHeroTP::GetHeroTypeAddition(hero_card->hero_template()->GetType(), 0);
	float atk_mul = CHeroTP::GetHeroTypeAddition(hero_card->hero_template()->GetType(), 1);
	float def_mul = CHeroTP::GetHeroTypeAddition(hero_card->hero_template()->GetType(), 2);
	float matk_mul = CHeroTP::GetHeroTypeAddition(hero_card->hero_template()->GetType(), 3);
	float mdef_mul = CHeroTP::GetHeroTypeAddition(hero_card->hero_template()->GetType(), 4);

	pto_hero->set_hero_id(hero_card->hero_id());

	int hero_level = hero_card->hero_level();
	pto_hero->set_level(hero_level);

	int color = CArmy::CalculateHeroRank(hero_card);
	pto_hero->set_color(hero_card->quality());

	int equip_atk{ 0 };
	int equip_def{ 0 };
	int equip_hp{ 0 };

	int final_str{ 0 };
	int final_int{ 0 };
	int final_cmd{ 0 };
	int exsits_time{ 60000 };

	int final_atk{ 0 };
	int final_matk{ 0 };
	int final_def{ 0 };
	int final_mdef{ 0 };
	int final_hp{ 0 };

	float final_move_speed{ 0 };
	float final_crit_odds{ 0 };
	float final_prevent_crit{ 0 };
	float final_atk_speed_{ 0 };
	int suit_skill_id{ 0 };

	const CHeroQuality* quality = CHeroQuality::GetHeroQuality(hero_card->quality());
	if (quality)
	{
		exsits_time += quality->time();
	}

	if (is_fair)	//公平数值
	{
		final_str = final_int = final_cmd = 150;
		equip_atk = CAddition::fair_equip_atk();
		equip_def = CAddition::fair_equip_def();
		equip_hp = CAddition::fair_equip_hp();

		final_atk = (int)((hero_card->hero_template()->GetBaseATK() + equip_atk) * atk_mul);
		final_matk = (int)((hero_card->hero_template()->GetBaseATK() + equip_atk) * matk_mul);
		final_def = (int)((hero_card->hero_template()->GetBaseDEF() + equip_def) * def_mul);
		final_mdef = (int)((hero_card->hero_template()->GetBaseDEF() + equip_def) * mdef_mul);
		final_hp = (int)((hero_card->hero_template()->GetBaseHP() + equip_hp) * hp_mul);
	}
	else
	{
		SuitAddition suit_addition;
		__GetSuitAddtion(hero_card, &suit_addition);
		suit_skill_id = suit_addition.skill_id;
		final_atk_speed_ = suit_addition.atk_speed_;
		final_move_speed = suit_addition.move_speed_;
		final_prevent_crit = suit_addition.prevent_crit_;
		final_crit_odds = suit_addition.crit_odds_;

		final_str = hero_card->hero_template()->GetStr() + hero_card->train_str() + suit_addition.str_;
		final_int = hero_card->hero_template()->GetInt() + hero_card->train_int() + suit_addition.int_;
		final_cmd = hero_card->hero_template()->GetCmd() + hero_card->train_cmd() + suit_addition.cmd_;

		for (auto &it : player_.knapsack()->GetHeroEquip(hero_card->hero_id()))
		{
			switch (it->equip_template()->attribute_type())
			{
			case 1:
				equip_atk += CAddition::GetHeroEquipAddition(it->level(), it->equip_template(), it->quality());
				break;
			case 2:
				equip_def += CAddition::GetHeroEquipAddition(it->level(), it->equip_template(), it->quality());
				break;
			case 3:
				equip_hp += CAddition::GetHeroEquipAddition(it->level(), it->equip_template(), it->quality());
				break;
			default:
				break;
			}
		}

		final_atk = static_cast<int>(((hero_card->hero_template()->GetBaseATK() + equip_atk) * atk_mul) + (hero_card->hero_template()->GetGrowupAtk() * (hero_level - 1)) + suit_addition.atk_);
		final_matk = static_cast<int>(((hero_card->hero_template()->GetBaseATK() + equip_atk) * matk_mul) + (hero_card->hero_template()->GetGrowupMAtk() * (hero_level - 1)) + suit_addition.atk_);
		final_def = static_cast<int>(((hero_card->hero_template()->GetBaseDEF() + equip_def) * def_mul) + (hero_card->hero_template()->GetGrowupDef() * (hero_level - 1)) + suit_addition.def_);
		final_mdef = static_cast<int>(((hero_card->hero_template()->GetBaseDEF() + equip_def) * mdef_mul) + (hero_card->hero_template()->GetGrowupMDef() * (hero_level - 1)) + suit_addition.def_);
		final_hp = static_cast<int>(((hero_card->hero_template()->GetBaseHP() + equip_hp) * hp_mul) + (hero_card->hero_template()->GetGrowupHP() * (hero_level - 1)) + suit_addition.hp_);

		for (auto &it : player_.knapsack()->GetHeroEquip(hero_card->hero_id()))
		{
			switch (it->enchanting1())
			{
			case kEnchantingTypeAtk:			final_atk += (int)it->enchanting1_value();			break;
			case kEnchantingTypeMAtk:			final_matk += (int)it->enchanting1_value();			break;
			case kEnchantingTypeDef:			final_def += (int)it->enchanting1_value();			break;
			case kEnchantingTypeMDef:			final_mdef += (int)it->enchanting1_value();			break;
			case kEnchantingTypeHP:				final_hp += (int)it->enchanting1_value();			break;
			case kEnchantingTypeAtkSpeed:		final_atk_speed_ += (int)it->enchanting1_value();	break;
			case kEnchantingTypeCritOdds:		final_crit_odds += (int)it->enchanting1_value();		break;
			case kEnchantingTypePreventCrit:	final_prevent_crit += (int)it->enchanting1_value();	break;
			case kEnchantingTypeTime:			exsits_time += (int)it->enchanting1_value();			break;
			default:break;
			}
			if (it->enchanting2_is_active())
			{
				switch (it->enchanting2())
				{
				case kEnchantingTypeAtk:			final_atk += (int)it->enchanting2_value();			break;
				case kEnchantingTypeMAtk:			final_matk += (int)it->enchanting2_value();			break;
				case kEnchantingTypeDef:			final_def += (int)it->enchanting2_value();			break;
				case kEnchantingTypeMDef:			final_mdef += (int)it->enchanting2_value();			break;
				case kEnchantingTypeHP:				final_hp += (int)it->enchanting2_value();			break;
				case kEnchantingTypeAtkSpeed:		final_atk_speed_ += (int)it->enchanting2_value();	break;
				case kEnchantingTypeCritOdds:		final_crit_odds += (int)it->enchanting2_value();	break;
				case kEnchantingTypePreventCrit:	final_prevent_crit += (int)it->enchanting2_value();	break;
				case kEnchantingTypeTime:			exsits_time += (int)it->enchanting2_value();		break;
				default:break;
				}
			}
		}

		exsits_time += suit_addition.max_time_;

		if (quality)
		{
			final_atk += quality->atk();
			final_matk += quality->matk();
			final_def += quality->def();
			final_mdef += quality->mdef();
			final_hp += quality->hp();
			final_str += quality->strength();
			final_cmd += quality->command();
			final_int += quality->intelligence();
		}
	}

	if (player_.knapsack()->body_fashion())
	{
		const CFashionTP* fashion = CFashionTP::GetFashionTP(player_.knapsack()->body_fashion()->id);
		if (fashion)
		{
			final_atk += int(fashion->hero_atk() * atk_mul);
			final_matk += int(fashion->hero_matk() * matk_mul);
			final_def += int(fashion->hero_def() * def_mul);
			final_mdef += int(fashion->hero_mdef() * mdef_mul);
			final_hp += int(fashion->hero_hp() * hp_mul);
		}
	}

	RuneAddition rune_addition;
	CRuneLibrary::CalculateRuneAddition(&rune_addition, hero_card->rune());

	pto_hero->set_atk(rune_addition.m_nAtk + final_atk);
	pto_hero->set_matk(rune_addition.m_nMAtk + final_matk);
	pto_hero->set_def(rune_addition.m_nDef + final_def);
	pto_hero->set_mdef(rune_addition.m_nMDef + final_mdef);
	pto_hero->set_hp(rune_addition.m_nHP + final_hp);
	pto_hero->set_move_speed((hero_card->hero_template()->move_speed() + rune_addition.m_fMoveSpeed + final_move_speed));

	float fAtkSpeed = rune_addition.m_fAtkSpeed + final_atk_speed_;
	int m_nAtkInterval = hero_card->hero_template()->atk_interval();
	pto_hero->set_atk_interval(static_cast<int>(m_nAtkInterval - m_nAtkInterval * (fAtkSpeed / (1 + fAtkSpeed))));

	int m_nAtkPrev = hero_card->hero_template()->atk_prev();
	pto_hero->set_prev_atk(static_cast<int>(m_nAtkPrev - m_nAtkPrev * (fAtkSpeed / (1 + fAtkSpeed))));

	pto_hero->set_exsits_time(static_cast<int>(exsits_time + rune_addition.m_nMaxTime));
	pto_hero->set_crit_odds(hero_card->hero_template()->crit() + rune_addition.m_fCrit + final_crit_odds);
	pto_hero->set_crit_value(hero_card->hero_template()->crit_value());
	pto_hero->set_crit_resistance(rune_addition.m_fPreventCrit + final_prevent_crit);
	pto_hero->set_strength(final_str + rune_addition.m_nAddStr);
	pto_hero->set_command(final_cmd + rune_addition.m_nAddCmd);
	pto_hero->set_intelligence(final_int + rune_addition.m_nAddInt);
	pto_hero->set_suit_effect(suit_skill_id);
	pto_hero->set_building_atk(hero_card->hero_template()->GetAtkBuildingDamage() + rune_addition.m_nBulidingAtk);
}

void CArmy::__TrainHero(CHeroCard* hero, int ratio, int& train_str, int& train_cmd, int& train_int)
{
	const CHeroTP* pHeroTP = CHeroTP::GetHeroTP(hero->hero_id());

	if (!pHeroTP)
		return;

	int max = (hero->level_ - 30) * 2 + 20;

	HERO_TYPE enHeroType = pHeroTP->GetType();

	int nTrainLevel;

	if (HT_ATK == enHeroType)
	{
		nTrainLevel = __GetTrainLevel(hero, TrainValueType::kStr);

		if (__TrainValue(nTrainLevel, ratio + 50))
		{
			train_str = __GetTrainAdd(nTrainLevel);
		}
		else
		{
			train_str = 0 - __GetTrainReduce(nTrainLevel);
		}

		nTrainLevel = __GetTrainLevel(hero, TrainValueType::kCmd);
		if (__TrainValue(nTrainLevel, ratio))
		{
			train_cmd = __GetTrainAdd(nTrainLevel);
		}
		else
		{
			train_cmd = 0 - __GetTrainReduce(nTrainLevel);
		}

		nTrainLevel = __GetTrainLevel(hero, TrainValueType::kInt);
		if (__TrainValue(nTrainLevel, ratio))
		{
			train_int = __GetTrainAdd(nTrainLevel);
		}
		else
		{
			train_int = 0 - __GetTrainReduce(nTrainLevel);
		}
	}
	else if (HT_DEF == enHeroType)
	{
		nTrainLevel = __GetTrainLevel(hero, TrainValueType::kCmd);

		if (__TrainValue(nTrainLevel, ratio + 50))
		{
			train_cmd = __GetTrainAdd(nTrainLevel);
		}
		else
		{
			train_cmd = 0 - __GetTrainReduce(nTrainLevel);
		}

		nTrainLevel = __GetTrainLevel(hero, TrainValueType::kStr);
		if (__TrainValue(nTrainLevel, ratio))
		{
			train_str = __GetTrainAdd(nTrainLevel);
		}
		else
		{
			train_str = 0 - __GetTrainReduce(nTrainLevel);
		}

		nTrainLevel = __GetTrainLevel(hero, TrainValueType::kInt);
		if (__TrainValue(nTrainLevel, ratio))
		{
			train_int = __GetTrainAdd(nTrainLevel);
		}
		else
		{
			train_int = 0 - __GetTrainReduce(nTrainLevel);
		}
	}
	else if (HT_INT == enHeroType)
	{
		nTrainLevel = __GetTrainLevel(hero, TrainValueType::kInt);

		if (__TrainValue(nTrainLevel, ratio + 50))
		{
			train_int = __GetTrainAdd(nTrainLevel);
		}
		else
		{
			train_int = 0 - __GetTrainReduce(nTrainLevel);
		}

		nTrainLevel = __GetTrainLevel(hero, TrainValueType::kStr);
		if (__TrainValue(nTrainLevel, ratio))
		{
			train_str = __GetTrainAdd(nTrainLevel);
		}
		else
		{
			train_str = 0 - __GetTrainReduce(nTrainLevel);
		}

		nTrainLevel = __GetTrainLevel(hero, TrainValueType::kCmd);
		if (__TrainValue(nTrainLevel, ratio))
		{
			train_cmd = __GetTrainAdd(nTrainLevel);
		}
		else
		{
			train_cmd = 0 - __GetTrainReduce(nTrainLevel);
		}
	}
	else if (HT_SKILL == enHeroType)
	{
		nTrainLevel = __GetTrainLevel(hero, TrainValueType::kInt);

		if (__TrainValue(nTrainLevel, ratio + 25))
		{
			train_int = __GetTrainAdd(nTrainLevel);
		}
		else
		{
			train_int = 0 - __GetTrainReduce(nTrainLevel);
		}

		nTrainLevel = __GetTrainLevel(hero, TrainValueType::kStr);

		if (__TrainValue(nTrainLevel, ratio + 25))
		{
			train_str = __GetTrainAdd(nTrainLevel);
		}
		else
		{
			train_str = 0 - __GetTrainReduce(nTrainLevel);
		}

		nTrainLevel = __GetTrainLevel(hero, TrainValueType::kCmd);
		if (__TrainValue(nTrainLevel, ratio))
		{
			train_cmd = __GetTrainAdd(nTrainLevel);
		}
		else
		{
			train_cmd = 0 - __GetTrainReduce(nTrainLevel);
		}
	}

	//最终修正值
	if (hero->train_str() + train_str > max) train_str = max - hero->train_str();
	if (hero->train_cmd() + train_cmd > max) train_cmd = max - hero->train_cmd();
	if (hero->train_int() + train_int > max) train_int = max - hero->train_int();

	if (hero->train_str() + train_str < 0) train_str = -hero->train_str();
	if (hero->train_cmd() + train_cmd < 0) train_cmd = -hero->train_cmd();
	if (hero->train_int() + train_int < 0) train_int = -hero->train_int();
}

bool CArmy::__TrainValue(int nTrainLevel, int nRatio)
{
	int nRand = GetRandom(1, 1000);
	bool bRes{ false };

	switch (nTrainLevel)
	{
	case 1:
		if (nRand < 100 + nRatio)
			bRes = true;
		break;
	case 2:
		if (nRand < 150 + nRatio)
			bRes = true;
		break;
	case 3:
		if (nRand < 250 + nRatio)
			bRes = true;
		break;
	case 4:
		if (nRand < 400 + nRatio)
			bRes = true;
		break;
	case 5:
		if (nRand < 550 + nRatio)
			bRes = true;
		break;
	case 6:
		bRes = true;
		break;
	}

	return bRes;
}

int	CArmy::__GetTrainAdd(int nTrainLevel)
{
	int nRes{ 0 };

	switch (nTrainLevel)
	{
	case 1:
		nRes = 1;
		break;
	case 2:
		nRes = GetRandom(1, 2);
		break;
	case 3:
		nRes = GetRandom(1, 3);
		break;
	case 4:
		nRes = GetRandom(1, 5);
		break;
	case 5:
	case 6:
		nRes = GetRandom(1, 10);
		break;
	}

	return nRes;
}

int	CArmy::__GetTrainReduce(int nTrainLevel)
{
	int nRes{ 0 };

	switch (nTrainLevel)
	{
	case 1:
		nRes = GetRandom(0, 12);
		break;
	case 2:
		nRes = GetRandom(0, 8);
		break;
	case 3:
		nRes = GetRandom(0, 5);
		break;
	case 4:
		nRes = GetRandom(0, 3);
		break;
	case 5:
		nRes = GetRandom(0, 1);
		break;
	}

	return nRes;
}

int CArmy::__RandomExp()
{
	/*概率:
	70%  3~5星
	25%  10~20星 (小暴击)
	5%   30~50星 (大暴击) - by Lux 2014.11.18*/

	int exp_chance[3]{ 70, 25, 5 };

	int nChance = GetRandom(0, 100);
	if (nChance < exp_chance[0])
	{
		int nIndex = GetRandom(3, 5);
		return nIndex;
	}
	else if (nChance < (exp_chance[0] + exp_chance[1]))
	{
		int nIndex = GetRandom(10, 20);
		return nIndex;
	}
	else
	{
		int nIndex = GetRandom(30, 50);
		return nIndex;
	}
}

void CArmy::__SaveFormation(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_SaveFormation pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	auto formation = pto.formation();

	int result{ 0 };
	if (formation.id() != 1) result = 6;
	if (formation.hero_id_size() != 3) result = 6;
	if (formation.soldier_id_size() != 6) result = 6;
	//是否武将重复
	if (0 == result)
	{	
		for (int i = 0; i < formation.hero_id_size(); i++)
		{
			for (int j = i + 1; j < formation.hero_id_size(); j++)
			{
				if ((formation.hero_id(i) == formation.hero_id(j)) && formation.hero_id(i) != 0)
				{
					result = 6;
					break;
				}
			}
		}
	}
	//是否士兵重复
	if (0 == result)
	{	
		for (int i = 0; i < formation.soldier_id_size(); i++)
		{
			for (int j = i + 1; j < formation.soldier_id_size(); j++)
			{
				if ((formation.soldier_id(i) == formation.soldier_id(j)) && formation.soldier_id(i) != 0)
				{
					result = 6;
					break;
				}
			}
		}
	}
	//是否拥有士兵
	int elite_soldier_num{ 0 };	//精英兵数量
	if (0 == result)
	{
		for (int i = 0; i < formation.soldier_id_size(); i++)
		{
			int soldier_id{ formation.soldier_id(i) };

			if (soldier_id)
			{
				if (soldiers_.cend() != soldiers_.find(soldier_id))
				{
					if (SOLDIER_TYPE::ELITE == CSoldierTP::GetSoldierTP(soldier_id)->GetType() ||
						SOLDIER_TYPE::STRATEGY == CSoldierTP::GetSoldierTP(soldier_id)->GetType())
						++elite_soldier_num;
				}
				else
				{
					result = 2;
					break;
				}
			}
		}

		if (elite_soldier_num > 3)
			result = 5;
	}

	if (0 == result)
	{
		//if (player_->OpenGuid(2))
		{
			formation_heroes[0] = formation.hero_id(0);
			formation_heroes[1] = formation.hero_id(1);
			formation_heroes[2] = formation.hero_id(2);
		}

		//if (player_->OpenGuid(4))
		{
			formation_soldiers[0] = formation.soldier_id(0);
			formation_soldiers[1] = formation.soldier_id(1);
			formation_soldiers[2] = formation.soldier_id(2);
			formation_soldiers[3] = formation.soldier_id(3);
			formation_soldiers[4] = formation.soldier_id(4);
			formation_soldiers[5] = formation.soldier_id(5);
		}

		for (int i = 0; i < kMaxSoldierMun; i++)
			player_.ImplementMission(EMissionTargetType::MTT_UseSoldier, formation.soldier_id(i), 0);
		for (int i = 0; i < kMaxHeroNum; i++)
			player_.ImplementMission(EMissionTargetType::MTT_UseHero, formation.hero_id(i), 0);
	}

	pto_PLAYER_S2C_RES_SaveFormation pto_res;
	pto_res.set_res(result);
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_SaveFormation);
}

void CArmy::__TrainHero(const CMessage& msg)
{
	if (!player_.IsOpenGuide(14))
		return;

	enum TrainMode
	{
		TRAIN_NORMAL,		//普通
		TRAIN_REINFORCE,	//加强
		TRAIN_PLATINUM,		//白金
		TRAIN_EXTREME,		//至尊
		TRAIN_RECOVER		//恢复
	};

	pto_PLAYER_C2S_REQ_TrainHero pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	int result{ 0 };

	CHeroCard*	hero{ nullptr };

	auto itHero = hero_cards_.find(pto.hero_id());

	if (hero_cards_.cend() != itHero)
		hero = itHero->second;
	else
		return;

	int need_honor{ 0 };
	int need_gold{ 0 };
	int ratio{ 0 };
	int train_values[3]{0, 0, 0};

	const CRootData* root_data{ CRootData::GetRootData(player_.level()) };

	switch (pto.train_type())
	{
	case TRAIN_NORMAL:
	{
		if (!root_data)
			return;

		ratio = 0;
		need_honor = root_data->GetTrainSpendHonour();

		if (player_.honor() < need_honor)
			result = 2;
		else
			__TrainHero(hero, ratio, train_values[0], train_values[1], train_values[2]);
	}
		break;
	case TRAIN_REINFORCE:
	{
		ratio = 10;
		need_gold = CGoldSpend::GetGoldSpend(GST_TrainHero);

		if (player_.gold() < need_gold)
			result = 3;
		else
			__TrainHero(hero, ratio, train_values[0], train_values[1], train_values[2]);
	}
		break;
	case TRAIN_PLATINUM:
	{
		ratio = 150;
		need_gold = CGoldSpend::GetGoldSpend(GST_TrainPlatinum);

		if (!CVIPFun::GetVIPFun(player_.vip_level(), VFT_OpenTrainPlatinum))
			result = 4;
		else if (player_.gold() < need_gold)
			result = 3;
		else
			__TrainHero(hero, ratio, train_values[0], train_values[1], train_values[2]);
	}
		break;
	case TRAIN_EXTREME:
	{
		ratio = 450;
		need_gold = CGoldSpend::GetGoldSpend(GST_TrainExtreme);

		if (!CVIPFun::GetVIPFun(player_.vip_level(), VFT_OpenTrainExtreme))
			result = 4;
		else if (player_.gold() < need_gold)
			result = 3;
		else
			__TrainHero(hero, ratio, train_values[0], train_values[1], train_values[2]);
	}
		break;
	default:
		return;
	}

	if (0 == result)
	{
		player_.ChangeHonor(-need_honor);
		player_.ChangeGold(-need_gold, 39);

		player_.ImplementMission(EMissionTargetType::MTT_TrainHero, pto.hero_id(), 0);
		player_.ImplementRewardMission(RewardMissionType::RMT_Train, 0, 1);
	}

	pto_PLAYER_S2C_RES_TrainHero pto_res;
	pto_res.set_res(result);

	if (0 == result)
	{
		pto_res.set_hero_id(hero->hero_id());
		pto_res.set_add_str(train_values[0]);
		pto_res.set_add_cmd(train_values[1]);
		pto_res.set_add_int(train_values[2]);

		hero_train_temp_.fill(0);
		hero_train_temp_[0] = hero->hero_id();
		hero_train_temp_[1] = train_values[0];
		hero_train_temp_[2] = train_values[1];
		hero_train_temp_[3] = train_values[2];
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_TrainHero);
}

void CArmy::__SaveTrainHero(const CMessage& msg)
{
	pto_PLAYER_S2C_RES_SaveTrainHero pto;

	if (hero_train_temp_.empty())
		return;

	auto itHero = hero_cards_.find(hero_train_temp_[0]);

	CHeroCard*	hero{ nullptr };

	if (hero_cards_.cend() == itHero)
	{
		pto.set_res(1);
	}
	else
	{
		hero = itHero->second;

		if (!hero)
			return;

		int nOldRank = CArmy::CalculateHeroRank(hero);

		pto.set_res(0);

		hero->change_train_str(hero_train_temp_[1]);
		hero->change_train_cmd(hero_train_temp_[2]);
		hero->change_train_int(hero_train_temp_[3]);

		pto.set_hero_id(hero_train_temp_[0]);
		pto.set_add_str(hero_train_temp_[1]);
		pto.set_add_cmd(hero_train_temp_[2]);
		pto.set_add_int(hero_train_temp_[3]);

		hero_train_temp_.fill(0);
	}

	player_.SendToAgent(pto, PLAYER_S2C_RES_SaveTrainHero);
}

void CArmy::__RecruitHero(const CMessage& msg)
{
	if (!player_.IsOpenGuide(11))
		return;

	pto_PLAYER_C2S_REQ_RecruitHero pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	int hero_id{ pto.id() };

	const CRecruitLib* recruit_lib{ CRecruitLib::GetRecruitLib(pto.id()) };

	auto itIntimacy = heroes_intimacy_.find(hero_id);

	int result{ 0 };

	if (!recruit_lib || !CHeroTP::GetHeroTP(hero_id))
	{
		result = 1;
	}
	else if (player_.reputation() < recruit_lib->GetReputation())
	{
		result = 2;
	}
	else if (itIntimacy == heroes_intimacy_.cend() || itIntimacy->second < recruit_lib->GetNeedLevel())
	{
		result = 2;
	}
	else if (player_.silver() < recruit_lib->GetSilver())
	{
		result = 3;
	}
	else if (hero_cards_.find(hero_id) != hero_cards_.cend())
	{
		result = 4;
	}
	else if (player_.level() < recruit_lib->GetNeedLevel())
	{
		result = 5;
	}
	else
	{
		LOCK_BLOCK(lock_);
		AddHero(hero_id, false);
		player_.ChangeSilver(-recruit_lib->GetSilver());
		player_.ImplementMission(EMissionTargetType::MTT_RecruitHero, hero_id, 0);
	}

	pto_PLAYER_S2C_RES_RecruitHero pto_res;
	pto_res.set_id(hero_id);
	pto_res.set_res(result);
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_RecruitHero);
}

void CArmy::__UpgradeTechnology(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_UnitTechnologyLvUp pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	SoldierTechnology tech_type{ static_cast<SoldierTechnology>(pto.technology_type()) };

	if (tech_type < 0 || tech_type >= kSoldierTechCount)
		return;

	pto_PLAYER_S2C_RES_UnitTechnologyLvUp pto_res;
	pto_res.set_res(0);

	int			tech_level{ 0 };
	long long	need_exp{ 0 };

	tech_level = soldiers_technology_[tech_type];

	if (tech_level >= player_.level())
	{
		pto_res.set_res(2);
	}
	else
	{
		need_exp = CExpLibrary::GetSoldierExp(tech_level, tech_type);

		if (CVIPFun::GetVIPFun(player_.vip_level(), VFT_LowerTechnologyPrice))
			need_exp = __int64(need_exp * 0.9);

		if (player_.exp() < need_exp || need_exp <= 0)
			pto_res.set_res(1);
	}

	if (!pto_res.res())
	{
		soldiers_technology_[tech_type]++;

		player_.ChangeExp(-need_exp);

		player_.ImplementMission(MTT_TechnologyLv, pto.technology_type() + 1, soldiers_technology_[tech_type]);

		pto_res.set_res(0);
		pto_res.set_technology_type(pto.technology_type());
		pto_res.set_technology_lv(soldiers_technology_[tech_type]);

		player_.ImplementMission(EMissionTargetType::MTT_UnitTechnology, pto.technology_type(), 0);
		player_.ImplementMission(EMissionTargetType::MTT_AllTechnologyLevel, 0, 0);
		player_.ImplementRewardMission(RewardMissionType::RMT_UpgradeTechnology, 0, 1);
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_UnitTechnologyLvUp);
}

void CArmy::__BuyRunePage()
{
	pto_PLAYER_S2C_RES_BuyRunePage pto;

	int result{ 0 };

	int need_gold{ 0 };

	switch (rune_page_)
	{
	case 0:	need_gold = 100;		break;
	case 1:	need_gold = 200;		break;
	case 2:	need_gold = 300;		break;
	default:result = 2;				break;
	}

	if (player_.vip_level() < 3)
		result = 3;

	if (player_.gold() < need_gold)
		result = 1;

	if (0 == result)
	{
		player_.ChangeGold(-need_gold, 40);
		rune_page_++;
		pto.set_open_page(rune_page_);
	}

	pto.set_res(result);

	player_.SendToAgent(pto, PLAYER_S2C_RES_BuyRunePage);
}

void CArmy::__LockRune(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_LockRune pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_LockRune pto_res;
	pto_res.set_id(pto.id());
	pto_res.set_hero_id(pto.hero_id());
	pto_res.set_lock(pto.lock());

	int result{ 0 };

	if (0 > pto.id() || 16 < pto.id())
		result = 2;

	CHeroCard* hero{ FindHero(pto.hero_id()) };

	if (!hero)
		result = 1;

	if (0 == result)
		CRuneLibrary::SetLock(hero->runes_[pto.id()], pto.lock());

	pto_res.set_res(result);

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_LockRune);
}

void CArmy::__FullRuneEnergy()
{
	pto_PLAYER_S2C_RES_FullRuneEnergy pto;

	int result{ 0 };

	if (player_.gold() < 10)
		result = 2;
	if (rune_energy_ >= 50)
		result = 1;
	if (!CVIPFun::GetVIPFun(player_.vip_level(), VFT_FullRuneEngery))
		result = 3;

	if (0 == result)
	{
		rune_energy_ = 50;
		player_.ChangeGold(-10, 41);
	}

	pto.set_res(result);
	player_.SendToAgent(pto, PLAYER_S2C_RES_FullRuneEnergy);
}

void CArmy::__WashRune(const CMessage& msg)
{
	if (!player_.IsOpenGuide(27))
		return;

	pto_PLAYER_C2S_REQ_WashRune pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_WashRune pto_res;
	pto_res.set_hero_id(pto.hero_id());
	pto_res.set_page(pto.page());

	int result{ 0 };

	int nStartPos = 0;
	switch (pto.page())
	{
	case 0:	nStartPos = 0;	break;
	case 1:	nStartPos = 4;	break;
	case 2:	nStartPos = 8;	break;
	case 3:	nStartPos = 12;	break;
	}

	CHeroCard* hero{ FindHero(pto.hero_id()) };

	if (hero == nullptr)
		return;

	int nLockNum{ 0 };

	for (int i = nStartPos; i < nStartPos + 4; i++)
	{
		if (CRuneLibrary::HasLock(hero->runes_[i]))
			nLockNum++;
	}

	__int64 nSilver = CRootData::GetRootData(player_.level(), RDT_WashRuneHonour);
	switch (nLockNum)
	{
	case 1:
		nSilver = int(nSilver * 1.5f);
		break;
	case 2:
		nSilver = nSilver * 3;
		break;
	case 3:
		nSilver = nSilver * 6;
		break;
	}

	if (player_.silver() < nSilver)
	{
		result = 1;
	}
	else if (nLockNum == 4)
	{
		result = 7;
	}
	else if (pto.page() > rune_page_)
	{
		result = 5;
	}
	else if (1 == pto.type())
	{
		player_.ChangeSilver(-nSilver);
		rune_energy_++;

		player_.ImplementMission(MTT_WashRune, 0, 0);

		if (rune_energy_ > 50)
			rune_energy_ = 50;
		int nBrokeRune{ 0 };
		for (int i = nStartPos; i < nStartPos + 4; i++)
		{
			unsigned nRune = hero->runes_[i];
			if (!CRuneLibrary::HasLock(nRune))
			{
				nRune = CRuneLibrary::WashRune(WRT_Nomal, 1, nBrokeRune);
				while (nRune == hero->runes_[i])
					nRune = CRuneLibrary::WashRune(WRT_Nomal, 1, nBrokeRune);
				hero->runes_[i] = nRune;
				if (Broke_Rune_ID == CRuneLibrary::GetRuneID(nRune))
					nBrokeRune++;
				CRuneLibrary::SetLock(nRune, false);
			}

			pto_PLAYER_Struck_Player_Rune* pRune = pto_res.add_runes();
			pRune->set_rune_id(CRuneLibrary::GetRuneID(nRune));
			if (pRune->rune_id() != Broke_Rune_ID)
				pRune->set_colour(CRuneLibrary::GetRuneColor(nRune));
			else
				pRune->set_colour(0);

			pRune->set_lock(CRuneLibrary::HasLock(nRune));

		}
	}
	else if (2 == pto.type())
	{
		if (rune_energy_ < 50)
		{
			result = 2;
		}
		else
		{
			player_.ChangeSilver(-nSilver);
			rune_energy_ = 0;
			player_.ImplementMission(MTT_WashRune, 0, 0);
			for (int i = nStartPos; i < nStartPos + 4; i++)
			{
				unsigned nRune = hero->runes_[i];
				if (!CRuneLibrary::HasLock(nRune))
				{
					nRune = CRuneLibrary::WashRune(WRT_Strengthen, CRuneLibrary::GetRuneCellColour(i), 0);
					while (nRune == hero->runes_[i])
						nRune = CRuneLibrary::WashRune(WRT_Strengthen, CRuneLibrary::GetRuneCellColour(i), 0);
					hero->runes_[i] = nRune;
					CRuneLibrary::SetLock(nRune, false);
				}

				pto_PLAYER_Struck_Player_Rune* pRune = pto_res.add_runes();
				pRune->set_rune_id(CRuneLibrary::GetRuneID(nRune));
				if (pRune->rune_id() != Broke_Rune_ID)
					pRune->set_colour(CRuneLibrary::GetRuneColor(nRune));
				else
					pRune->set_colour(0);
				pRune->set_lock(CRuneLibrary::HasLock(nRune));
			}
		}
	}

	else if (3 == pto.type())
	{
		if (rune_energy_ < 50)
			result = 2;
		else if (!player_.knapsack()->FindItem(91004, ItemLocation::IL_KNAPSACK))
			result = 2;
		else
		{
			player_.ChangeSilver(-nSilver);
			player_.knapsack()->MoveItem(false, 91004, 1, ItemLocation::IL_KNAPSACK, ItemLocation::IL_NULL);
			player_.knapsack()->SendUseItem(91004, -1);
			rune_energy_ = 0;
			player_.ImplementMission(MTT_WashRune, 0, 0);
			for (int i = nStartPos; i < nStartPos + 4; i++)
			{
				unsigned nRune = hero->runes_[i];
				if (!CRuneLibrary::HasLock(nRune))
				{
					nRune = CRuneLibrary::WashRune(WRT_Item, CRuneLibrary::GetRuneCellColour(i), 0);
					while (nRune == hero->runes_[i])
						nRune = CRuneLibrary::WashRune(WRT_Item, CRuneLibrary::GetRuneCellColour(i), 0);
					hero->runes_[i] = nRune;
					CRuneLibrary::SetLock(nRune, false);
				}

				pto_PLAYER_Struck_Player_Rune* pRune = pto_res.add_runes();
				pRune->set_rune_id(CRuneLibrary::GetRuneID(nRune));
				if (pRune->rune_id() != Broke_Rune_ID)
					pRune->set_colour(CRuneLibrary::GetRuneColor(nRune));
				else
					pRune->set_colour(0);
				pRune->set_lock(CRuneLibrary::HasLock(nRune));
			}
		}
	}

	pto_res.set_rune_engery(rune_energy_);
	pto_res.set_res(result);
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_WashRune);
}

void CArmy::__IntimacyGame(const CMessage& msg)
{
	if (!player_.IsOpenGuide(11))
		return;

	pto_PLAYER_C2S_REQ_IntimacyGame pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_IntimacyGame pto_res;
	pto_res.set_res(0);
	pto_res.set_hero_id(pto.hero_id());

	const CRecruitLib* pRecruit = CRecruitLib::GetRecruitLib(pto.hero_id());
	auto itIntimacy = heroes_intimacy_.find(pto.hero_id());

	if (FindHero(pto.hero_id()))
		pto_res.set_res(1);
	else if (!pRecruit)
		pto_res.set_res(1);
	else if (pRecruit->GetNeedLevel() > player_.level())
		pto_res.set_res(3);
	else if (itIntimacy == heroes_intimacy_.cend())
		heroes_intimacy_.insert(std::make_pair(pto.hero_id(), 0));

	int nReputation{ 0 };

	for (int i = 0; i < pto.bet_size(); i++)
		nReputation += pto.bet(i).reputation();

	if (nReputation > player_.reputation())
		pto_res.set_res(2);

	if (!pto_res.res())
	{
		auto it = heroes_intimacy_.find(pto.hero_id());
		int nCellID = CRecruitLib::ProduceCellID();
		int nGiftID = CRecruitLib::GetGiftID(nCellID);
		pto_res.set_cell_id(nCellID);
		int nIntimacy{ 0 };
		for (int i = 0; i < pto.bet_size(); i++)
		{
			if (nGiftID == pto.bet(i).gift_id())
				nIntimacy += pto.bet(i).reputation() * CRecruitLib::GetGiftRatio(nGiftID);
			else
				nIntimacy += pto.bet(i).reputation();
		}

		if (it->second + nIntimacy >= pRecruit->GetReputation())
		{
			nIntimacy = pRecruit->GetReputation() - it->second;

			AddHero(it->first, true);
			player_.ImplementMission(EMissionTargetType::MTT_RecruitHero, pto.hero_id(), 0);
		}

		it->second += nIntimacy;
		pto_res.set_intimacy(nIntimacy);
		player_.ChangeReputation(-nReputation);
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_IntimacyGame);
}

void CArmy::__ExchangeHeroExp(const CMessage& msg)
{
	if (!player_.IsOpenGuide(37))
		return;

	pto_PLAYER_C2S_REQ_ExchangeHeroExp pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_ExchangeHeroExp pto_res;
	pto_res.set_res(0);
	pto_res.set_hero_id(pto.hero_id());

	if (practice_seconds_ <= time(0) - last_practice_time_)
	{
		practice_seconds_ = 0;
		is_in_practice_cd_ = false;
	}
	else if (!CVIPFun::GetVIPFun(player_.vip_level(), VFT_RelievedHeroInteriorCD))
	{
		practice_seconds_ -= static_cast<int>(time(0) - last_practice_time_);
		last_practice_time_ = time(0);
		if (is_in_practice_cd_)
			pto_res.set_res(4);
	}

	int nHonor = 0;

	CHeroCard* hero{ FindHero(pto.hero_id()) };

	if (!hero)
	{
		pto_res.set_res(2);
	}
	else if (hero->level_ >= player_.level())
	{
		pto_res.set_res(3);
	}
	else
	{
		nHonor = (int)CRootData::GetRootData(hero->level_, RDT_HeroLevelHonour);
		if (player_.honor() < nHonor)
			pto_res.set_res(1);
	}

	if (!pto_res.res())
	{
		if (!CVIPFun::GetVIPFun(player_.vip_level(), VFT_RelievedHeroInteriorCD))
		{
			practice_seconds_ += 60;
			if (practice_seconds_ >= 3600)
				is_in_practice_cd_ = true;
			last_practice_time_ = time(0);
		}
		else
			last_practice_time_ = 0;

		player_.ChangeHonor(-nHonor);
		player_.ImplementMission(MTT_HeroLevelUp, 0, 0);

		int get_exp = __RandomExp();

		int max_exp = hero->level_ - hero->exp_;

		if (get_exp >= max_exp)
		{
			hero->level_++;
			hero->exp_ = 0;
			pto_res.set_level_up(true);
			pto_res.set_exp(max_exp);

			player_.ImplementMission(MTT_HeroLevel, hero->hero_id(), hero->level_);
		}
		else
		{
			hero->exp_ += get_exp;
			pto_res.set_level_up(false);
			pto_res.set_exp(get_exp);
		}

		pto_res.set_exchange_cd(practice_seconds_);
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_ExchangeHeroExp);
}

void CArmy::__ClearExchangeCD()
{
	pto_PLAYER_S2C_RES_ClearExchangeCD pto;
	pto.set_res(0);

	int nRemainTime{ 0 };
	int nGold{ 0 };

	if (practice_seconds_ <= time(0) - last_practice_time_)
		pto.set_res(1);
	else
		nRemainTime = static_cast<int>(practice_seconds_ - (time(0) - last_practice_time_));

	if (!(nRemainTime % 60))
		nGold = nRemainTime / 60;
	else
		nGold = (nRemainTime - (nRemainTime % 60)) / 60 + 1;

	if (nGold > player_.gold())
		pto.set_res(2);

	if (!pto.res())
	{
		practice_seconds_ = 0;
		is_in_practice_cd_ = false;
		player_.ChangeGold(-nGold, 42);
	}

	player_.SendToAgent(pto, PLAYER_S2C_RES_ClearExchangeCD);
}

int	CArmy::__GetTrainLevel(CHeroCard* hero, TrainValueType type)
{
	int max_num = ((hero->level_ - 30) * 2) + 20;

	int train_num;

	switch (type)
	{
	case TrainValueType::kStr:
		train_num = hero->train_str();
		break;
	case TrainValueType::kCmd:
		train_num = hero->train_cmd();
		break;
	case TrainValueType::kInt:
		train_num = hero->train_int();
		break;
	}

	if (train_num < 10)
		return 6;
	else if (max_num <= train_num)
		return 0;
	else if (5 >= max_num - train_num)
		return 1;
	else if (15 >= max_num - train_num)
		return 2;
	else if (25 >= max_num - train_num)
		return 3;
	else if (40 >= max_num - train_num)
		return 4;
	else
		return 5;
}

void CArmy::__GetSuitAddtion(CHeroCard* hero_card, SuitAddition* suit_addition)
{
	std::map<int, int> suits;
	std::vector<int> suit_id;
	for (auto it_hero_equip : player_.knapsack()->GetHeroEquip(hero_card->hero_id()))
	{
		auto it_suit = suits.find(it_hero_equip->suit_id());
		if (suits.cend() == it_suit)
		{
			suits.insert(std::make_pair(it_hero_equip->suit_id(), 1));
			suit_id.push_back(it_hero_equip->suit_id());
		}
		else
			it_suit->second++;
	}

	int level = (player_.level() / 20 + 1);
	for (size_t i = 0; i < suit_id.size(); i++)
	{
		const CSuitTP* suit_tp = CSuitTP::GetSuitTP(suit_id.at(i));

		if (suit_tp)
		{
			auto it_suit = suits.find(suit_id.at(i));
			if (it_suit != suits.cend())
			{
				if (it_suit->second >= 2)
				{
					float attribute = suit_tp->stage(2) * level + suit_tp->attribute(2);
					switch (suit_tp->attribute_type(2))
					{
					case kSuitAtk:			suit_addition->atk_ += (int)attribute;			break;
					case kSuitDef:			suit_addition->def_ += (int)attribute;			break;
					case kSuitHP:			suit_addition->hp_ += (int)attribute;			break;
					case kSuitMoveSpeed:	suit_addition->move_speed_ += (int)attribute;	break;
					case kSuitCritOdds:		suit_addition->crit_odds_ += (int)attribute;		break;
					case kSuitPreventCrit:	suit_addition->prevent_crit_ += (int)attribute;	break;
					case kSuitStr:			suit_addition->str_ += (int)attribute;			break;
					case kSuitCmd:			suit_addition->cmd_ += (int)attribute;			break;
					case kSuitInt:			suit_addition->int_ += (int)attribute;			break;
					case kSuitAtkSpeed:		suit_addition->atk_speed_ += (int)attribute;		break;
					case kSuitMaxTime:		suit_addition->max_time_ += (int)attribute;		break;
					}
				}

				if (it_suit->second >= 4)
				{
					float attribute = suit_tp->stage(4) * level + suit_tp->attribute(4);
					switch (suit_tp->attribute_type(4))
					{
					case kSuitAtk:			suit_addition->atk_ += (int)attribute;			break;
					case kSuitDef:			suit_addition->def_ += (int)attribute;			break;
					case kSuitHP:			suit_addition->hp_ += (int)attribute;			break;
					case kSuitMoveSpeed:	suit_addition->move_speed_ += (int)attribute;	break;
					case kSuitCritOdds:		suit_addition->crit_odds_ += (int)attribute;	break;
					case kSuitPreventCrit:	suit_addition->prevent_crit_ += (int)attribute;	break;
					case kSuitStr:			suit_addition->str_ += (int)attribute;			break;
					case kSuitCmd:			suit_addition->cmd_ += (int)attribute;			break;
					case kSuitInt:			suit_addition->int_ += (int)attribute;			break;
					case kSuitAtkSpeed:		suit_addition->atk_speed_ += (int)attribute;	break;
					case kSuitMaxTime:		suit_addition->max_time_ += (int)attribute;		break;
					}
				}

				if (it_suit->second >= 6)
					suit_addition->skill_id = it_suit->first;
			}
		}
	}
}

void CArmy::__UpgradeSoldierTrainLevel(const CMessage& msg)
{
	if (!player_.IsOpenGuide(19))
		return;

	pto_PLAYER_C2S_REQ_UpgradeSoldierTrainLevel pto_c2s;
	pto_c2s.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_UpgradeSoldierTrainLevel pto_s2c;
	pto_s2c.set_res(0);
	pto_s2c.set_soldier_id(pto_c2s.soldier_id());

	auto it = soldiers_.find(pto_c2s.soldier_id());
	if (it == soldiers_.cend())
		pto_s2c.set_res(3);
	else
	{
		__int64 silver = CRootData::GetTrainSoldierSilver(it->second->train_level_ + 31);
		if (it->second->train_level_ >= (player_.level() - 30))
		{
			pto_s2c.set_res(2);
		}
		else if (silver > player_.silver())
		{
			pto_s2c.set_res(1);
		}
		else
		{
			player_.ChangeSilver(-silver);
			it->second->train_level_++;

			player_.ImplementMission(EMissionTargetType::MTT_UseBarracksCell, 0, 0);
		}
	}

	player_.SendToAgent(pto_s2c, PLAYER_S2C_RES_UpgradeSoldierTrainLevel);
}

void CArmy::__CultivateHero(const CMessage& msg)
{
	if (!player_.IsOpenGuide(41))
		return;

	pto_PLAYER_C2S_REQ_CultivateHero pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_CultivateHero pto_res;
	pto_res.set_res(0);
	pto_res.set_hero_id(pto_req.hero_id());

	auto itHero = hero_cards_.find(pto_req.hero_id());
	int need_item{ 0 };
	if (itHero != hero_cards_.cend())
	{
		if (nullptr == CHeroQuality::GetHeroQuality(itHero->second->quality_ + 1))
			pto_res.set_res(2);
		else
		{
			switch (itHero->second->quality_)
			{
			case 1:
				need_item = 1;
				break;
			case 2:
				need_item = 3;
				break;
			case 3:
				need_item = 7;
				break;
			case 4:
				need_item = 15;
				break;
			case 5:
				need_item = 30;
				break;
			case 6:
				need_item = 50;
				break;
			default:
				pto_res.set_res(2);
			}
		}
	}
	else
		pto_res.set_res(3);
	CItem* item = player_.knapsack()->FindItem(91002, ItemLocation::IL_KNAPSACK);
	if (nullptr == item)
		pto_res.set_res(2);
	else if (item->quantity() < need_item)
		pto_res.set_res(2);

	//if (pto_req.safety()
	//	&& nullptr == player_.knapsack()->FindItem(91003, ItemLocation::IL_KNAPSACK))
	//	pto_res.set_res(2);

	if (!pto_res.res())
	{
		player_.knapsack()->MoveItem(false, 91002, need_item, ItemLocation::IL_KNAPSACK, ItemLocation::IL_NULL);
		player_.knapsack()->SendUseItem(91002, -need_item);

		//if (pto_req.safety())
		//{
		//	player_.knapsack()->MoveItem(false, 91003, 1, ItemLocation::IL_KNAPSACK, ItemLocation::IL_NULL);
		//	player_.knapsack()->SendUseItem(91003, -1);
		//}

		//bool result{ false };
		//if (itHero->second->safety_times_ < CHeroQuality::GetHeroQuality(itHero->second->quality_ + 1)->min_safety())
		//	result = false;
		//else
		//	result = CHeroQuality::QualityUp(itHero->second->quality_, itHero->second->safety_times_ * 0.0005f);

		//if (pto_req.safety())
		//	itHero->second->safety_times_++;
		//if (result)
		{
			itHero->second->quality_ += 1;

			if (itHero->second->quality_ == 5)
				GAME_WORLD()->Annunciate(player_.name(), itHero->second->hero_id(), 0, AnnunciateType::TrainHeroOrange);
			else if (itHero->second->quality_ == 6)
				GAME_WORLD()->Annunciate(player_.name(), itHero->second->hero_id(), 0, AnnunciateType::TrainHeroRed);
			else if (itHero->second->quality_ == 7)
				GAME_WORLD()->Annunciate(player_.name(), itHero->second->hero_id(), 0, AnnunciateType::TrainHeroGold);
		}
		//else if (!pto_req.safety())
		//	itHero->second->quality_ = 1;

		player_.ImplementMission(EMissionTargetType::MTT_TrainHeroRank, itHero->second->hero_id(), itHero->second->quality_);


		pto_res.set_succeed(1);
		pto_res.set_quality(itHero->second->quality_);
		pto_res.set_safety_times(itHero->second->safety_times_);
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_CultivateHero);
}

void CArmy::__LoadFromDatabase()
{
	std::ostringstream sql;
	sql << "select * from tb_player_army where pid = " << player_.pid();
	ResultSetPtr result_army{ MYSQL_QUERY(sql.str()) };

	if (result_army && result_army->next())
	{
		soldiers_technology_[kSoldierTechAtk] = result_army->getInt("tech_atk");
		soldiers_technology_[kSoldierTechMatk] = result_army->getInt("tech_matk");
		soldiers_technology_[kSoldierTechDef] = result_army->getInt("tech_def");
		soldiers_technology_[kSoldierTechMdef] = result_army->getInt("tech_mdef");
		soldiers_technology_[kSoldierTechHP] = result_army->getInt("tech_hp");

		auto it_intimacy = CSerializer<int>::ParsePairFromString(result_army->getString("hero_intimacy").c_str());
		for (auto &it : it_intimacy)
			heroes_intimacy_[it.first] = it.second;

		is_in_practice_cd_ = result_army->getBoolean("is_in_practice_cd");
		practice_seconds_ = result_army->getInt("parctice_seconds");
		last_practice_time_ = result_army->getInt64("last_parctice_time");

		rune_page_ = result_army->getInt("rune_page");
		rune_energy_ = result_army->getInt("rune_energy");

		formation_heroes[0] = result_army->getInt("hero1");
		formation_heroes[1] = result_army->getInt("hero2");
		formation_heroes[2] = result_army->getInt("hero3");
		formation_soldiers[0] = result_army->getInt("soldier1");
		formation_soldiers[1] = result_army->getInt("soldier2");
		formation_soldiers[2] = result_army->getInt("soldier3");
		formation_soldiers[3] = result_army->getInt("soldier4");
		formation_soldiers[4] = result_army->getInt("soldier5");
		formation_soldiers[5] = result_army->getInt("soldier6");
	}
	else
	{
		throw 6;
	}

	sql.str("");
	sql << "select * from tb_player_soldier where pid = " << player_.pid();
	ResultSetPtr result_soldiers{ MYSQL_QUERY(sql.str()) };
	if (result_soldiers)
	{
		while (result_soldiers->next())
		{
			SoldierData* soldier_data = new SoldierData;
			soldier_data->soul_level_ = result_soldiers->getInt("soul_lv");
			soldier_data->soul_exp_ = result_soldiers->getInt("soul_exp");
			soldier_data->train_level_ = result_soldiers->getInt("train");

			if (false == soldiers_.insert(std::make_pair(result_soldiers->getInt("sodlier_id"), soldier_data)).second)
				delete soldier_data;
		}
	}

	sql.str("");
	sql << "select * from tb_player_hero where pid = " << player_.pid();
	ResultSetPtr result_heroes{ MYSQL_QUERY(sql.str()) };
	if (result_heroes)
	{
		while (result_heroes->next())
		{
			CHeroCard* cards{ new CHeroCard(result_heroes->getInt("hid")) };
			if (cards->hero_id())
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
				hero_cards_[cards->hero_id()] = cards;
			}
			else
			{
				delete cards;
			}
		}
	}


}


