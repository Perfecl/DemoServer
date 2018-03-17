#pragma once

class CBlueprint
{
public:
	static void Load();
	static const CBlueprint* GetBlueprint(int id);

private:
	static std::map<int, const CBlueprint*> blueprint_library_;
	

public:
	CBlueprint();
	~CBlueprint();

	EquipRankColor ProduceEquipRankColor() const;
	int	id()				const { return id_; }
	int target_id()			const { return target_id_; }
	int material_1_id()		const { return material_1_id_; }
	int material_1_num()	const { return material_1_num_; }
	int material_2_id()		const{ return material_2_id_; }
	int material_2_num()	const { return material_2_num_; }
	
private:
	int						id_{ 0 };
	int						target_id_{ 0 };
	int						material_1_id_{ 0 };
	int						material_1_num_{ 0 };
	int						material_2_id_{ 0 };
	int						material_2_num_{ 0 };
	std::array<float, 5>	rank_odds_;
};

