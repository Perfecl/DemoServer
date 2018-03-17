#pragma once
#include "EquipTP.h"

enum AdditionType
{
	kAdditionTypeNull = -1,

	kAdditionTypeWeapon,
	kAdditionTypeHands,
	kAdditionTypeChest,
	kAdditionTypeLegs,
	kAdditionTypeHead,
	kAdditionTypeFeet,

	kAdditionTypeUnitAtk,
	kAdditionTypeUnitMAtk,
	kAdditionTypeUnitDef,
	kAdditionTypeUnitMDef,
	kAdditionTypeUnitHP,
};

class CAddition
{
public:
	static void Load();
	static const CAddition* GetAddition(int stage);
	static const float GetAddition(int stage, AdditionType type);

	static int  GetHeroEquipAddition(int level, const CEquipTP* equip, int quality);	//��ҵ���װ���ӳ� 
	static int	GetSoldierAddition(int level, const AdditionType type);	//ʿ���Ƽ��ӳ�
	static int  GetCustomMasterEquipAtk(int level);	//�Զ���MASTERӢ�۹�����ħ���ӳ�
	static int  GetCustomMasterEquipDef(int level);	//�Զ���MASTERӢ�۷�����ħ���ӳ�
	static int  GetCustomMasterEquipHP(int level);	//�Զ���MASTERӢ��Ѫ���ӳ�

	static int fair_equip_atk() { return fair_equip_atk_;}
	static int fair_equip_def() { return fair_equip_def_; }
	static int fair_equip_hp() { return fair_equip_hp_; }
	static int fair_soldier_atk() { return fair_soldier_atk_; }
	static int fair_soldier_matk() { return fair_soldier_matk_; }
	static int fair_soldier_def() { return fair_soldier_def_; }
	static int fair_soldier_mdef() { return fair_soldier_mdef_; }
	static int fair_soldier_hp() { return fair_soldier_hp_; }

private:
	static std::map<int, const CAddition*> addition_library_;

	static int fair_equip_atk_;
	static int fair_equip_def_;
	static int fair_equip_hp_;
	static int fair_soldier_atk_;
	static int fair_soldier_matk_;
	static int fair_soldier_def_;
	static int fair_soldier_mdef_;
	static int fair_soldier_hp_;

public:
	CAddition();
	~CAddition();

	float atk() const { return atk_; }
	float m_atk() const { return m_atk_; }
	float def() const { return def_; }
	float m_def() const { return m_def_; }
	float hp() const { return hp_; }

	float weapon() const { return weapon_; }
	float hands() const { return hands_; }
	float chest() const { return chest_; }
	float legs() const { return legs_; }
	float head() const { return head_; }
	float feet() const { return feet_; }

	float GetTypeAddtion(AdditionType type) const;

private:
	int stage_{ 0 };

	float atk_{ 0 };
	float m_atk_{ 0 };
	float def_{ 0 };
	float m_def_{ 0 };
	float hp_{ 0 };

	float weapon_{ 0 };	//����
	float hands_{ 0 };	//����
	float chest_{ 0 };	//�ؼ�
	float legs_{ 0 };	//�ȼ�
	float head_{ 0 };	//ͷ��
	float feet_{ 0 };	//Ь��
};

