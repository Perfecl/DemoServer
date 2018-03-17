#pragma once

enum EnchantingType
{
	kEnchantingTypeNull = 0,
	kEnchantingTypeAtk,
	kEnchantingTypeMAtk,
	kEnchantingTypeDef,
	kEnchantingTypeMDef,
	kEnchantingTypeHP,
	kEnchantingTypeAtkSpeed,
	kEnchantingTypeCritOdds,
	kEnchantingTypePreventCrit,
	kEnchantingTypeTime,
};
class CEnchanting
{
public:
	CEnchanting();
	~CEnchanting();
	static void Load();
	static int RandomEnchantingID();
	static const CEnchanting* GetEnchanting(int id);

	float min_base() const { return min_base_; }
	float min_parameter() const { return min_parameter_; }
	float max_base() const { return max_base_; }
	float max_parameter() const { return max_parameter_; }
	float quality_parameter() const { return quality_parameter_; }

private:
	static std::map<int, const CEnchanting*> enchanting_library_;
	static std::vector<int> enchanting_id_;
	int id_{ 0 };
	float min_base_{ 0 };
	float min_parameter_{ 0 };
	float max_base_{ 0 };
	float max_parameter_{ 0 };
	float quality_parameter_{ 0 };
};

