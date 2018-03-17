#include "stdafx.h"
#include "Item.h"
#include "GameServer.h"

CItem::CItem(int item_id) :
item_template_{ CItemTP::GetItemTP(item_id) }
{
	if (!item_template_)
		RECORD_WARNING("ÎïÆ·ID³ö´í");
}

CItem::~CItem()
{

}

int	CItem::GetID() const
{
	if (item_template_)
		return item_template_->GetItemID();
	else
		return 0;
}

int	CItem::ChangeQuantity(int num)
{
	get_time_ = time(0);
	return quantity_ += num;
}