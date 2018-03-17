#include "stdafx.h"
#include "Mission.h"
#include "Player.h"
#include "Army.h"
#include "HeroCard.h"
#include "Knapsack.h"
#include "Equip.h"
#include "Town.h"

CMission::CMission(CPlayer& player) :
player_{ player }
{
	__LoadFromDatabase();
}

CMission::~CMission()
{

}

void CMission::ProcessMessage(const CMessage& msg)
{
	switch (msg.GetProtocolID())
	{
	case MISSION_C2S_REQ_AcceptMission:			__AcceptMission(msg);						break;
	case MISSION_C2S_REQ_GiveBackMission:		__CommitMission(msg);						break;
	default:RECORD_WARNING(FormatString("未知的Mission协议号:", msg.GetProtocolID()));		break;
	}
}

void CMission::SendInitialMessage()
{
	if (player_.guild_id())
		ImplementMission(EMissionTargetType::MTT_CreateOrJoinGuild, 0, 0);
	for (auto it : accept_mission_)
	{
		const CMissionTP* mission = CMissionTP::GetMission(it.first);
		if (mission &&mission->GetType() == 1 && mission->GetTargetType() == EMissionTargetType::MTT_ExchangeFashion)
			it.second = mission->GetNeedTargetNum();
	}

	__UpdateMissionToClient();
}

void CMission::SaveToDatabase()
{
	CSerializer<int> accet_ser;
	for (auto &it : accept_mission_)
		accet_ser.AddPair(it.first, it.second);

	CSerializer<int> complete_ser;
	for (auto &it : complete_branch_)
		complete_ser.AddValue(it);

	std::ostringstream sql;
	sql << "update tb_player_mission set main_progress = " << main_progress_ <<
		", accept_mission = '" << accet_ser.SerializerPairToString().c_str() <<
		"',complete_branch = '" << complete_ser.SerializerToString().c_str() <<
		"',stage_normal = " << stage_normal_progress_ <<
		",stage_hard = " << stage_hard_progress_ <<
		",stage_nightmare = " << stage_nightmare_progress_ <<
		",stage_elite = " << stage_elite_progress_ <<
		",stage_multi = " << stage_multi_progress_ <<
		" where pid = " << player_.pid();

	MYSQL_UPDATE(sql.str());
}

void CMission::ImplementMission(EMissionTargetType target_type, int target_id, int target_id_ex)
{
	for (auto &it : accept_mission_)
	{
		const CMissionTP* mission_templete = CMissionTP::GetMission(it.first);
		if (nullptr == mission_templete)
			continue;

		switch (target_type)
		{
		case MTT_TalkToNpc:				//1.与NPC对话
			break;
		case MTT_CompleteMap:			//2.完成关卡
			if (target_id == mission_templete->GetTargetID()
				&& target_id_ex == mission_templete->GetTargetIDEx()
				&& target_type == mission_templete->GetTargetType()
				&& it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_EquipLvUpTimes:		//3.强化装备次数
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_UseHero:				//4.武将出阵
			if (target_id == mission_templete->GetTargetID() && target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_UseEquip:				 //5.穿戴装备
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_EquipLvUpLv:			//6.强化装备等级
			if (target_id == mission_templete->GetTargetID() && target_id_ex >= mission_templete->GetTargetIDEx() && target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_UseSoldier:			  //7.士兵出阵
			if (target_id == mission_templete->GetTargetID() && target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_UnitTechnology:			//8.强化士兵
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_TrainHero:					 //9.武将培养
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_Enchanting:				 //10.装备重铸
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_RewardMission:				//11.完成悬赏
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
			break;
		case MTT_UseProduceCell:			//12.完成耕作
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_UseBarracksCell:			 //13.完成训练
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_UseSearchCell:				//14.完成探访
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_CreateOrJoinGuild:			//15.创建或加入公会
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_GuildContribute:			//16.募捐银币
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_RecruitHero:				//17.招募武将
			if (target_id == mission_templete->GetTargetID() && target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_DIYLvUp:
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;

		case MTT_TechnologyLv:
			if (target_id == mission_templete->GetTargetID() && target_id_ex >= mission_templete->GetTargetIDEx() && target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_PetLevel:
			break;
		case MTT_OfflineBattle:
			if (target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_EliteStage:
			if (target_id == mission_templete->GetTargetID() &&
				target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_MultiStage:
			if (target_id == mission_templete->GetTargetID() &&
				target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_SpeedStage:
			if (target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_FairArena:
			if (target_id == mission_templete->GetTargetID() &&
				target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_Escort:
			if (target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_RobEscort:
			if (target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_AllTechnologyLevel:
			if (target_type == mission_templete->GetTargetType())
			{
				for (size_t i = kSoldierTechAtk; i < kSoldierTechCount; i++)
				{
					if (player_.army()->soldier_technology(static_cast<SoldierTechnology>(i)) < mission_templete->GetTargetIDEx())
						return;
				}

				if (it.second < mission_templete->GetNeedTargetNum())
					it.second++;

				__SendCountUpdate();
			}
			break;
		case MTT_GetTownBox:
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_TrainHeroRank:
			if (target_id == mission_templete->GetTargetID() &&
				target_id_ex >= mission_templete->GetTargetIDEx() &&
				target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_HeroLevel:
			if (target_id == mission_templete->GetTargetID() &&
				target_id_ex >= mission_templete->GetTargetIDEx() &&
				target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_WashRune:
			if (target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_HeroLevelUp:
			if (target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_UseLevy:
			if (target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_ExchangeFashion:
			if (target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_Shopping:
			if (target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
			break;
		case MTT_Trade:
			if (target_type == mission_templete->GetTargetType() &&
				it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_Security:
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_War:
			if (target_type == mission_templete->GetTargetType() && it.second < mission_templete->GetNeedTargetNum())
			{
				it.second++;
				__SendCountUpdate();
			}
			break;
		case MTT_AC:
			it.second = mission_templete->GetNeedTargetNum();
			__SendCountUpdate();
			break;
		default:
			break;
		}
	}
}

void CMission::SetMission(int progress)
{
	main_progress_ = progress;

	accept_mission_.clear();

	__UpdateMissionToClient();
}

bool CMission::IsAcceptMission(int mission_id)
{
	for (auto it : accept_mission_)
	{
		if (it.first == mission_id)
			return true;
	}
	return false;
}

bool CMission::IsCompleteMission(int mission_id)
{
	for (auto it : complete_branch_)
	{
		if (it == mission_id)
			return true;
	}
	return false;
}

void CMission::SetStageProcess(int level, StageType type)
{
	switch (type)
	{
	case StageType::NORMAL:
		if (level > stage_normal_progress_)
		{
			const CTownBox* pTownBox{ CTownBox::GetTownBoxByStageLevel(level) };
			if (pTownBox)
			{
				CTown* town{ GAME_WORLD()->FindTown(pTownBox->GetTownID()) };
				if (town)
					town->AddTop10Player(player_.pid());
			}
			stage_normal_progress_ = level;
		}
		break;
	case StageType::HARD:
		if (level > stage_hard_progress_)
			stage_hard_progress_ = level;
		break;
	case StageType::NIGHTMARE:
		if (level > stage_nightmare_progress_)
			stage_nightmare_progress_ = level;
		break;
	case StageType::ELITE:
		if (level > stage_elite_progress_)
			stage_elite_progress_ = level;
		break;
	case StageType::MULTI:
		if (level > stage_multi_progress_)
			stage_multi_progress_ = level;
		break;
	}

	if (type == StageType::ELITE)
	{
		pto_STAGE_S2C_NTF_UpdateEliteStage pto;
		pto.set_progress(stage_elite_progress_);
		pto.set_buy_times(0);
		player_.SendToAgent(pto, STAGE_S2C_NTF_UpdateEliteStage);
	}
	else
	{
		pto_STAGE_S2C_NTF_PLAYER_STAGE_PROGRESS pto;
		pto.set_normal_progress(stage_normal_progress_);
		pto.set_hard_progress(stage_hard_progress_);
		pto.set_nightmare_progress(stage_nightmare_progress_);
		player_.SendToAgent(pto, STAGE_S2C_NTF_StageProgress);
	}
}

void CMission::__AcceptMission(const CMessage &msg)
{
	pto_MISSION_C2S_REQ_ACCEPT pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_MISSION_S2C_ACCEPT_RES_TYPE ptoRes{ AcceptMission };

	int nMissionID{ pto.mission_id() };

	const CMissionTP* mission_templete = CMissionTP::GetMission(nMissionID);

	if (!mission_templete)
	{
		ptoRes = AcceptMissionIDDoesNotExist;
	}
	else if (IsAcceptMission(nMissionID))
	{
		ptoRes = HasTakenMission;
	}
	else if (main_progress_ < mission_templete->GetNeedMainProgress())
	{
		ptoRes = PlayerMainLineProgressIsTooLow;
	}
	else if (mission_templete->GetType() == 1 &&
		main_progress_ != mission_templete->GetNeedMainProgress())
	{
		ptoRes = PlayerMainLineProgressIsTooLow;
	}
	else if (IsCompleteMission(nMissionID))
	{
		ptoRes = HasTakenMission;
	}
	else if (player_.level() < mission_templete->GetNeedLevel())
	{
		ptoRes = PlayerLvIsTooLow;
	}
	else
	{
		accept_mission_.insert(std::make_pair(nMissionID, 0));
		if (mission_templete->GetType() == 1)
			main_progress_ = mission_templete->GetNeedMainProgress();
	}

	pto_MISSION_S2C_RES_ACCEPT ptoAccept;
	ptoAccept.set_mission_id(nMissionID);
	ptoAccept.set_nres(ptoRes);
	player_.SendToAgent(ptoAccept, MISSION_S2C_RES_AcceptMission);

	if (0 == ptoRes)
	{
		__GetOffer(mission_templete);
		__UpdateMissionToClient();

		if (mission_templete->m_nTargetType == MTT_EquipLvUpLv || mission_templete->GetTargetType() == MTT_TechnologyLv || mission_templete->GetTargetType() == MTT_GetTownBox)
			__JudgeMission();
		else if (mission_templete->m_nTargetType == MTT_DIYLvUp && player_.level() >= player_.max_level())
			CMission::ImplementMission(MTT_DIYLvUp, 0, 0);
		else if (mission_templete->m_nTargetType == MTT_EquipLvUpTimes)
			__JudgeEquipLvUpTimes(mission_templete->GetNeedTargetNum());
		else if (mission_templete->GetTargetType() == MTT_AllEquipLevel || mission_templete->GetTargetType() == MTT_AllTechnologyLevel)
			CMission::ImplementMission((EMissionTargetType)mission_templete->GetTargetType(), 0, 0);

		else if (mission_templete->GetTargetType() == MTT_RecruitHero && player_.army()->FindHero(mission_templete->GetTargetID()))
			CMission::ImplementMission(MTT_RecruitHero, mission_templete->GetTargetID(), 0);
		else if (mission_templete->GetTargetType() == MTT_HeroLevel)
		{
			CHeroCard* hero_card = player_.army()->FindHero(mission_templete->GetTargetID());
			if (hero_card)
				CMission::ImplementMission(MTT_HeroLevel, hero_card->hero_id(), hero_card->hero_level());
		}
		else if (mission_templete->GetTargetType() == MTT_TrainHeroRank)
		{
			CHeroCard* hero_card = player_.army()->FindHero(mission_templete->GetTargetID());
			if (hero_card)
				CMission::ImplementMission(MTT_TrainHeroRank, hero_card->hero_id(), hero_card->quality());
		}

		if (mission_templete->GetMissionID() == 21 && player_.level() > 19)
		{
			auto it = accept_mission_.find(pto.mission_id());
			if (it != accept_mission_.cend())
			{
				it->second = mission_templete->GetNeedTargetNum();
				__SendCountUpdate();
			}
		}
	}
}

void CMission::__CommitMission(const CMessage &msg)
{
	pto_MISSION_C2S_REQ_GIVEBACK pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_MISSION_S2C_GIVEBACK_RES_TYPE ptoRes{ GiveBackMission };

	int nMissionID = pto.mission_id();
	const CMissionTP* mission_templete = CMissionTP::GetMission(nMissionID);

	auto it = accept_mission_.find(pto.mission_id());
	if (it == accept_mission_.cend())
		ptoRes = HaveNotAcceptMission;

	else
	{
		if (!mission_templete)
			ptoRes = GiveBackMissionIDDoesNotExist;
		else if (it->second < mission_templete->GetNeedTargetNum())
			ptoRes = HaveNotFinishObject;
	}

	pto_MISSION_S2C_RES_GIVEBACK pto_res;
	pto_res.set_mission_id(nMissionID);
	pto_res.set_nres(ptoRes);
	player_.SendToAgent(pto_res, MISSION_S2C_RES_GiveBackMission);

	if (0 == ptoRes)
	{
		accept_mission_.erase(pto.mission_id());
		if (mission_templete->GetType() == 1)
			main_progress_++;
		else
			complete_branch_.push_back(pto.mission_id());
		__UpdateMissionToClient();
		__GetReward(mission_templete);
	}
}

void CMission::__GetReward(const CMissionTP* mission_templete)
{
	if (!mission_templete)
		return;

	player_.ChangeExp(mission_templete->m_nRewardExp);
	player_.ChangeSilver(mission_templete->m_nRewardSilver);
	player_.ChangeGold(mission_templete->m_nRewardGold,31);
	player_.ChangeHonor(mission_templete->m_nRewardHonor);
	player_.ChangeStamina(mission_templete->m_nRewardStamina);
	player_.ChangeReputation(mission_templete->m_nRewardReputation);


	player_.army()->AddHero(mission_templete->m_nRewardHeroID, true);
	player_.army()->AddSoldier(mission_templete->m_nRewardSoldier);

	player_.knapsack()->GiveNewItem(mission_templete->m_nRewardItem1, mission_templete->m_nRewardItem1Num);
	player_.knapsack()->GiveNewItem(mission_templete->m_nRewardItem2, mission_templete->m_nRewardItem2Num);
	player_.knapsack()->GiveNewItem(mission_templete->m_nRewardItem3, mission_templete->m_nRewardItem3Num);
	player_.knapsack()->GiveNewItem(mission_templete->m_nRewardItem4, mission_templete->m_nRewardItem4Num);
	player_.knapsack()->GiveNewItem(mission_templete->m_nRewardItem5, mission_templete->m_nRewardItem5Num);
	player_.knapsack()->GiveNewItem(mission_templete->m_nRewardItem6, mission_templete->m_nRewardItem6Num);

	player_.knapsack()->GiveNewEquip(mission_templete->m_nRewardEquip1, 0);
	player_.knapsack()->GiveNewEquip(mission_templete->m_nRewardEquip2, 0);
	player_.knapsack()->GiveNewEquip(mission_templete->m_nRewardEquip3, 0);
	player_.knapsack()->GiveNewEquip(mission_templete->m_nRewardEquip4, 0);
}

void CMission::__GetOffer(const CMissionTP* mission_templete)
{
	if (!mission_templete)
		return;

	player_.ChangeSilver(mission_templete->m_nOfferSilver);
	player_.ChangeHonor(mission_templete->m_nOfferHonor);
	player_.ChangeStamina(mission_templete->m_nOfferStamina);
	player_.ChangeReputation(mission_templete->m_nOfferReputation);
	player_.ChangeExp(mission_templete->m_nOfferEx);

	player_.knapsack()->GiveNewEquip(mission_templete->m_nOfferEquip1, 0);
	player_.knapsack()->GiveNewEquip(mission_templete->m_nOfferEquip2, 0);

	player_.army()->AddSoldier(mission_templete->m_nOfferSoldier);
	player_.army()->AddHero(mission_templete->m_nOfferHero, true);
}

void CMission::__SendCountUpdate()
{
	pto_MISSION_S2C_NTF_CountUpdate pto;

	for (auto it : accept_mission_)
	{
		pto_TakenMissionInfo* info = pto.add_update_missions();
		info->set_mission_id(it.first);
		info->set_mission_object_counter(it.second);
	}

	player_.SendToAgent(pto, MISSION_S2C_NTF_CountUpdate);
}

void CMission::__JudgeMission()
{
	if (player_.GetBoxState(1, StageType::NORMAL))
		ImplementMission(MTT_GetTownBox, 0, 0);

	for (size_t i = kSoldierTechAtk; i < kSoldierTechCount; i++)
		ImplementMission(MTT_TechnologyLv, i + 1, player_.army()->soldier_technology(static_cast<SoldierTechnology>(i)));
}

void CMission::__JudgeEquipLvUpTimes(int target_num)
{

	int nTimes{ 0 };

	for (auto &it : player_.knapsack()->FindAllEquip())
		nTimes += (player_.level() - it->level());

	if (nTimes <= target_num)
		CMission::ImplementMission(MTT_AC, 0, 0);
}

void CMission::__UpdateMissionToClient()
{
	//已接任务
	pto_MISSION_S2C_NTF_UpdateTakenMissionInfo ptoTakenMission;
	for (auto it : accept_mission_)
	{
		auto ptoMission = ptoTakenMission.add_taken_missions_info();
		ptoMission->set_mission_id(it.first);
		ptoMission->set_mission_object_counter(it.second);
	}

	player_.SendToAgent(ptoTakenMission, MISSION_S2C_NTF_UpdataTakenMissionInfo);

	//可接任务
	pto_MISSION_S2C_NTF_UpdateAvailableMissionList ptoAvailable;
	const CMissionTP* pMission = CMissionTP::GetMissionByMainProgress(main_progress_);
	if (pMission && !IsAcceptMission(pMission->GetMissionID()))
		ptoAvailable.add_available_mission_list(pMission->GetMissionID());

	std::vector<int> branch;
	CMissionTP::AvailableBranchList(&branch, player_.level(), main_progress_);
	for (auto it : branch)
	{
		if (!IsAcceptMission(it) &&
			!IsCompleteMission(it))
			ptoAvailable.add_available_mission_list(it);
	}

	ptoAvailable.set_player_main_line_progress(main_progress_);
	player_.SendToAgent(ptoAvailable, MISSION_S2C_NTF_UpdataAvailableMissionList);
}

void CMission::__LoadFromDatabase()
{
	std::ostringstream sql;
	sql << "select * from tb_player_mission where pid =" << player_.pid();

	ResultSetPtr result{ MYSQL_QUERY(sql.str()) };

	if (result && result->next())
	{
		main_progress_ = result->getInt("main_progress");
		if (0 == main_progress_)
			accept_mission_[1] = 0;

		auto temp_accept = CSerializer<int>::ParsePairFromString(result->getString("accept_mission").c_str());
		for (auto it : temp_accept)
			accept_mission_[it.first] = it.second;

		auto temp_complete = CSerializer<int>::ParseFromString(result->getString("complete_branch").c_str());
		for (auto it : temp_complete)
			complete_branch_.push_back(it);

		stage_normal_progress_ = result->getInt("stage_normal");
		stage_hard_progress_ = result->getInt("stage_hard");
		stage_nightmare_progress_ = result->getInt("stage_nightmare");
		stage_elite_progress_ = result->getInt("stage_elite");
		stage_multi_progress_ = result->getInt("stage_multi");
	}
	else
	{
		throw 5;
	}
}
