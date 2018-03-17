#pragma once

class CItem
{
	friend class CKnapsack;
public:
	CItem(int item_id);
	virtual ~CItem();

	int				GetID() const;								//获得模板ID

	inline int		quantity() const { return quantity_; }
	inline time_t	get_time() const { return get_time_; }
	inline void		get_time(time_t time){ get_time_ = time; }

	inline int		cell_max_num() const { return item_template_->GetMaxNum(); }

	int				ChangeQuantity(int num);

private:
	const CItemTP* const	item_template_;							//物品模版
	int						quantity_{ 0 };							//数量
	time_t					get_time_{ 0 };							//获得时间
};
