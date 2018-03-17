#pragma once
class CStageLoot
{
public:
	static void Load();
	static int ProduceBulletID();
	static bool IsStageLoot(int item_id);
	static const CStageLoot* GetStageLootByBulletID(int bullet_id);
private:
	static std::vector<const CStageLoot*> stage_loot_lib_;
public:
	CStageLoot();
	~CStageLoot();
	int num() const { return num_; }
private:
	int id_{ 0 };
	float odds_{ 0 };//¼¸ÂÊ
	int item_id_{ 0 };
	int num_{ 0 };
	int bullet_id_{ 0 };
};

