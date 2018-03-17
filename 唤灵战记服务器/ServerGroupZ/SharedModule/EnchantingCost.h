#pragma once
class CEnchantingCost
{
public:
	CEnchantingCost();
	~CEnchantingCost();
	static void Load();
	static const CEnchantingCost* GetEnchantingCost(int stage);

	int enchanting_stone_id() const { return enchanting_stone_id_; }
	int silver() const { return silver_; }

private:
	static std::map<int, const CEnchantingCost*> enchanting_cost_lirary_;
	int equip_stage_{ 0 };
	int enchanting_stone_id_{ 0 };
	int silver_{ 0 };
};

