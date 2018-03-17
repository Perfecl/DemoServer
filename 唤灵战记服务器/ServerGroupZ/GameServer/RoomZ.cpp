#include "stdafx.h"
#include "RoomZ.h"
#include "Player.h"
#include "Mission.h"
#include <regex>

CRoomZ::CRoomZ(int room_id, BattleMode mode, int owner_pid) :
room_mode_{ mode },
room_id_{ room_id }
{
	for (size_t i = 0; i < slots_.size(); ++i)
		slots_[i].solt_id_ = i;

	//如果是跨服的,关闭一个阵营
	if (IsCrossRealm(mode) || room_mode_ == BattleMode::MULTISTAGE)
	{
		for (size_t i = 3; i < slots_.size(); ++i)
			slots_[i].is_enable_ = false;
	}

	if (mode == BattleMode::ARENA1v1)
	{
		slots_[1].is_enable_ = false;
		slots_[2].is_enable_ = false;
	}

	//设置房主
	slots_[owner_slot_id_].pid_ = owner_pid;
}

CRoomZ::~CRoomZ()
{

}

int	CRoomZ::GetMaxNum()
{
	int num{ 0 };

	LOCK_BLOCK(lock_);

	for (auto &it : slots_)
		if (it.is_enable_)
			num++;

	return num;
}

int	CRoomZ::GetPlayerNum()
{
	int num{ 0 };

	LOCK_BLOCK(lock_);

	for (auto &it : slots_)
	{
		if (it.is_enable_ && it.pid_)
			num++;
	}

	return num;
}

void CRoomZ::SendToAllPlayerInRoom(const std::string& content, unsigned char protocol_type, int protocol_id, int except_pid)
{
	LOCK_BLOCK(lock_);

	for (auto &it : slots_)
	{
		if (it.is_enable_ && it.pid_ && except_pid != it.pid_)
		{
			SPPlayer player = FIND_PLAYER(it.pid_);
			if (player)
				GAME_WORLD()->SendToAgentServer(content, protocol_type, protocol_id, it.pid_, player->session());
			else
				RECORD_WARNING(FormatString("发送房间消息时找不到玩家,pid", it.pid_));
		}
	}
}

int	CRoomZ::JoinRoom(int pid, std::string password)
{
	if (!IsWaiting())
		return 4;

	if (false == room_password_.empty())
	{
		if (room_password_ != password)
			return 2;
	}

	LOCK_BLOCK(lock_);

	for (auto &it : slots_)
	{
		if (it.is_enable_ && 0 == it.pid_)
		{
			SPPlayer player = FIND_PLAYER(pid);

			if (!player)
			{
				RECORD_WARNING(FormatString("加入房间时找不到玩家,pid", pid));
				return 1;
			}

			if (room_mode_ == BattleMode::MULTISTAGE)
			{
				const CEliteStage* stage = CEliteStage::GetMultiStageByLevel(room_map_id_);
				if (stage && stage->GetOpenLevel() > player->level())
					return 5;
				else if (stage && stage->GetLevel() > player->mission()->stage_multi_progress() + 1)
					return 5;
			}

			it.pid_ = pid;

			pto_ROOM_S2C_NTF_Join pto;
			pto.set_pid(pid);
			pto.set_slotid(it.solt_id_);
			pto.set_lv(player->level());
			pto.set_clothes(player->clothes());
			std::string str;
			pto.SerializeToString(&str);
			SendToAllPlayerInRoom(str, MSG_S2C, ROOM_S2C_NTF_Join, player->pid());

			return 0;
		}
	}

	return 1;
}

void CRoomZ::QuitRoom(int pid)
{
	std::string str;

	CRoomSlot* slot = __FindSlotByPID(pid);

	if (slot)
	{
		pto_ROOM_S2C_NTF_Quit ptoQuit;
		ptoQuit.set_slotid(slot->solt_id_);
		ptoQuit.SerializeToString(&str);
		SendToAllPlayerInRoom(str, MSG_S2C, ROOM_S2C_NTF_Quit, 0);

		slot->pid_ = 0;
		slot->is_ready_ = false;

		if (owner_slot_id_ == slot->solt_id_)
		{
			LOCK_BLOCK(lock_);

			for (auto &it : slots_)
			{
				if (it.is_enable_ && it.pid_)
				{
					//交换房主
					owner_slot_id_ = it.solt_id_;
					it.is_ready_ = false;

					pto_ROOM_S2C_NTF_ChangeOwner pto;
					pto.set_slotid(owner_slot_id_);
					pto.SerializeToString(&str);
					SendToAllPlayerInRoom(str, MSG_S2C, ROOM_S2C_NTF_ChangeOwner, pid);
					break;
				}
			}
		}
	}

	if (0 == GetPlayerNum())
		GAME_WORLD()->DeleteRoom(room_id_);
}

void CRoomZ::SetRoomIsLock(bool is_lock, const std::string& password, int pid)
{
	if (false == IsWaiting() || !IsOwner(pid))
		return;

	//相同状态就返回
	if (!room_password_.empty() == is_lock)
		return;

	int result{ 0 };

	if (is_lock)
	{
		std::wstring	w_password(UTF8_to_UNICODE(password));
		std::wregex		regex(L"[0-9]");
		size_t			length{ 0 };

		for (size_t i = 0; i < w_password.length(); i++)
		{
			if (regex_match(w_password.substr(i, 1), regex))
				length += 1;
			else
				result = 2;
		}

		if ((0 == result) && (length > 4 || length < 1))
			result = 2;

		if (0 == result)
			room_password_ = password;
	}
	else
	{
		room_password_.clear();
		result = 3;
	}

	pto_ROOM_S2C_NTF_Lock pto;
	pto.set_res(result);
	pto.set_is_lock(!room_password_.empty());
	pto.set_password(room_password_);
	std::string str;
	pto.SerializeToString(&str);
	SendToAllPlayerInRoom(str, MSG_S2C, ROOM_S2C_NTF_Lock, 0);
}

void CRoomZ::KickPlayer(int pid, int target_index)
{
	if (false == IsWaiting() || !IsOwner(pid))
		return;

	CRoomSlot* slot = __FindCellByIndex(target_index);

	if (!slot || !slot->is_enable_ || !slot->pid_)
		return;

	pto_ROOM_S2C_NTF_Kick pto;
	pto.set_slotid(slot->solt_id_);
	std::string str;
	pto.SerializeToString(&str);
	SendToAllPlayerInRoom(str, MSG_S2C, ROOM_S2C_NTF_Kick, 0);

	SPPlayer player = FIND_PLAYER(pid);
	if (player)
		player->OnRoomKick();
}

void CRoomZ::Ready(int pid, bool is_ready)
{
	if (false == IsWaiting() || IsOwner(pid))
		return;

	CRoomSlot* slot = __FindSlotByPID(pid);

	if (slot)
	{
		slot->is_ready_ = is_ready;

		pto_ROOM_S2C_NTF_Ready pto;
		pto.set_is_ready(slot->is_ready_);
		pto.set_solt_id(slot->solt_id_);
		std::string str;
		pto.SerializeToString(&str);
		SendToAllPlayerInRoom(str, MSG_S2C, ROOM_S2C_NTF_Ready, 0);
	}
}

void CRoomZ::ChangePos(int pid, int target_index)
{
	if (false == IsWaiting())
		return;

	CRoomSlot* slot = __FindSlotByPID(pid);
	if (!slot || slot->is_ready_)
		return;

	CRoomSlot* target_slot = __FindCellByIndex(target_index);
	if (!target_slot || !target_slot->is_enable_ || target_slot->pid_)
		return;

	{
		LOCK_BLOCK(lock_);

		if (owner_slot_id_ == slot->solt_id_)
			owner_slot_id_ = target_slot->solt_id_;

		target_slot->pid_ = slot->pid_;
		slot->pid_ = 0;
	}

	pto_ROOM_S2C_NTF_ChangePos pto;
	pto.set_source_id(slot->solt_id_);
	pto.set_target_id(target_slot->solt_id_);
	std::string str;
	pto.SerializeToString(&str);
	SendToAllPlayerInRoom(str, MSG_S2C, ROOM_S2C_NTF_ChangePos, 0);
}

void CRoomZ::Start(int pid)
{
	if (false == IsWaiting() || !IsOwner(pid))
		return;

	if (IsCrossRealm(room_mode_))
		__MatchBattle(pid);
	else
		__StartBattle(pid);
}

void CRoomZ::CancelAllReady()
{
	LOCK_BLOCK(lock_);

	for (auto &it : slots_)
	{
		if (it.is_enable_ && it.pid_ && owner_slot_id_ != it.solt_id_)
		{
			it.is_ready_ = false;

			std::string str;
			pto_ROOM_S2C_NTF_Ready pto;
			pto.set_is_ready(false);
			pto.set_solt_id(it.solt_id_);
			pto.SerializeToString(&str);
			SendToAllPlayerInRoom(str, MSG_S2C, ROOM_S2C_NTF_Ready, 0);
		}
	}
}

void CRoomZ::SetProtocol(pto_ROOM_STRUCT_RoomInfo* pto)
{
	if (nullptr == pto)
		return;

	pto->set_roomid(room_id_);
	pto->set_roomtype((int)room_mode_);
	pto->set_name(room_name_);
	pto->set_mapid(room_map_id_);

	if (room_password_.empty())
		pto->set_haslock(false);
	else
		pto->set_haslock(true);

	pto->set_password(room_password_);

	LOCK_BLOCK(lock_);

	for (auto &it : slots_)
	{
		auto cell = pto->add_slotlistatk();
		cell->set_isenable(it.is_enable_);

		if (it.pid_)
		{
			SPPlayer player = FIND_PLAYER(it.pid_);

			if (player)
			{
				cell->set_pid(player->pid());
				cell->set_lv(player->level());
				cell->set_module(player->clothes());
			}
		}
		else
		{
			cell->set_pid(0);
		}

		cell->set_isowner(owner_slot_id_ == it.solt_id_);
		cell->set_isready(it.is_ready_);
		cell->set_isai(false);
	}
}

void CRoomZ::SetProtocol(pto_ROOM_STRUCT_RoomSimpleInfo* pto)
{
	if (nullptr == pto)
		return;

	pto->set_roomid(room_id_);
	pto->set_roomtype((int)room_mode_);
	pto->set_name(room_name_);
	pto->set_mapid(room_map_id_);
	if (room_password_.empty())
		pto->set_haslock(false);
	else
		pto->set_haslock(true);

	pto->set_player_max_num(GetMaxNum());
	pto->set_player_num(GetPlayerNum());

	if (room_state_ == RoomState::BATTLING)
		pto->set_is_battle(true);
	else
		pto->set_is_battle(false);
}

bool CRoomZ::IsOwner(int pid)
{
	LOCK_BLOCK(lock_);

	for (auto &it : slots_)
	{
		if (it.is_enable_ && pid == it.pid_ && owner_slot_id_ == it.solt_id_)
			return true;
	}

	return false;
}

CRoomSlot* CRoomZ::__FindSlotByPID(int pid)
{
	LOCK_BLOCK(lock_);

	for (auto &it : slots_)
	{
		if (it.is_enable_ && it.pid_ == pid)
			return &it;
	}

	return nullptr;
}

CRoomSlot* CRoomZ::__FindCellByIndex(size_t index)
{
	LOCK_BLOCK(lock_);

	if (index < 0 || index >= static_cast<int>(slots_.size()))
		return nullptr;
	else
		return &slots_[index];
}

void CRoomZ::__StartBattle(int pid)
{
	int result{ 0 };

	LOCK_BLOCK(lock_);

	std::vector<int> atk_masters;
	std::vector<int> def_masters;

	for (auto &it : slots_)
	{
		if (it.is_enable_ && it.pid_)
		{
			if (it.is_ready_ || owner_slot_id_ == it.solt_id_)
			{
				auto player = FIND_PLAYER(it.pid_);

				if (!player)
				{
					RECORD_WARNING("开始战斗找不到玩家");
					return;
				}

				if (PlayerState::NORMAL != player->state())
				{
					result = 2;
					break;
				}

				if (it.solt_id_ < 3)
					atk_masters.push_back(it.pid_);
				else
					def_masters.push_back(it.pid_);
			}
			else
			{
				result = 1;
				break;
			}
		}
	}

	if (0 == result)
	{
		if (BattleMode::MULTISTAGE == room_mode_)
		{
			const CEliteStage* stage = CEliteStage::GetMultiStageByLevel(room_map_id_);
			if (stage)
				GAME_WORLD()->StartBattle(stage->GetMapID(), room_mode_, atk_masters, def_masters, stage->GetLevel(), 0, stage->ProduceBoxNum());
		}
		else
		{
			GAME_WORLD()->StartBattle(room_map_id_, room_mode_, atk_masters, def_masters);
		}
	}
	else
	{
		auto player = FIND_PLAYER(pid);
		if (player)
		{
			pto_ROOM_S2C_RES_Start pto;
			pto.set_result(result);
			player->SendToAgent(pto, ROOM_S2C_RES_Start);
		}
	}
}

void CRoomZ::__MatchBattle(int pid)
{
	if (!GAME_WORLD()->HasBattleServerConnect())
		return;

	int result{ 0 };

	LOCK_BLOCK(lock_);

	pto_BATTLE_S2B_REQ_MatchBattle_Ex pto;
	pto.set_is_match(true);
	pto.set_match_mode((int)room_mode_);
	pto.set_sid(GAME_WORLD()->sid());

	int point{ 0 };

	for (auto &it : slots_)
	{
		if (it.is_enable_ && it.pid_)
		{
			if (it.is_ready_ || owner_slot_id_ == it.solt_id_)
			{
				auto player = FIND_PLAYER(it.pid_);

				if (!player)
				{
					RECORD_WARNING("开始战斗找不到玩家");
					return;
				}

				if (PlayerState::NORMAL != player->state())
				{
					result = 2;
					break;
				}

				if (it.solt_id_ < 3)
					player->SetBattleMasterProto(pto.add_master(), IsFairMode(room_mode_));

				switch (room_mode_)
				{
				case BattleMode::ARENA1v1:
					point = point > player->ponit1v1() ? point : player->ponit1v1();
					pto.set_is_first(player->Is1v1FisrBattle());
					break;
				case BattleMode::ARENA3v3:
					point = point > player->ponit3v3() ? point : player->ponit3v3();
					break;
				default:
					RECORD_WARNING("异常的匹配模式");
					break;
				}
			}
			else
			{
				result = 1;
				break;
			}
		}
	}

	pto.set_point(point);

	auto player_owner = FIND_PLAYER(pid);

	if (!player_owner)
	{
		RECORD_WARNING("开始战斗找不到玩家");
		return;
	}

	if (result == 0)
	{
		room_state_ = RoomState::MATCHING;

		for (auto &it : slots_)
		{
			if (it.is_enable_ && it.pid_)
			{
				auto player = FIND_PLAYER(it.pid_);

				if (!player)
				{
					RECORD_WARNING("开始战斗找不到玩家");
					return;
				}

				player->state(PlayerState::MATCH);
			}
		}

		std::string str;
		pto.SerializeToString(&str);

		GAME_WORLD()->SendToBattleServer(str, MSG_S2B, BATTLE_S2B_REQ_MatchBattle, player_owner->pid());
		pto_ROOM_S2C_NTF_MatchBattle ptoNTF;
		ptoNTF.set_is_match(true);
		ptoNTF.SerializeToString(&str);
		SendToAllPlayerInRoom(str, MSG_S2C, ROOM_S2C_NTF_MatchBattle, 0);
	}
	else
	{
		pto_ROOM_S2C_RES_Start pto_start;
		pto_start.set_result(result);
		player_owner->SendToAgent(pto_start, ROOM_S2C_RES_Start);
	}
}