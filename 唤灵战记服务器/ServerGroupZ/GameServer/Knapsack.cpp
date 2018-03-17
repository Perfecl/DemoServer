#include "stdafx.h"
#include "Knapsack.h"
#include "GameServer.h"
#include "Item.h"
#include "Equip.h"
#include "Mail.h"
#include "Mission.h"
#include "GameWorld.h"
#include "Town.h"
#include "Army.h"
#include "Mail.h"
#include "GameWorld.h"
#include "HeroCard.h"
#include "Player.h"

CKnapsack::CKnapsack(CPlayer& player) :
player_{ player }
{
	__LoadFromDatabase();
}

CKnapsack::~CKnapsack()
{
	for (auto &it : items_)
		item_pool_.destroy(it.second);
	for (auto &it : temporary_items_)
		item_pool_.destroy(it.second);
	for (auto &it : equips_)
		delete it.first;
	for (auto &it : fashions_)
		delete it.second;
}

void CKnapsack::ProcessMessage(const CMessage& msg)
{
	switch (msg.GetProtocolID())
	{
	case PLAYER_C2S_REQ_ExchangeEquip:		__ExchangeEquip(msg);					break;
	case PLAYER_C2S_REQ_UseItem:			__UseItem(msg);							break;
	case PLAYER_C2S_REQ_RemoveItem:			__DeleteItem(msg);						break;
	case PLAYER_C2S_REQ_SellItem:			__SellItem(msg);						break;
	case PLAYER_C2S_REQ_ItemToBackpack:		__PutItemToKnapsack(msg);				break;
	case PLAYER_C2S_REQ_EquipLvUp:			__UpgradeEquip(msg);					break;
	case PLAYER_C2S_NTF_CloseLottery:		__CloseLottery();						break;
	case PLAYER_C2S_REQ_ChooseLottery:		__ChooseLottery(msg);					break;
	case PLAYER_C2S_REQ_ReadMail:			__ReadMail(msg);						break;
	case PLAYER_C2S_REQ_GetMailReward:		__GetMailReward(msg);					break;
	case PLAYER_C2S_REQ_DeleteMail:			__DeleteMail(msg);						break;
	case PLAYER_C2S_REQ_GetMailList:		__GetMailList(msg);						break;
	case PLAYER_C2S_REQ_MakeEquip:			__MakeEquip(msg);						break;
	case PLAYER_C2S_REQ_ActivateEquipEnchanting:__ActivateEquipEnchanting(msg);		break;
	case PLAYER_C2S_REQ_Enchanting:			__Enchanting(msg);						break;
	case PLAYER_C2S_REQ_ExchangeFashion:	__ExchangeFashion(msg);					break;
	case PLAYER_C2S_REQ_BuyFashion:			__BuyFashion(msg);						break;
	case PLAYER_C2S_REQ_RecastEquip:		__RecastEquip(msg);						break;
	case PLAYER_C2S_REQ_ResolveEquip:		__ResolveEquip(msg);					break;
	case PLAYER_C2S_REQ_GetFashion:			__GetFashion(msg);						break;
	case PLAYER_C2S_REQ_BatchSell:			__BatchSell(msg);						break;
	case PLAYER_C2S_REQ_BuyInMall:			__BuyInMall(msg);						break;
	case PLAYER_C2S_REQ_Inherit:			__Inherit(msg);							break;
	default:RECORD_WARNING(FormatString("未知的Kapsack协议号:", msg.GetProtocolID()).c_str()); break;
	}
}

void CKnapsack::SendInitialMessage()
{
	pto_ITEM_S2C_NTF_INIT pto;

	Equip_Model* pto_equip{ nullptr };

	for (auto &it : equips_)
	{
		switch (it.second)
		{
		case IL_STROEROOM:	pto_equip = pto.add_equip2(); break;
		case IL_KNAPSACK:	pto_equip = pto.add_equip1(); break;
		default:			pto_equip = pto.add_equip0(); break;
		}

		if (!pto_equip)
			continue;

		it.first->SetProtocol(pto_equip);

		pto_equip->set_hero_id(abs(it.second));
	}

	for (auto &it : items_)
	{
		auto pto_item = pto.add_item1();
		pto_item->set_id(it.second->GetID());
		pto_item->set_num(it.second->quantity());
	}
	for (auto &it : temporary_items_)
	{
		auto pto_item = pto.add_item2();
		pto_item->set_id(it.second->GetID());
		pto_item->set_num(it.second->quantity());
	}

	for (auto &it : fashions_)
	{
		auto pto_fashion = pto.add_fashions();
		pto_fashion->set_fashion_id(it.second->id);
		pto_fashion->set_due_time(it.second->due_time);
		pto_fashion->set_is_on_body(it.second == body_fashion_);
	}

	player_.SendToAgent(pto, PLAYER_S2C_NTF_UpdataPlayerBackpack);
}

void CKnapsack::SaveToDatabase()
{
	std::string str_sql;
	std::ostringstream sql;
	sql << "delete from tb_player_item where pid = " << player_.pid();
	MYSQL_UPDATE(sql.str());

	if (!items_.empty())
	{
		sql.str("");
		sql << "insert into tb_player_item values";
		for (auto &it : items_)
			sql << "(" << player_.pid() << ", " << it.second->GetID() << ", " << it.second->quantity() << ", " << it.second->get_time() << ", " << 0 << "),";
		str_sql = std::move(sql.str());
		str_sql.erase(str_sql.length() - 1, 1);
		MYSQL_UPDATE(str_sql);
	}

	if (!temporary_items_.empty())
	{
		sql.str("");
		sql << "insert into tb_player_item values";
		for (auto &it : temporary_items_)
			sql << "(" << player_.pid() << ", " << it.second->GetID() << ", " << it.second->quantity() << ", " << it.second->get_time() << ", " << 1 << "),";
		str_sql = std::move(sql.str());
		str_sql.erase(str_sql.length() - 1, 1);
		MYSQL_UPDATE(str_sql);
	}


	sql.str("");
	sql << "delete from tb_player_equip where pid = " << player_.pid();
	MYSQL_UPDATE(sql.str());
	if (!equips_.empty())
	{
		sql.str("");
		sql << "insert into tb_player_equip values";
		for (auto &it : equips_)
		{
			CSerializer<int> ser_gem;
			for (auto &it_gem : it.first->gems_)
				ser_gem.AddValue(it_gem);
			sql << "(" << player_.pid() << ", " << it.first->GetID() << ", " << it.first->level_ << ", " << it.first->quality_ << ", " << it.first->suit_id_ << ", " << it.first->enchanting1_.first << ", "
				<< it.first->enchanting1_.second << "," << it.first->enchanting2_.first << "," << it.first->enchanting2_.second << "," << it.first->enchanting2_is_active_ << ",'" << ser_gem.SerializerToString().c_str() << "',"
				<< it.first->get_time() << "," << it.second << "),";
		}
		str_sql = std::move(sql.str());
		str_sql.erase(str_sql.length() - 1, 1);
		MYSQL_UPDATE(str_sql);
	}

	sql.str("");
	sql << "delete from tb_player_mail where pid = " << player_.pid();
	MYSQL_UPDATE(sql.str());

	//邮件
	if (!mails_.empty())
	{
		sql.str("");
		sql << "insert into tb_player_mail values";

		for (auto &it : mails_)
		{
			CSerializer<__int64> ser_item;
			for (auto &it_item : it.second->reward_items_)
				ser_item.AddPair(it_item.first, it_item.second);
			CSerializer<int> ser_equip;
			for (auto &it_equip : it.second->reward_equips_)
				ser_equip.AddValue(it_equip);
			sql << "(" << player_.pid() << ", " << it.second->model_id_ << ", " << it.second->param0_ << ", " << it.second->param1_ << ", '" << it.second->name_ << "', " << it.second->recv_time_
				<< "," << it.second->is_read_ << "," << it.second->reward_hero_id_ << "," << it.second->reward_soldier_id_ << ",'" << ser_item.SerializerPairToString().c_str() << "','" << ser_equip.SerializerToString().c_str() << "'," << it.second->reward_title_ << "),";
		}
		str_sql = std::move(sql.str());
		str_sql.erase(str_sql.length() - 1, 1);
		MYSQL_UPDATE(str_sql);
	}


	//时装
	if (!fashions_.empty())
	{
		sql.str("");
		sql << "replace into tb_player_fashion values";

		for (auto &it : fashions_)
		{
			bool is_on_body{ false };
			if (body_fashion_ == it.second)
				is_on_body = true;
			sql << "(" << player_.pid() << ", " << it.second->id << ", " << it.second->get_time << ", " << it.second->due_time << ", " << is_on_body << "),";
		}
		str_sql = std::move(sql.str());
		str_sql.erase(str_sql.length() - 1, 1);
		MYSQL_UPDATE(str_sql);
	}
}

void CKnapsack::GiveNewEquip(int equip_id, int quality)
{
	if (equip_id <= 0)
		return;

	CEquip *equip{ new CEquip(equip_id) };

	if (nullptr == equip->equip_template_)
	{
		RECORD_WARNING(FormatString("给与物品失败，无此ID", equip_id).c_str());
		delete equip;
		return;
	}

	equip->unique_id_ = equip_unique_id_allcoator_++;
	equip->suit_id_ = equip->equip_template_->ProduceSuitID(0);
	equip->quality_ = quality;

	ItemLocation location{ IL_KNAPSACK };

	if (IsFull())
		location = IL_STROEROOM;

	{
		LOCK_BLOCK(lock_);
		equips_[equip] = location;
		equip->get_time(time(0));
	}

	pto_EQUIP_S2C_NTF_ADD pto;
	pto.set_bagtype(location);

	auto pto_equip = pto.mutable_equip();
	equip->SetProtocol(pto_equip);

	player_.SendToAgent(pto, PLAYER_S2C_NTF_PlayerGetEquip);

}

void CKnapsack::GiveNewItem(int item_id, int num, bool send_msg)
{
	if (item_id <= 0 || num <= 0)
		return;

	const CItemTP* item_template{ CItemTP::GetItemTP(item_id) };

	if (nullptr == item_template)
	{
		RECORD_WARNING("不存在的道具");
		return;
	}

	int			surplus{ num };
	const int	cell_max_num{ item_template->GetMaxNum() };

	CItem* item{ FindItem(item_id, IL_KNAPSACK) };

	//可以放下数数量
	int can_contain = (CVIPFun::GetVIPFun(player_.vip_level(), VIPFunType::VFT_Bag) - __CalculateKnapsackSpace()) * cell_max_num;
	if (can_contain < 0)
		can_contain = 0;

	if (item)
	{
		int temp{ item->quantity() % cell_max_num };
		if (temp)
			can_contain += cell_max_num - temp;
	}
	else if (can_contain > 0)
	{
		item = item_pool_.construct(item_id);
		items_[item_id] = item;
		item->get_time(time(0));
	}

	if (can_contain > 0)
	{
		if (surplus <= can_contain)
		{
			item->ChangeQuantity(surplus);
			surplus = 0;
		}
		else
		{
			item->ChangeQuantity(can_contain);
			surplus -= can_contain;
		}

		if (send_msg)
			__SendItemProtocol(item_id, num - surplus, IL_KNAPSACK);
	}

	if (surplus > 0)
	{
		CItem* item_store{ FindItem(item_id, IL_STROEROOM) };

		if (nullptr == item_store)
		{
			item_store = item_pool_.construct(item_id);
			temporary_items_[item_id] = item_store;
			item_store->get_time(time(0));
		}

		item_store->ChangeQuantity(surplus);

		if (send_msg)
			__SendItemProtocol(item_id, surplus, IL_STROEROOM);

		surplus = 0;
	}
}

CItem* CKnapsack::FindItem(int item_id, ItemLocation location)
{
	LOCK_BLOCK(lock_);

	if (IL_KNAPSACK == location)
	{
		auto it = items_.find(item_id);
		if (items_.cend() == it)
			return nullptr;
		else
			return it->second;

	}
	else if (IL_STROEROOM == location)
	{
		auto it = temporary_items_.find(item_id);
		if (temporary_items_.cend() == it)
			return nullptr;
		else
			return it->second;
	}
	else
	{
		return nullptr;
	}

}

void CKnapsack::DeleteItem(int item_id, ItemLocation location)
{
	LOCK_BLOCK(lock_);

	if (IL_KNAPSACK == location)
	{
		auto it = items_.find(item_id);
		if (items_.cend() != it)
		{
			item_pool_.destroy(it->second);
			items_.erase(it);
		}
	}
	else if (IL_STROEROOM == location)
	{
		auto it = temporary_items_.find(item_id);
		if (temporary_items_.cend() != it)
		{
			item_pool_.destroy(it->second);
			temporary_items_.erase(it);
		}
	}
}

CEquip*	CKnapsack::FindEquip(ItemLocation location, EquipPart part)
{
	LOCK_BLOCK(lock_);

	for (auto &it : equips_)
	{
		if (location == it.second && part == it.first->equip_template_->type())
			return it.first;
	}

	return nullptr;
}

CEquip*	CKnapsack::FindEquip(int unique_id)
{
	LOCK_BLOCK(lock_);

	for (auto &it : equips_)
	{
		if (unique_id == it.first->unique_id())
			return it.first;
	}

	return nullptr;
}

std::vector<const CEquip*> CKnapsack::FindAllEquip()
{
	LOCK_BLOCK(lock_);

	std::vector<const CEquip*> temp;

	for (auto &it : equips_)
		temp.push_back(it.first);

	return std::move(temp);
}

SPMail	CKnapsack::FindMail(int mail_id)
{
	LOCK_BLOCK(lock_);

	auto it = mails_.find(mail_id);

	if (mails_.cend() == it)
		return nullptr;
	else
		return it->second;
}

bool CKnapsack::DeleteMail(int mail_id)
{
	LOCK_BLOCK(lock_);

	auto it = mails_.find(mail_id);

	if (mails_.cend() != it)
	{
		mails_.erase(mail_id);
		return true;
	}
	return false;
}

void CKnapsack::AddMail(SPMail mail, bool send_msg)
{
	LOCK_BLOCK(lock_);

	mail->id_ = mail_unique_id_allcoator_++;

	if (mails_.insert(std::make_pair(mail->id_, mail)).second)
	{
		if (send_msg)
		{
			pto_PLAYER_S2C_NTF_NewMail pto;
			mail->SetProtocol(pto.mutable_mail());
			player_.SendToAgent(pto, PLAYER_S2C_NTF_NewMail);
		}
	}
}

bool CKnapsack::HasUnreadMails()
{
	LOCK_BLOCK(lock_);

	for (auto &it : mails_)
	{
		if (false == it.second->is_read_)
			return true;
	}

	return false;
}

std::vector<const CEquip*>	CKnapsack::GetHeroEquip(int hero_id)
{
	LOCK_BLOCK(lock_);

	std::vector<const CEquip*> temp;

	if (hero_id <= 0)
		return std::move(temp);

	for (auto &it : equips_)
	{
		if (hero_id == -it.second)
			temp.push_back(it.first);
	}

	return std::move(temp);
}

int	CKnapsack::clothes_id() const
{
	if (body_fashion_)
		return body_fashion_->id;

	return 0;
}

bool CKnapsack::IsFull()
{
	return __CalculateKnapsackSpace() >= CVIPFun::GetVIPFun(player_.vip_level(), VIPFunType::VFT_Bag);
}

bool CKnapsack::MoveItem(bool is_equip, int id, int num, ItemLocation src, ItemLocation dest)
{
	if (is_equip)
	{
		LOCK_BLOCK(lock_);

		for (auto &it : equips_)
		{
			if (it.first && id == it.first->unique_id_)
			{
				if (IL_NULL == dest)
				{
					delete it.first;
					equips_.erase(it.first);
				}
				else
				{
					it.second = dest;
				}

				return true;
			}
		}

		return false;
	}
	else
	{
		CItem* src_item{ FindItem(id, src) };

		if (!src_item || num > src_item->quantity() || num < 0)
			return false;

		if (src_item->ChangeQuantity(-num) <= 0)
			DeleteItem(id, src);

		if (IL_KNAPSACK == dest)
			GiveNewItem(id, num, false);

		return true;
	}
}

void CKnapsack::AddFashion(int id)
{
	if (!id)
		return;

	int		fashion_id{ id };
	time_t	add_time{ -1 };

	int result{ 0 };

	const CFashionTP* fashion_template = CFashionTP::GetFashionTP(fashion_id);

	CFashion*	fashion{ nullptr };

	if (fashion_template)
	{
		auto it = fashions_.find(fashion_id);
		if (fashions_.cend() == it)
		{
			fashion = new CFashion;
			fashion->id = fashion_id;
			fashion->get_time = time(0);

			fashion->due_time = add_time;

			fashions_[fashion_id] = fashion;
		}
		else
		{
			fashion = it->second;
			fashion->due_time += add_time;
		}
	}
	else
	{
		result = 1;
	}

	pto_PLAYER_S2C_RES_BuyFashion pto_res;
	pto_res.set_result(result);
	if (0 == result)
	{
		auto pto_fashion = pto_res.mutable_fashion();
		pto_fashion->set_fashion_id(fashion->id);
		pto_fashion->set_due_time(fashion->due_time);
		pto_fashion->set_is_on_body(fashion == body_fashion_);
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_BuyFashion);
}

void CKnapsack::SendUseItem(int item_id, int num)
{
	pto_ITEM_S2C_NTF_UPDATE pto_update;
	Item_Model* pto_item = pto_update.mutable_item();
	CItem* item = FindItem(item_id, ItemLocation::IL_KNAPSACK);
	pto_item->set_id(item_id);
	pto_item->set_num(num);
	pto_update.set_bagtype(ItemLocation::IL_KNAPSACK);
	player_.SendToAgent(pto_update, PLAYER_S2C_NTF_UpdateItem);
}

void CKnapsack::GiveSuit(int suit_id)
{
	for (size_t i = 701; i <= 706; i++)
	{
		CEquip *equip{ new CEquip(i) };

		if (nullptr == equip->equip_template_)
		{
			RECORD_WARNING(FormatString("给与物品失败，无此ID", i).c_str());
			delete equip;
			return;
		}

		equip->unique_id_ = equip_unique_id_allcoator_++;
		equip->suit_id_ = suit_id;
		equip->quality_ = 5;

		ItemLocation location{ IL_KNAPSACK };

		if (IsFull())
			location = IL_STROEROOM;

		{
			LOCK_BLOCK(lock_);
			equips_[equip] = location;
			equip->get_time(time(0));
		}

		pto_EQUIP_S2C_NTF_ADD pto;
		pto.set_bagtype(location);

		auto pto_equip = pto.mutable_equip();
		equip->SetProtocol(pto_equip);

		player_.SendToAgent(pto, PLAYER_S2C_NTF_PlayerGetEquip);
	}
}

int	CKnapsack::__CalculateKnapsackSpace()
{
	int num{ 0 };

	LOCK_BLOCK(lock_);

	for (auto &it : items_)
	{
		num += it.second->quantity() / it.second->cell_max_num();

		if (it.second->quantity() % it.second->cell_max_num())
			++num;
	}

	for (auto &it : equips_)
	{
		if (IL_KNAPSACK == it.second)
			++num;
	}

	return num;
}

void CKnapsack::__SendItemProtocol(int item_id, int num, ItemLocation location)
{
	pto_ITEM_S2C_NTF_ADD pto;
	Item_Model* pto_item = pto.mutable_item();
	pto_item->set_id(item_id);
	pto_item->set_num(num);
	pto.set_bagtype(location);
	player_.SendToAgent(pto, PLAYER_S2C_NTF_PlayerGetItem);
}

void CKnapsack::__PutItemToKnapsack(const CMessage& msg)
{
	pto_ITEM_C2S_REQ_ToBackpack pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_ITEM_S2C_RES_ToBackpack pto_res;

	if (pto.num() < 1)
		pto_res.set_res(1);
	else if (IsFull())
		pto_res.set_res(2);
	else
	{
		if (MoveItem(1 == pto.item_type(), pto.id(), pto.num(), IL_STROEROOM, IL_KNAPSACK))
			pto_res.set_res(0);
		else
			pto_res.set_res(1);
	}

	pto_res.set_id(pto.id());
	pto_res.set_num(pto.num());
	pto_res.set_item_type(pto.item_type());
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_ItemToBackpack);
}

void CKnapsack::__DeleteItem(const CMessage& msg)
{
	pto_ITEM_C2S_REQ_REMOVE pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	ItemLocation location{ static_cast<ItemLocation>(pto.bagtype()) };

	pto_ITEM_S2C_RES_REMOVE pto_res;

	if (pto.num() < 1)
		pto_res.set_res(1);
	else if (MoveItem(1 == pto.itemtype(), pto.id(), pto.num(), location, IL_NULL))
		pto_res.set_res(0);
	else
		pto_res.set_res(1);

	pto_res.set_id(pto.id());
	pto_res.set_itemtype(pto.itemtype());
	pto_res.set_num(pto.num());
	pto_res.set_bagtype(pto.bagtype());

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_RemoveItem);
}

void CKnapsack::__SellItem(const CMessage& msg)
{
	pto_ITEM_C2S_REQ_SELL pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	ItemLocation location{ static_cast<ItemLocation>(pto.bagtype()) };

	__int64 price{ 0 };

	int result{ 0 };

	if (0 == pto.itemtype())
	{
		CItem* item{ FindItem(pto.id(), location) };

		if (nullptr == item)
			result = 2;
		else if (pto.num() < 1 || pto.num() > item->quantity())
			result = 3;
		else if (item->item_template_->GetSellPrice() <= 0)
			result = 4;

		if (0 == result)
		{
			price = item->item_template_->GetSellPrice() * pto.num();

			if (item->ChangeQuantity(-pto.num()) <= 0)
				DeleteItem(pto.id(), location);
		}
	}
	else
	{
		LOCK_BLOCK(lock_);

		result = 1;
		for (auto &it : equips_)
		{
			if (it.first && pto.id() == it.first->unique_id_ &&
				it.second >= 0)
			{
				price = it.first->equip_template_->sell_price() * (it.first->quality() + 1);

				for (int i = 1; i <= it.first->level(); i++)
					price += __int64(CEquipTP::GetImproveEquipPrice(i, it.first->equip_template_->type()) * 0.7f);

				delete it.first;
				equips_.erase(it.first);
				result = 0;
				break;
			}
		}
	}

	if (price > 0)
		player_.ChangeSilver(price);

	pto_ITEM_S2C_RES_SELL pto_res;
	pto_res.set_res(result);
	pto_res.set_bagtype(pto.bagtype());
	pto_res.set_id(pto.id());
	pto_res.set_itemtype(pto.itemtype());
	pto_res.set_num(pto.num());
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_SellItem);
}

void CKnapsack::__UseItem(const CMessage& msg)
{
	pto_ITEM_C2S_REQ_USE pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_ITEM_S2C_RES_USE pto_res;

	int result{ 0 };

	CItem* item{ FindItem(pto.id(), IL_KNAPSACK) };

	if (nullptr == item)
		result = 1;
	else if (item->item_template_->GetUseLevel() > player_.level())
		result = 2;
	else if (item->quantity() < pto.num())
		result = 3;
	else if (pto.num() < 1)
		result = 3;

	int used_num{ 0 };

	if (0 == result)
	{
		for (; used_num < pto.num(); used_num++)
		{
			if (ITEM_ATTRIBUTE::IA_OPEN == item->item_template_->GetType())
			{
				__GetOpenReward(COpenTP::GetOpenTP(item->item_template_->GetFunctional()));
			}
			else if (ITEM_ATTRIBUTE::IA_LOTTERY == item->item_template_->GetType())
			{
				__StartLottery(item->item_template_->GetFunctional());
			}
			else if (ITEM_ATTRIBUTE::IA_HeroExp == item->item_template_->GetType())
			{
				CHeroCard* pHero = player_.army()->FindHero(pto.user_id());

				if (pHero)
				{
					if (pHero->hero_level() >= player_.level())
					{
						if (pto.num() == used_num)
						{
							result = 2;
							pto_res.set_num(pto.num());
						}
						else
						{
							result = 0;
							pto_res.set_num(pto.num() - used_num);
						}
						break;
					}
					pto_PLAYER_S2C_RES_ExchangeHeroExp pto_hero_exp;
					pto_hero_exp.set_res(0);
					pto_hero_exp.set_hero_id(pto.user_id());

					int get_exp = item->item_template_->GetFunctional();
					int max_exp = pHero->hero_level() - pHero->hero_exp();

					if (get_exp >= max_exp)
					{
						pHero->ChangeLevel(1);
						pHero->SetExp(0);
						pto_hero_exp.set_level_up(true);
						pto_hero_exp.set_exp(max_exp);
						player_.ImplementMission(MTT_HeroLevel, pHero->hero_id(), pHero->hero_level());
					}
					else
					{
						pHero->ChangeExp(get_exp);
						pto_hero_exp.set_level_up(false);
						pto_hero_exp.set_exp(get_exp);
					}

					int exchange_cd = 0;
					if (player_.army()->practice_seconds() <= time(0) - player_.army()->last_practice_time())
						exchange_cd = 0;
					else
						exchange_cd = int(player_.army()->practice_seconds() - (time(0) - player_.army()->last_practice_time()));

					pto_hero_exp.set_exchange_cd(exchange_cd);

					player_.SendToAgent(pto_hero_exp, PLAYER_S2C_RES_ExchangeHeroExp);
				}
			}
		}

		MoveItem(false, pto.id(), used_num, IL_KNAPSACK, IL_NULL);

		pto_res.set_res(result);
		pto_res.set_id(pto.id());
		pto_res.set_num(used_num);
		player_.SendToAgent(pto_res, PLAYER_S2C_RES_UseItem);
	}
}

void CKnapsack::__ExchangeEquip(const CMessage& msg)
{
	pto_EQUIP_C2S_REQ_EXCHANGE pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());
	int to_body_id{ pto.id0() };
	int to_bag_id{ pto.id1() };
	int hero_id{ pto.hero_id() };

	CHeroCard* hero = player_.army()->FindHero(hero_id);

	if (nullptr == hero)
	{
		RECORD_WARNING(FormatString("找不到英雄", hero_id).c_str());
		return;
	}

	int result{ 0 };

	CEquip*	equip{ nullptr };

	if (to_body_id)
	{
		equip = FindEquip(to_body_id);

		if (!equip)
		{
			result = 1;
		}
		else if (false == IsEquip(equip->equip_template_->type()))
		{
			result = 3;
		}
		else if (hero->hero_level() < equip->equip_template_->use_level())
		{
			result = 2;
		}
		else
		{
			ItemLocation location{ equips_[equip] };

			CEquip* using_equip{ FindEquip(static_cast<ItemLocation>(-hero_id), equip->equip_template_->type()) };

			equips_[equip] = static_cast<ItemLocation>(-hero_id);

			if (using_equip)
			{
				to_bag_id = using_equip->unique_id();
				equips_[using_equip] = location;
			}

			player_.ImplementMission(EMissionTargetType::MTT_UseEquip, static_cast<int>(equip->equip_template_->type()), 0);
		}
	}
	else if (to_bag_id)
	{
		equip = FindEquip(to_bag_id);

		if (!equip)
			result = 1;
		else if (IsFull())
			result = 2;
		else
			equips_[equip] = IL_KNAPSACK;
	}

	pto_EQUIP_S2C_RES_EXCHANGE pto_res;
	pto_res.set_res(result);
	pto_res.set_id0(to_body_id);
	pto_res.set_id1(to_bag_id);
	pto_res.set_hero_id(pto.hero_id());
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_ExchangeEquip);
}

void CKnapsack::__GetOpenReward(const COpenTP* open)
{
	if (!open)
		return;

	player_.ChangeExp(open->m_nRewardExp);
	player_.ChangeSilver(open->m_nRewardSilver);
	player_.ChangeHonor(open->m_nRewardHonour);
	player_.ChangeReputation(open->m_nRewardReputation);
	player_.ChangeGold(open->m_nRewardGold,33);
	player_.ItemChangeStamina(open->m_nRewardStamina);

	GiveNewItem(open->m_nRewardItem1ID, open->m_nRewardItem1Num);
	GiveNewItem(open->m_nRewardItem2ID, open->m_nRewardItem2Num);
	GiveNewItem(open->m_nRewardItem3ID, open->m_nRewardItem3Num);
	GiveNewItem(open->m_nRewardItem4ID, open->m_nRewardItem4Num);
	GiveNewItem(open->m_nRewardItem5ID, open->m_nRewardItem5Num);
	GiveNewItem(open->m_nRewardItem6ID, open->m_nRewardItem6Num);

	GiveNewEquip(open->m_nRewardEquip1ID, 0);
	GiveNewEquip(open->m_nRewardEquip2ID, 0);
	GiveNewEquip(open->m_nRewardEquip3ID, 0);
	GiveNewEquip(open->m_nRewardEquip4ID, 0);
}

void CKnapsack::__UpgradeEquip(const CMessage& msg)
{
	if (!player_.IsOpenGuide(1))
		return;

	pto_EQUIP_C2S_REQ_EquipLvUp pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_EQUIP_S2C_RES_EquipLvUp pto_res;
	pto_res.set_uniqueid(pto.uniqueid());

	CEquip* equip{ FindEquip(pto.uniqueid()) };

	int		result{ 0 };
	int		crit{ 0 };

	int		times{ 1 };
	if (pto.batch())
		times = 10;

	if (equip)
	{
		__int64 need_price = CEquipTP::GetImproveEquipPrice(equip->level() + 1, equip->equip_template_->type());

		if (player_.level() <= equip->level())
		{
			result = 2;
		}
		else if (player_.silver() < need_price || (0 == need_price))
		{
			result = 3;
		}
		else if (equip->level() >= player_.level())
		{
			result = 4;
		}
		else if (false == IsEquip(equip->equip_template_->type()))
		{
			result = 5;
		}
		else
		{
			while (times > 0 && player_.silver() > need_price && player_.level() > equip->level())
			{
				equip->level_up();

				player_.ChangeSilver(-need_price);

				if (CVIPFun::GetVIPFun(player_.vip_level(), VIPFunType::VFT_UpgradeEquipCrit) && GetRandom(0, 100) <= 10)
				{
					equip->level_up();
					crit++;
				}
				pto_res.set_level(equip->level());
				pto_res.set_crit_times(crit);

				times--;
				need_price = CEquipTP::GetImproveEquipPrice(equip->level() + 1, equip->equip_template_->type());
				player_.ImplementMission(EMissionTargetType::MTT_EquipLvUpTimes, (int)equip->equip_template_->type(), 0);
				player_.ImplementRewardMission(RewardMissionType::RMT_UpgradeEquip, 0, 1);
			}

			player_.ImplementMission(EMissionTargetType::MTT_EquipLvUpLv, (int)equip->equip_template_->type(), equip->level());
			player_.ImplementMission(EMissionTargetType::MTT_AllEquipLevel, 0, 0);
		}
	}
	else
	{
		result = 1;
	}

	pto_res.set_res(result);
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_EquipLvUp);
}

void CKnapsack::__ReadMail(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_ReadMail pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_ReadMail pto_res;

	pto_res.set_id(pto.id());
	pto_res.set_res(0);

	LOCK_BLOCK(lock_);

	SPMail mail{ FindMail(pto.id()) };

	if (!mail)
		pto_res.set_res(1);
	else
		mail->is_read_ = true;

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_ReadMail);
}

void CKnapsack::__GetMailReward(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_GetMailReward pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_GetMailReward ptoS2C;
	ptoS2C.set_res(0);

	LOCK_BLOCK(lock_);

	for (int i = 0; i < pto.id_size(); i++)
	{
		ptoS2C.add_id(pto.id(i));

		SPMail pMail{ FindMail(pto.id(i)) };

		if (pMail)
		{
			pMail->GetReward(FIND_PLAYER(player_.pid()));

			DeleteMail(pto.id(i));
		}
	}

	player_.SendToAgent(ptoS2C, PLAYER_S2C_RES_GetMailReward);
}

void CKnapsack::__DeleteMail(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_DeleteMail pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_DeleteMail ptoS2C;
	ptoS2C.set_res(0);

	for (int i = 0; i < pto.id_size(); i++)
	{
		ptoS2C.add_id(pto.id(i));
		DeleteMail(pto.id(i));
	}

	player_.SendToAgent(ptoS2C, PLAYER_S2C_RES_DeleteMail);
}

void CKnapsack::__GetMailList(const CMessage& msg)
{
	pto_PLAYER_S2C_RES_GetMailList pto;

	pto.set_res(0);

	LOCK_BLOCK(lock_);

	for (auto &it : mails_)
		it.second->SetProtocol(pto.add_mails());

	player_.SendToAgent(pto, PLAYER_S2C_RES_GetMailList);
}

void CKnapsack::__MakeEquip(const CMessage& msg)
{
	if (!player_.IsOpenGuide(12))
		return;

	pto_PLAYER_C2S_REQ_MakeEquip pto_c2s;
	pto_c2s.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_MakeEquip pto_s2c;
	pto_s2c.set_blueprint_id(pto_c2s.blueprint_id());
	pto_s2c.set_res(0);

	const CBlueprint* blueprint = CBlueprint::GetBlueprint(pto_c2s.blueprint_id());
	if (!blueprint)
		pto_s2c.set_res(2);
	else
	{
		CItem* item_blueprint = FindItem(pto_c2s.blueprint_id(), IL_KNAPSACK);
		if (!item_blueprint)
			pto_s2c.set_res(1);

		CItem* item_1 = FindItem(blueprint->material_1_id(), IL_KNAPSACK);
		if (!item_1 || item_1->quantity() < blueprint->material_1_num())
			pto_s2c.set_res(1);

		CItem* item_2 = FindItem(blueprint->material_2_id(), IL_KNAPSACK);
		if (!item_2 || item_2->quantity() < blueprint->material_2_num())
			pto_s2c.set_res(1);

	}

	player_.SendToAgent(pto_s2c, PLAYER_S2C_RES_MakeEquip);

	if (!pto_s2c.res())
	{
		MoveItem(false, blueprint->id(), 1, IL_KNAPSACK, IL_NULL);
		MoveItem(false, blueprint->material_1_id(), blueprint->material_1_num(), IL_KNAPSACK, IL_NULL);
		MoveItem(false, blueprint->material_2_id(), blueprint->material_2_num(), IL_KNAPSACK, IL_NULL);

		GiveNewEquip(blueprint->target_id(), blueprint->ProduceEquipRankColor());
	}
}

void CKnapsack::__ChooseLottery(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_ChooseLottery pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_ChooseLottery ptoRES;
	ptoRES.set_res(0);

	int nMaxChooseTimes = 2;
	if (CVIPFun::GetVIPFun(player_.vip_level(), VFT_ExLottery))
		nMaxChooseTimes = 3;

	if (m_nChooseLotteryTimes >= nMaxChooseTimes)
		ptoRES.set_res(2);

	else if (0 > pto.id() || 5 < pto.id())
		ptoRES.set_res(3);

	else if (m_arrTempLottery.at(pto.id()).m_bChoose)
		ptoRES.set_res(1);

	if (0 == ptoRES.res())
	{
		pto_PLAYER_STRUCT_Lottery* pPtoLottery = ptoRES.mutable_lottery_cell();
		if (LT_Item == m_arrTempLottery.at(pto.id()).m_enLotteryType)
			pPtoLottery->set_id(m_arrTempLottery[pto.id()].m_nID);
		else
			pPtoLottery->set_id(m_arrTempLottery[pto.id()].m_enLotteryType);
		pPtoLottery->set_num(m_arrTempLottery[pto.id()].m_nNum);

		if (LT_Item == m_arrTempLottery[pto.id()].m_enLotteryType)
		{
			GiveNewItem(pPtoLottery->id(), m_arrTempLottery[pto.id()].m_nNum, true);
		}
		else
		{
			switch (m_arrTempLottery[pto.id()].m_enLotteryType)
			{
			case LT_Silver:
				player_.ChangeSilver(pPtoLottery->num());
				break;
			case LT_Gold:
				player_.ChangeGold(pPtoLottery->num(),34);
				break;
			case LT_Honour:
				player_.ChangeHonor(pPtoLottery->num());
				break;
			case LT_Reputation:
				player_.ChangeReputation(pPtoLottery->num());
				break;
			case LT_Stamina:
				player_.ChangeStamina(pPtoLottery->num());
				break;
			case LT_Exp:
				player_.ChangeExp(pPtoLottery->num());
				break;
			default:
				break;
			}
		}
		m_arrTempLottery[pto.id()].m_bChoose = true;
		m_nChooseLotteryTimes++;
	}

	player_.SendToAgent(ptoRES, PLAYER_S2C_RES_ChooseLottery);
}

void CKnapsack::__StartLottery(int nID)
{
	__RestartLottery();

	const CLottery* pLottery = CLottery::GetLottery(nID);
	if (nullptr == pLottery)
		return;

	m_nChooseLotteryTimes = 0;
	__ProduceLotteryResult(pLottery);
	__SendLottery();
	__RandomLottery();
}

void CKnapsack::__ProduceLotteryResult(const CLottery* pLottery)
{
	for (size_t i = 0; i < 2; i++)
	{
		while (LT_Null == m_arrTempLottery[i].m_enLotteryType)
		{
			int nK = GetRandom(0, 5);

			m_arrTempLottery[i].m_enLotteryType = LT_Item;
			m_arrTempLottery[i].m_nID = pLottery->GetCertainLootID(nK);
			m_arrTempLottery[i].m_nNum = pLottery->GetCertainLootNum(nK);
			m_arrTempLottery[i].m_bChoose = false;
		}
	}

	int  chance_loot_num{ 0 };
	for (size_t i = 0; i < 8; i++)
	{
		if (GetRandom(0, 100) < (pLottery->GetChanceLootChance(i) * 100))
		{
			m_arrTempLottery[2 + chance_loot_num].m_enLotteryType = LT_Item;
			m_arrTempLottery[2 + chance_loot_num].m_nID = pLottery->GetChanceLootID(i);
			m_arrTempLottery[2 + chance_loot_num].m_nNum = pLottery->GetChanceLottNum(i);
			m_arrTempLottery[2 + chance_loot_num].m_bChoose = false;
			chance_loot_num++;
			if (chance_loot_num >= 4)
				break;
		}
	}

	if (chance_loot_num < 4)
	{
		int nNormalBegin = 2 + chance_loot_num;
		int nLotteryChanceSize = CLotteryChance::GetLotteryChanceSize();
		if (0 == nLotteryChanceSize)
			return;

		for (size_t i = nNormalBegin; i < 6; i++)
		{
			while (LT_Null == m_arrTempLottery[i].m_enLotteryType)
			{
				int nK = GetRandom(1, nLotteryChanceSize);
				const CLotteryChance* pLotteryChance = CLotteryChance::GetLottertChanceByIndex(nK);
				if (GetRandom(0, 100) < (pLotteryChance->GetChance() * 100))
				{
					m_arrTempLottery[i].m_enLotteryType = (LotteryType)pLotteryChance->GetType();
					m_arrTempLottery[i].m_nID = pLotteryChance->GetID();
					m_arrTempLottery[i].m_nNum = pLotteryChance->GetBaseNum() + (pLotteryChance->GetMul() * pLottery->GetLootLevel());
					m_arrTempLottery[i].m_bChoose = false;
				}
			}
		}
	}
}

void CKnapsack::__RestartLottery()
{
	for (size_t i = 0; i < 6; i++)
	{
		m_arrTempLottery[i].m_enLotteryType = LT_Null;
		m_arrTempLottery[i].m_nNum = 0;
		m_arrTempLottery[i].m_nID = 0;
		m_arrTempLottery[i].m_bChoose = true;
	}
	m_nChooseLotteryTimes = 10086;
}

void CKnapsack::__RandomLottery()
{
	std::array<TempLottery, 6> tempLottery;
	std::vector<int> vctTemp;
	for (size_t i = 0; i < 6; i++)
	{
		int nK = GetRandom(0, 5);
		while (__InTempVct(nK, &vctTemp))
		{
			nK = GetRandom(0, 5);
		}
		vctTemp.push_back(nK);
		tempLottery.at(i).m_enLotteryType = m_arrTempLottery[nK].m_enLotteryType;
		tempLottery.at(i).m_nID = m_arrTempLottery[nK].m_nID;
		tempLottery.at(i).m_nNum = m_arrTempLottery[nK].m_nNum;
	}

	for (size_t i = 0; i < 6; i++)
	{
		m_arrTempLottery[i].m_enLotteryType = tempLottery.at(i).m_enLotteryType;
		m_arrTempLottery[i].m_nID = tempLottery.at(i).m_nID;
		m_arrTempLottery[i].m_nNum = tempLottery.at(i).m_nNum;
	}
}

void CKnapsack::__SendLottery()
{
	pto_PLAYER_S2C_NTF_OpenLottery pto;
	for (size_t i = 0; i < 6; i++)
	{
		pto_PLAYER_STRUCT_Lottery* pPtoLottery = pto.add_lottery_cell();
		if (LT_Item == m_arrTempLottery[i].m_enLotteryType)
			pPtoLottery->set_id(m_arrTempLottery[i].m_nID);
		else
			pPtoLottery->set_id(m_arrTempLottery[i].m_enLotteryType);
		pPtoLottery->set_num(m_arrTempLottery[i].m_nNum);
	}

	player_.SendToAgent(pto, PLAYER_S2C_NTF_OpenLottery);
}

bool CKnapsack::__InTempVct(int nNum, std::vector<int>* pVct)
{
	for (size_t i = 0; i < pVct->size(); i++)
		if (nNum == pVct->at(i))
			return true;

	return false;
}

void CKnapsack::__CloseLottery()
{
	__RestartLottery();
}

void CKnapsack::__ActivateEquipEnchanting(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_ActivateEquipEnchanting pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_ActivateEquipEnchanting pto_res;
	pto_res.set_res(0);
	pto_res.set_unique_id(pto_req.unique_id());

	CEquip* equip = FindEquip(pto_req.unique_id());
	if (!equip)
		pto_res.set_res(3);
	else if (player_.gold() < 100)
		pto_res.set_res(2);
	else if (equip->enchanting2_is_active())
		pto_res.set_res(1);

	if (!pto_res.res())
	{
		player_.ChangeGold(-100,35);
		equip->SetEnchanting2IsActive(true);
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_ActivateEquipEnchanting);
}

void CKnapsack::__Enchanting(const CMessage& msg)
{
	if (!player_.IsOpenGuide(42))
		return;

	pto_PLAYER_C2S_REQ_Enchanting pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_Enchanting pto_res;
	pto_res.set_res(0);
	pto_res.set_unique_id(pto_req.unique_id());

	int gold{ 0 };
	int stone_id{ 0 };
	CEquip* equip = FindEquip(pto_req.unique_id());

	if (!equip)
	{
		pto_res.set_res(3);
	}
	else if ((pto_req.enchanting_1_lock() && pto_req.enchanting_2_lock()) || (pto_req.enchanting_1_lock() && !equip->enchanting2_is_active()))
	{
		pto_res.set_res(2);
	}
	else
	{
		const CEnchantingCost* cost = CEnchantingCost::GetEnchantingCost(equip->equip_template()->stage());
		if (!cost)
		{
			pto_res.set_res(2);
		}
		else
		{
			int silver = cost->silver();
			int stone_id = cost->enchanting_stone_id();
			int gold{ 0 };

			if (pto_req.enchanting_1_lock() || pto_req.enchanting_2_lock())
				gold = 80;

			if (player_.silver() < silver)
				pto_res.set_res(1);
			if (player_.gold() < gold)
				pto_res.set_res(1);

			CItem* stone = FindItem(stone_id, IL_KNAPSACK);
			if (!stone)
				pto_res.set_res(1);

			if (!pto_res.res())
			{
				player_.ChangeSilver(-silver);
				player_.ChangeGold(-gold,36);
				MoveItem(false, stone_id, 1, IL_KNAPSACK, IL_NULL);
				player_.knapsack()->SendUseItem(stone_id, -1);

				player_.ImplementMission(EMissionTargetType::MTT_Enchanting, 0, 0);

				int		type{ 0 };
				float	value{ 0 };
				if (!pto_req.enchanting_1_lock())
				{
					__Enchanting(type, value, equip);
					equip->SetEnchanting1(type, value);
				}

				if (!pto_req.enchanting_2_lock()
					&& equip->enchanting2_is_active())
				{
					__Enchanting(type, value, equip);
					equip->SetEnchanting2(type, value);
				}
				pto_res.set_enchant(equip->enchanting1());
				pto_res.set_enchant_value(equip->enchanting1_value());
				pto_res.set_enchant2(equip->enchanting2());
				pto_res.set_enchant2_value(equip->enchanting2_value());
			}
		}
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_Enchanting);
}

void CKnapsack::__Enchanting(int& type, float& value, CEquip* equip)
{
	type = CEnchanting::RandomEnchantingID();
	const CEnchanting* enchanting = CEnchanting::GetEnchanting(type);
	if (enchanting)
	{
		float min = enchanting->min_base() + (enchanting->min_parameter() * (equip->equip_template()->stage() - 1));
		float max = enchanting->max_base() + (enchanting->max_parameter() * (equip->equip_template()->stage() - 1)) + (enchanting->quality_parameter() * (equip->quality() - 1));
		if (type <= 5)
			value = (float)GetRandom((int)min, (int)max);
		else
			value = (float)GetRandom((int)(min * 100), (int)(max * 100)) / 100;
	}
}

void CKnapsack::__ExchangeFashion(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_ExchangeFashion pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	int fashion_id = pto.fashion_id();

	int result{ 0 };

	if (0 == fashion_id)
	{
		body_fashion_ = nullptr;
	}
	else if (body_fashion_ && body_fashion_->id == fashion_id)
	{
		result = 3;
	}
	else
	{
		auto it = fashions_.find(fashion_id);

		if (fashions_.cend() == it)
		{
			result = 2;
		}
		else if (-1 != it->second->due_time &&  it->second->due_time <= time(0))
		{
			result = 1;
		}
		else
		{
			body_fashion_ = it->second;
			player_.ImplementMission(EMissionTargetType::MTT_ExchangeFashion, 0, 0);
		}
	}

	pto_PLAYER_S2C_RES_ExchangeFashion pto_res;
	pto_res.set_result(result);

	if (body_fashion_)
		pto_res.set_fashion_id(body_fashion_->id);
	else
		pto_res.set_fashion_id(0);

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_ExchangeFashion);

	if (0 == result)
	{
		CTown* town = GAME_WORLD()->FindTown(player_.town_id());

		if (town)
		{
			pto_TOWN_S2C_NTF_ChangeModule pto_change;
			pto_change.set_pid(player_.pid());
			pto_change.set_moduleid(clothes_id());
			std::string str;
			pto_change.SerializeToString(&str);
			town->SendAllPlayersInTown(str, MSG_S2C, TOWN_S2C_NTF_ChangeModule, 0);
		}
	}
}

void CKnapsack::__BuyFashion(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_BuyFashion pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	int		fashion_id{ pto.fashion_id() };
	time_t	add_time{ 24 * 60 * 60 };

	int result{ 0 };

	const CFashionTP* fashion_template = CFashionTP::GetFashionTP(fashion_id);

	CFashion*	fashion{ nullptr };
	int			need_gold{ 0 };

	if (fashion_template)
	{
		switch (pto.buy_type())
		{
		case 1:
			add_time *= 7;
			need_gold = fashion_template->price();
			break;
		case 2:
			add_time *= 30;
			need_gold = fashion_template->price() * 3;
			break;
		case 3:
			add_time *= 90;
			need_gold = fashion_template->price() * 6;
			break;
		case 4:
			add_time = -1;
			need_gold = fashion_template->price() * 15;
			break;
		default:
			return;
		}

		if (false == fashion_template->renew())
		{
			result = 3;
		}
		else if (need_gold > player_.gold())
		{
			result = 2;
		}
		else
		{
			auto it = fashions_.find(fashion_id);

			if (fashions_.cend() == it)
			{
				fashion = new CFashion;
				fashion->id = fashion_id;
				fashion->get_time = time(0);

				if (-1 == add_time)
					fashion->due_time = -1;
				else
					fashion->due_time = time(0) + add_time;

				fashions_[fashion_id] = fashion;
			}
			else
			{
				fashion = it->second;

				if (-1 == fashion->due_time)
					return;

				if (-1 == add_time)
					fashion->due_time = -1;
				else
					fashion->due_time += add_time;
			}

			player_.ImplementMission(MTT_Shopping, 0, 0);
			player_.ChangeGold(-need_gold,37);
		}
	}
	else
	{
		result = 1;
	}

	pto_PLAYER_S2C_RES_BuyFashion pto_res;
	pto_res.set_result(result);
	if (0 == result)
	{
		auto pto_fashion = pto_res.mutable_fashion();
		pto_fashion->set_fashion_id(fashion->id);
		pto_fashion->set_due_time(fashion->due_time);
		pto_fashion->set_is_on_body(fashion == body_fashion_);
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_BuyFashion);
}

void CKnapsack::__RecastEquip(const CMessage& msg)
{
	if (!player_.IsOpenGuide(42))
		return;

	pto_PLAYER_C2S_REQ_RecastEquip pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_RecastEquip pto_res;
	pto_res.set_res(0);
	pto_res.set_unique_id(pto_req.uniqueid());

	int stone_num{ 0 };
	CEquip* equip = FindEquip(pto_req.uniqueid());
	CItem* stone = FindItem(91001, ItemLocation::IL_KNAPSACK);

	if (!equip)
		pto_res.set_res(3);
	else
	{
		stone_num = CRecastCost::GetStoneNum(equip->quality());
		if (!stone)
			pto_res.set_res(1);
		else if (stone->quantity() < stone_num)
			pto_res.set_res(1);
	}

	if (!pto_res.res())
	{
		equip->SetSuitID(equip->equip_template_->ProduceSuitID(equip->suit_id()));
		MoveItem(false, 91001, stone_num, ItemLocation::IL_KNAPSACK, ItemLocation::IL_NULL);
		SendUseItem(91001, -stone_num);
		pto_res.set_suit_id(equip->suit_id());
	}
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_RecastEquip);
}

void CKnapsack::__ResolveEquip(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_ResolveEquip pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_ResolveEquip pto_res;
	pto_res.set_res(0);
	pto_res.set_unique_id(pto_req.unique_id());

	CEquip* equip = FindEquip(pto_req.unique_id());
	if (!equip)
		pto_res.set_res(2);
	else
	{
		const CResolveTP* reslove_tp = CResolveTP::GetResolveTP(equip->quality());
		if (!reslove_tp)
			pto_res.set_res(1);

		if (!pto_res.res())
		{
			const CEnchantingCost* cost = CEnchantingCost::GetEnchantingCost(equip->equip_template()->stage());
			if (cost)
			{
				int item_id = cost->enchanting_stone_id();
				int item_num = GetRandom(reslove_tp->enchant_min(), reslove_tp->enchant_max());
				GiveNewItem(item_id, item_num, true);
				pto_res.add_item_id(item_id);
				pto_res.add_item_num(item_num);
			}

			int item_id = 91001;
			int item_num = GetRandom(reslove_tp->catalyze_min(), reslove_tp->catalyze_max());
			GiveNewItem(item_id, item_num, true);
			pto_res.add_item_id(item_id);
			pto_res.add_item_num(item_num);
			MoveItem(true, pto_req.unique_id(), 1, ItemLocation::IL_KNAPSACK, ItemLocation::IL_NULL);
		}
	}
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_ResolveEquip);
}

void CKnapsack::__GetFashion(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_GetFashion pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_GetFashion pto_res;
	pto_res.set_res(0);
	pto_res.set_fashion_id(pto_req.fashion_id());

	bool in_mission = false;
	for (auto &it : *player_.mission()->GetAcceptMission())
	{
		const CMissionTP* mission = CMissionTP::GetMission(it.first);
		if (mission &&
			mission->GetType() == 1 &&
			mission->GetTargetType() == EMissionTargetType::MTT_ExchangeFashion)
			in_mission = true;
	}

	if (!in_mission)
		pto_res.set_res(1);
	auto it = fashions_.find(pto_req.fashion_id());
	if (it != fashions_.cend())
	{
		if ((float)(it->second->due_time - time(0)) / (24 * 60 * 60) > 7)
			pto_res.set_res(1);
	}

	if (!pto_res.res())
	{
		CFashion* fashion{ nullptr };
		auto it = fashions_.find(pto_req.fashion_id());
		if (fashions_.cend() == it)
		{
			fashion = new CFashion;
			fashion->id = pto_req.fashion_id();
			fashion->get_time = time(0);

			fashion->due_time = time(0) + (24 * 60 * 60 * 7);

			fashions_[pto_req.fashion_id()] = fashion;
		}
		else
		{
			fashion = it->second;
			fashion->due_time += (24 * 60 * 60 * 7);
		}

		pto_PLAYER_S2C_RES_BuyFashion pto_res;
		pto_res.set_result(0);

		auto pto_fashion = pto_res.mutable_fashion();
		pto_fashion->set_fashion_id(fashion->id);
		pto_fashion->set_due_time(fashion->due_time);
		pto_fashion->set_is_on_body(fashion == body_fashion_);

		player_.SendToAgent(pto_res, PLAYER_S2C_RES_BuyFashion);
	}
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_GetFashion);

}

void CKnapsack::__BatchSell(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_BatchSell pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_BatchSell pto_res;

	for (auto it : pto_req.sell())
	{
		ItemLocation location{ static_cast<ItemLocation>(it.bagtype()) };

		__int64 price{ 0 };

		int result{ 0 };

		if (0 == it.itemtype())
		{
			CItem* item{ FindItem(it.id(), location) };

			if (nullptr == item)
				result = 2;
			else if (it.num() < 1 || it.num() > item->quantity())
				result = 3;
			else if (item->item_template_->GetSellPrice() <= 0)
				result = 4;

			if (0 == result)
			{
				price = item->item_template_->GetSellPrice() * it.num();

				if (item->ChangeQuantity(-it.num()) <= 0)
					DeleteItem(it.id(), location);
			}
		}
		else
		{
			LOCK_BLOCK(lock_);

			for (auto &itEquip : equips_)
			{
				if (itEquip.first && it.id() == itEquip.first->unique_id_)
				{
					price = itEquip.first->equip_template_->sell_price() * (itEquip.first->quality() + 1);

					for (int i = 1; i <= itEquip.first->level(); i++)
						price += __int64(CEquipTP::GetImproveEquipPrice(i, itEquip.first->equip_template_->type()) * 0.8f);

					delete itEquip.first;
					equips_.erase(itEquip.first);

					break;
				}
			}
		}

		if (price > 0)
			player_.ChangeSilver(price);

		pto_ITEM_S2C_RES_SELL* sell = pto_res.add_sell();
		sell->set_res(result);
		sell->set_bagtype(it.bagtype());
		sell->set_id(it.id());
		sell->set_itemtype(it.itemtype());
		sell->set_num(it.num());
	}
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_BatchSell);
}

void CKnapsack::__BuyInMall(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_BuyInMall pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYEER_S2C_RES_BuyInMall pto_res;
	pto_res.set_id(pto_req.id());
	pto_res.set_res(0);
	pto_res.set_num(pto_req.num());

	int buy_num = pto_req.num();
	int use_currency_num = 0;
	const CMall* mall = CMall::GetMall(pto_req.id());
	if (!mall)
		pto_res.set_res(1);
	else if (buy_num < 1)
		pto_res.set_res(1);
	else
	{
		float discount = CMall::GetDiscount(pto_req.id());
		if (discount == 1 && player_.vip_level() >= 5 && mall->page_type() != 3) discount = 0.8f;
		use_currency_num = int(mall->currency_num() * buy_num * discount);

		if (mall->currency_type() == 1)	//道具
		{
			CItem* currency_item = FindItem(mall->currency_parameter(), IL_KNAPSACK);
			if (!currency_item || currency_item->quantity() < use_currency_num)
				pto_res.set_res(2);
		}
		if (mall->currency_type() == 2 && player_.gold() < use_currency_num)	//金币
		{
			pto_res.set_res(2);
		}
		if (mall->currency_type() == 3 && player_.silver() < use_currency_num)	//银币
		{
			pto_res.set_res(2);
		}
	}

	if (!pto_res.res())
	{
		if (mall->currency_type() == 1)	//道具
		{
			MoveItem(false, mall->currency_parameter(), use_currency_num, IL_KNAPSACK, IL_NULL);
			player_.knapsack()->SendUseItem(mall->currency_parameter(), -use_currency_num);
		}

		if (mall->currency_type() == 2)	//金币
			player_.ChangeGold(-use_currency_num,38);
		if (mall->currency_type() == 3)	//银币
			player_.ChangeSilver(-use_currency_num);

		if (mall->page_type() == 4)
		{
			GiveNewEquip(mall->item_id(), mall->num() - 1);
		}
		else
			GiveNewItem(mall->item_id(), mall->num() * buy_num, true);
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_BuyInMall);
}

void CKnapsack::__Inherit(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_Inherit pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_Inherit pto_res;
	pto_res.set_res(0);

	CEquip* source_equip = FindEquip(pto_req.source_uniqueid());
	CEquip* target_equip = FindEquip(pto_req.target_uniqueid());

	if (!source_equip ||
		!target_equip)
		pto_res.set_res(1);

	else if (source_equip->equip_template()->type() != target_equip->equip_template()->type())
		pto_res.set_res(2);

	else if (source_equip->level() < target_equip->level() ||
		source_equip->level() < 10)
		pto_res.set_res(3);

	if (!pto_res.res())
	{
		target_equip->SetLevel((int)(0.8f * source_equip->level()));
		source_equip->SetLevel(0);
		pto_res.set_source_uniqueid(source_equip->unique_id());
		pto_res.set_source_level(source_equip->level());
		pto_res.set_target_uniqueid(target_equip->unique_id());
		pto_res.set_target_level(target_equip->level());
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_Inherit);
}

void CKnapsack::__LoadFromDatabase()
{
	//道具
	std::ostringstream sql;
	sql << "select * from tb_player_item where pid =" << player_.pid();
	ResultSetPtr result_item{ MYSQL_QUERY(sql.str()) };

	if (result_item)
	{
		while (result_item->next())
		{
			time_t	get_time{ result_item->getInt64("time") };
			bool	is_temp{ result_item->getBoolean("is_temp") };

			//7天不放入背包道具消失
			int duration_day{ static_cast<int>(GetDurationCount(time(0), get_time, DURATION_TYPE::DAY)) };
			if (is_temp && (duration_day >= 7))
				continue;

			CItem* item{ item_pool_.construct(result_item->getInt("id")) };
			if (0 == item->GetID())
			{
				item_pool_.destroy(item);
				continue;
			}

			item->quantity_ = result_item->getInt("quantity");
			item->get_time_ = get_time;

			if (is_temp)
				temporary_items_[item->GetID()] = item;
			else
				items_[item->GetID()] = item;
		}
	}

	//装备
	sql.str("");
	sql << "select * from tb_player_equip where pid =" << player_.pid();
	ResultSetPtr result_equip{ MYSQL_QUERY(sql.str()) };
	if (result_equip)
	{
		while (result_equip->next())
		{
			time_t	get_time{ result_equip->getInt64("time") };
			int		location{ result_equip->getInt("loaction") };

			int duration_day{ static_cast<int>(GetDurationCount(time(0), get_time, DURATION_TYPE::DAY)) };
			if (IL_STROEROOM == location && (duration_day >= 7))
				continue;

			CEquip* equip{ new CEquip{ result_equip->getInt("id") } };
			if (0 == equip->GetID())
			{
				delete equip;
				continue;
			}

			equip->unique_id_ = equip_unique_id_allcoator_++;
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

	//邮件
	sql.str("");
	sql << "select * from tb_player_mail where pid = " << player_.pid();
	ResultSetPtr result_mail{ MYSQL_QUERY(sql.str()) };

	if (result_mail)
	{
		while (result_mail->next())
		{
			SPMail mail{ new CMail };
			mail->model_id_ = result_mail->getInt("model_id");
			mail->param0_ = result_mail->getInt("param0");
			mail->param1_ = result_mail->getInt("param1");
			mail->name_ = result_mail->getString("name").c_str();
			mail->recv_time_ = result_mail->getInt64("time");
			mail->is_read_ = result_mail->getBoolean("is_read");
			mail->reward_hero_id_ = result_mail->getInt("reward_hero");
			mail->reward_soldier_id_ = result_mail->getInt("reward_soldier");
			mail->reward_title_ = result_mail->getInt("reward_title");

			auto it_pairs = CSerializer<__int64>::ParsePairFromString(result_mail->getString("reward_items").c_str());
			for (auto &it : it_pairs)
				mail->reward_items_.push_back(std::make_pair(it.first, it.second));

			auto it_vct = CSerializer<int>::ParseFromString(result_mail->getString("reward_equips").c_str());
			for (auto &it : it_vct)
				mail->reward_equips_.push_back(it);

			AddMail(mail, false);
		}
	}

	//时装
	sql.str("");
	sql << "select * from tb_player_fashion where pid = " << player_.pid();
	ResultSetPtr result_fashion{ MYSQL_QUERY(sql.str()) };
	if (result_fashion)
	{
		while (result_fashion->next())
		{
			CFashion* fashion = new CFashion;
			fashion->id = result_fashion->getInt("id");
			fashion->get_time = result_fashion->getInt64("get_time");
			fashion->due_time = result_fashion->getInt64("due_time");

			if (result_fashion->getBoolean("is_on_body"))
				body_fashion_ = fashion;

			if (false == fashions_.insert(std::make_pair(fashion->id, fashion)).second)
			{
				delete fashion;
				RECORD_WARNING("时装加载问题");
			}
		}
	}
}