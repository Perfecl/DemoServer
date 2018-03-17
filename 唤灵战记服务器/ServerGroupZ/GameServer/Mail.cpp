#include "stdafx.h"
#include "Mail.h"
#include "Player.h"
#include "Knapsack.h"
#include "Army.h"
#include "GameServer.h"

CMail::CMail()
{

}

CMail::~CMail()
{

}

void CMail::GetReward(SPPlayer player)
{
	for (auto &it : reward_items_)
	{
		switch (it.first)
		{
		case LT_Silver:
			player->ChangeSilver(it.second);
			break;
		case LT_Gold:
			player->ChangeGold((int)it.second,32);
			break;
		case LT_Exp:
			player->ChangeExp(it.second);
			break;
		case LT_Honour:
			player->ChangeHonor((int)it.second);
			break;
		case LT_Reputation:
			player->ChangeReputation((int)it.second);
			break;
		case LT_Stamina:
			player->ChangeStamina((int)it.second);
			break;
		default:
			player->knapsack()->GiveNewItem(it.first, (int)it.second);
			break;
		}
	}

	for (auto &it : reward_equips_)
		player->knapsack()->GiveNewEquip(it, 0);

	if (reward_hero_id_)
		player->army()->AddHero(reward_hero_id_, true);

	if (reward_soldier_id_)
		player->army()->AddSoldier(reward_soldier_id_);

	player->GiveNewTitle(reward_title_);
}

void CMail::SetProtocol(dat_PLAYER_STRUCT_Mail *pto)
{
	pto->set_id(id_);
	pto->set_model_id(model_id_);
	pto->set_recvtime(recv_time_);
	pto->set_is_read(is_read_);
	pto->set_is_get(false);

	for (auto it : reward_items_)
	{
		pto_RewardItem* pItem = pto->add_reward_item();
		pItem->set_num((__int64)it.second);
		pItem->set_id(it.first);
	}

	for (auto it : reward_equips_)
		pto->add_reward_equip(it);
	pto->set_hero_id(reward_hero_id_);
	pto->set_soldier_id(reward_soldier_id_);
	pto->set_name0(name_);
	pto->set_param0(param0_);
	pto->set_param1(param1_);
	pto->set_reward_title(reward_title_);
}

void CMail::InsertToDatabase(SPMail mail, int pid)
{
	CSerializer<__int64> ser_item;
	for (auto &it_item : mail->reward_items_)
		ser_item.AddPair(it_item.first, it_item.second);

	CSerializer<int> ser_equip;
	for (auto &it_equip : mail->reward_equips_)
		ser_equip.AddValue(it_equip);

	std::ostringstream sql;
	sql << "insert into tb_player_mail values(" << pid << "," << mail->model_id_ << "," << mail->param0_ << "," << mail->param1_ << ",'" << mail->name_ << "'," << mail->recv_time_
		<< "," << mail->is_read_ << "," << mail->reward_hero_id_ << "," << mail->reward_soldier_id_ << ",'" << ser_item.SerializerPairToString().c_str() << "','" << ser_equip.SerializerToString().c_str() << "'," << mail->reward_title_ << ")";

	MYSQL_UPDATE(sql.str());
}
