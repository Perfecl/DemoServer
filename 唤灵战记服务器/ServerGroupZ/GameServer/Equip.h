#pragma once
#include "Item.h"

class CEquip
{
	friend class CKnapsack;
	friend class CPlayerCache;
public:
	CEquip(int equip_id);
	~CEquip();

	int							GetID() const;											//���ģ��ID
	void						SetProtocol(Equip_Model* pto);							//����Э��
	inline time_t				get_time() const { return get_time_; }
	inline void					get_time(time_t time){ get_time_ = time; }
	inline int					level() const { return level_; }
	inline int					level_up() { return ++level_; }
	inline int					unique_id() const { return unique_id_; }
	const CEquipTP*				equip_template() const { return equip_template_; }

	inline std::array<int, 4>	GetGems() const { return gems_; }
	inline int					quality() const { return quality_; }
	inline int					suit_id() const { return suit_id_; }
	inline int					enchanting1() const { return enchanting1_.first; }
	inline float				enchanting1_value() const { return enchanting1_.second; }
	inline int					enchanting2() const { return enchanting2_.first; }
	inline float				enchanting2_value() const { return enchanting2_.second; }
	inline bool					enchanting2_is_active() const { return enchanting2_is_active_; }

	inline void					SetLevel(int num){ level_ = num; }
	inline void					SetSuitID(int num){ suit_id_ = num; }
	inline void					SetQuality(int num){ quality_ = num; }
	inline void					SetGems(int index, int num){ gems_[index] = num; }
	inline void					SetEnchanting1(int type, float value){ enchanting1_.first = type; enchanting1_.second = value; }
	inline void					SetEnchanting2(int type, float value){ enchanting2_.first = type; enchanting2_.second = value; }
	inline void					SetEnchanting2IsActive(bool is_active){ enchanting2_is_active_ = is_active; }

private:
	const CEquipTP* const	equip_template_;								//��Ʒģ��
	int						unique_id_{ 0 };								//ΨһID
	int						level_{ 0 };									//�ȼ�

	int						suit_id_{ 0 };									//��װID

	int						quality_{ 0 };									//Ʒ��

	std::array<int, 4>		gems_;											//��ʯ��

	std::pair<int, float>	enchanting1_;									//��ħ1	(����,��ֵ)
	std::pair<int, float>	enchanting2_;									//��ħ2 (����,��ֵ)
	bool					enchanting2_is_active_{ false };				//��ħ2�Ƿ񼤻�

	time_t					get_time_{ 0 };									//���ʱ��
};
