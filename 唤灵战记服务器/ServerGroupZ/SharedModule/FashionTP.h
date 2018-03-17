#pragma once

class CFashionTP
{
public:
	static void Load();
	static const CFashionTP* GetFashionTP(int id);

private:
	static std::map<int, const CFashionTP*>  fashion_tp_;

public:
	CFashionTP();
	~CFashionTP();
	int id() const { return id_; }
	int type() const { return type_; }			
	bool renew() const { return renew_; }	
	int price() const { return price_; }		
	int hero_atk() const { return hero_atk_; }
	int hero_matk() const { return hero_matk_; }
	int hero_def() const { return hero_def_; }
	int hero_mdef() const { return hero_mdef_; }
	int hero_hp() const { return hero_hp_; }
	int unit_atk() const { return unit_atk_; }
	int unit_matk() const { return unit_matk_; }
	int unit_def() const { return unit_def_; }
	int unit_mdef() const { return unit_mdef_; }
	int unit_hp() const { return unit_hp_; }

private:
	int id_{ 0 };
	int type_{ 0 };			//类别 0男 1女 2通用
	bool renew_{ false };	//可否续费
	int price_{ 0 };		//续费价格
	int hero_atk_{ 0 };
	int hero_matk_{ 0 };
	int hero_def_{ 0 };
	int hero_mdef_{ 0 };
	int hero_hp_{ 0 };
	int unit_atk_{ 0 };
	int unit_matk_{ 0 };
	int unit_def_{ 0 };
	int unit_mdef_{ 0 };
	int unit_hp_{ 0 };
};

