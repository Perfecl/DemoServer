#pragma once

enum SuitAttributrType
{
	kSuitNull = 0,
	kSuitAtk,			//攻击
	kSuitDef,			//防御
	kSuitHP,			//血
	kSuitMoveSpeed,		//移动速度
	kSuitCritOdds,		//暴击率
	kSuitPreventCrit,	//免爆
	kSuitStr,			//力量
	kSuitCmd,			//统帅
	kSuitInt,			//智力
	kSuitAtkSpeed,		//攻击速度
	kSuitMaxTime,		//存在时间
};

struct SuitAddition
{
	int atk_{ 0 };
	int def_{ 0 };
	int	hp_{ 0 };
	float move_speed_{ 0 };
	float crit_odds_{ 0 };
	float prevent_crit_{ 0 };
	int str_{ 0 };
	int cmd_{ 0 };
	int int_{ 0 };
	float atk_speed_{ 0 };
	int max_time_{ 0 };
	int skill_id{ 0 };
};

class CSuitTP
{
public:
	static void Load();
	static const CSuitTP* GetSuitTP(int id);

private:
	static std::map<int, const CSuitTP*> suit_tp_;

public:
	CSuitTP();
	~CSuitTP();
	SuitAttributrType attribute_type(int suit_num) const;
	float attribute(int suit_num) const;
	float stage(int suit_num) const;
	int	skill_id() const { return skill_id_; }
private:
	int id_{ 0 };
	SuitAttributrType attribute_type_2_{ kSuitNull };	//2件效果类型
	float attribute_2_{ 0 };							//2件套基础参数
	float stage_2_{ 0 };								//2件套阶级参数
	SuitAttributrType attribute_type_4_{ kSuitNull };	//4件效果类型
	float attribute_4_{ 0 };							//4件套基础参数
	float stage_4_{ 0 };								//4件套阶级参数
	int skill_id_{ 0 };									//6件套技能ID
};
