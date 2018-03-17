#pragma once

enum class EquipPart
{
	kUnknow = -1,	//未知
	kWeapon,		//武器
	kHands,			//手套
	kChest,			//胸甲
	kLegs,			//腿甲
	kHead,			//头盔
	kFeet,			//鞋子
	kEquipCount,
};

struct ImproveEquipPrice
{
	int level{ 0 };
	__int64 weapon{ 0 };
	__int64 hands{ 0 };
	__int64 chest{ 0 };
	__int64 legs{ 0 };
	__int64 head{ 0 };
	__int64 feet{ 0 };
};

inline bool IsEquip(EquipPart part){ return part > EquipPart::kUnknow && part <= EquipPart::kFeet; }

class CEquipTP
{
public:
	static void		Load();
	static const	CEquipTP* GetEquipTP(int id);
	static const	CEquipTP* GetEquipTP(int stage, EquipPart type);
	static __int64	GetImproveEquipPrice(int nLv, EquipPart type);
	static int		GetEquipAttribute(int level, EquipPart type);

private:
	static std::map<int, const CEquipTP*>				new_equip_tp_;
	static std::map<int, const ImproveEquipPrice*>		improve_equip_price;			//升级装备价格表(nLevel,价格)

public:
	CEquipTP();
	~CEquipTP();
	inline int				id() const { return id_; }
	inline int				suit_id() const { return suit_id_; }
	inline EquipPart		type() const { return type_; }
	inline int				use_level() const { return use_level_; }
	inline int				stage() const { return stage_; }
	inline int				attribute_type() const { return attribute_type_; }
	inline float			attribute() const { return attribute_; }
	inline int				jewel_1() const { return jewel_1_; }
	inline int				jewel_2() const { return jewel_2_; }
	inline int				jewel_3() const { return jewel_3_; }
	inline int				jewel_4() const { return jewel_4_; }
	inline int				price() const { return price_; }
	inline int				sell_price() const { return sell_price_; }
	int						ProduceSuitID(int except_id) const;

private:
	int						id_{ 0 };							//ID
	int						suit_id_{ 0 };						//套装分组
	EquipPart				type_{ EquipPart::kUnknow };		//部位
	int						use_level_{ 0 };					//可用等级
	int						stage_{ 0 };						//阶级
	int						attribute_type_{ 0 };				//增加属性类型
	float					attribute_{ 0 };					//装备基础值
	int						jewel_1_{ 0 };						//宝石1
	int						jewel_2_{ 0 };						//宝石2
	int						jewel_3_{ 0 };						//宝石3
	int						jewel_4_{ 0 };						//宝石4
	std::vector<int>		possible_suit_;						//可能出现的套装属性ID
	int						price_{ 0 };
	int						sell_price_{ 0 };
};

