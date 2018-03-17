#pragma once

enum SuitAttributrType
{
	kSuitNull = 0,
	kSuitAtk,			//����
	kSuitDef,			//����
	kSuitHP,			//Ѫ
	kSuitMoveSpeed,		//�ƶ��ٶ�
	kSuitCritOdds,		//������
	kSuitPreventCrit,	//�ⱬ
	kSuitStr,			//����
	kSuitCmd,			//ͳ˧
	kSuitInt,			//����
	kSuitAtkSpeed,		//�����ٶ�
	kSuitMaxTime,		//����ʱ��
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
	SuitAttributrType attribute_type_2_{ kSuitNull };	//2��Ч������
	float attribute_2_{ 0 };							//2���׻�������
	float stage_2_{ 0 };								//2���׽׼�����
	SuitAttributrType attribute_type_4_{ kSuitNull };	//4��Ч������
	float attribute_4_{ 0 };							//4���׻�������
	float stage_4_{ 0 };								//4���׽׼�����
	int skill_id_{ 0 };									//6���׼���ID
};
