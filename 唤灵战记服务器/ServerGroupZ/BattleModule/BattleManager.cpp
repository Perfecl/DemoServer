#include "stdafx.h"
#include "BattleManager.h"
#include "StageBattle.h"
#include "EditMap.h"
#include "BuffTP.h"
#include "BulletTP.h"

CBattleManager* CBattleManager::instance_{ new CBattleManager };

CBattleManager::CBattleManager()
{

}

CBattleManager::~CBattleManager()
{
	if (timer_id_)
		timeKillEvent(timer_id_);
}

void CBattleManager::ProcessMessage(const CMessage& msg)
{
	switch (msg.GetProtocolID())
	{
	case BATTLE_S2B_REQ_CreateBattle:
		instance_->__CreateBattle(msg);
		break;
	case BATTLE_S2B_REQ_MatchBattle:
		break;
	default:
		instance_->lock_.lock();
		instance_->message_queue_.push(std::move(msg));
		instance_->lock_.unlock();
		break;
	}
}

void CBattleManager::StartBattleService(BattleCallback fun, void* param)
{
	instance_->callback_function_ = fun;
	instance_->callback_param_ = param;

	instance_->timer_id_ = timeSetEvent(kBattleLoopInterval, 1, [](UINT, UINT, DWORD_PTR dwUser, DWORD_PTR, DWORD_PTR)
	{
		CBattleManager* mgr{ reinterpret_cast<CBattleManager*>(dwUser) };

		mgr->Loop();

	}, reinterpret_cast<DWORD>(instance_), TIME_PERIODIC);
}

void CBattleManager::Loop()
{
	LOCK_BLOCK(lock_);
	while (!message_queue_.empty())
	{
		const CMessage& msg = message_queue_.front();

		auto it = player_battles_.find(msg.GetID());
		if (it != player_battles_.cend())
			it->second->ProcessMessage(msg);

		message_queue_.pop();
	}

	LOCK_BLOCK2(battle_lock_);
	for (auto it = battles_.begin(); it != battles_.cend();)
	{
		if (BtSt_Over == (*it)->GetBattleState())
		{
			(*it)->SendOverMessage((*it)->GetLoseForce());

			for (auto it_player = player_battles_.cbegin(); it_player != player_battles_.cend();)
			{
				if (*it == it_player->second)
					it_player = player_battles_.erase(it_player);
				else
					++it_player;
			}

			delete (*it);
			it = battles_.erase(it);
		}
		else
		{
			(*it)->Loop();
			++it;
		}
	}
}

void CBattleManager::LoadData()
{
	CBulletTP::Load();
	CBuffTP::Load();
	CEditMap::Load();
	CStageLoot::Load();
}

void CBattleManager::DoCallback(const std::string& content, unsigned char protocol_type, int protocol_id, int pid, int sid)
{
	DoCallback(CMessage{ content, protocol_type, protocol_id }, pid, sid);
}

void CBattleManager::DoCallback(CMessage msg, int pid, int sid)
{
	msg.SetID(pid);

	if (instance_->callback_function_)
		instance_->callback_function_(msg, instance_->callback_param_, pid, sid);
}

void CBattleManager::__CreateBattle(const CMessage& msg)
{
	pto_BATTLE_S2B_REQ_CreateBattle_Ex pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	bool result{ false };

	CBattle* battle{ nullptr };

	BattleMode mode{ (BattleMode)pto.mode() };

	if (pto.mode() == (int)BattleMode::ARENA1v1Easy || pto.mode() == (int)BattleMode::ARENA1v1Normal)
		mode = BattleMode::ARENA1v1;
	if (pto.mode() == (int)BattleMode::ARENA3v3Easy || pto.mode() == (int)BattleMode::ARENA3v3Normal)
		mode = BattleMode::ARENA3v3;

	if (IsStage(mode) && nullptr == CEditMap::GetEditMap(pto.map()))
		int(FormatString("找不到自定义地图", pto.map()).c_str());
	else
		battle = CBattle::NewBattle(mode);

	if (battle)
		result = true;

	pto_BATTLE_B2S_RES_CreateBattle pto_res;
	pto_res.set_result(result);
	pto_res.set_bsid(0);
	for (int i = 0; i < pto.atk_masters_size(); i++)
		pto_res.add_pids(pto.atk_masters(i).pid());
	for (int i = 0; i < pto.def_masters_size(); i++)
		pto_res.add_pids(pto.def_masters(i).pid());

	std::string str;
	pto_res.SerializeToString(&str);

	SEND_TO_GAME(str, MSG_B2S, BATTLE_B2S_RES_CreateBattle, msg.GetID(), pto.atk_sid());
	if (pto.atk_sid() != pto.def_sid())
		SEND_TO_GAME(str, MSG_B2S, BATTLE_B2S_RES_CreateBattle, msg.GetID(), pto.def_sid());

	if (battle)
	{
		for (int i = 0; i < pto.atk_masters_size(); i++)
			player_battles_[pto.atk_masters(i).pid()] = battle;
		for (int i = 0; i < pto.def_masters_size(); i++)
			player_battles_[pto.def_masters(i).pid()] = battle;

		battles_.insert(battle);

		//设置战斗
		if (pto.mode() != (int)BattleMode::ARENA1v1Easy && pto.mode() != (int)BattleMode::ARENA1v1Normal && pto.mode() != (int)BattleMode::ARENA3v3Easy && pto.mode() != (int)BattleMode::ARENA3v3Normal)
			battle->SetBattleByPto(&pto);
		else
			battle->SetAIBattleByPto(&pto);

		//初始化一些战斗数据
		battle->Initialize();
		battle->SetSID(pto.atk_sid(), pto.def_sid());
		battle->SetStageID(pto.stage_id());
		battle->SetDifficulty(pto.stage_difficulty());
		battle->SetBoxNum(pto.box_num());

		//发送战斗消息去客户端
		battle->SendBattleData();
	}
}
