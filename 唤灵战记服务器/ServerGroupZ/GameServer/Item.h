#pragma once

class CItem
{
	friend class CKnapsack;
public:
	CItem(int item_id);
	virtual ~CItem();

	int				GetID() const;								//���ģ��ID

	inline int		quantity() const { return quantity_; }
	inline time_t	get_time() const { return get_time_; }
	inline void		get_time(time_t time){ get_time_ = time; }

	inline int		cell_max_num() const { return item_template_->GetMaxNum(); }

	int				ChangeQuantity(int num);

private:
	const CItemTP* const	item_template_;							//��Ʒģ��
	int						quantity_{ 0 };							//����
	time_t					get_time_{ 0 };							//���ʱ��
};
