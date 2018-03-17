#pragma once
class CHeroQuality
{
public:
	CHeroQuality();
	~CHeroQuality();
	static void  Load();
	static const CHeroQuality* GetHeroQuality(int quality);
	static bool QualityUp(int quality, float ex_chance);

	int time() const { return time_; }
	int hp() const { return hp_; }
	int atk() const { return atk_; }
	int def() const { return def_; }
	int matk() const { return matk_; }
	int mdef() const { return mdef_; }
	int strength() const { return strength_; }
	int command() const { return command_; }
	int intelligence() const { return intelligence_; }
	int min_safety() const { return min_safety_; }

private:
	static std::map<int, const CHeroQuality*>	hero_quality_lib_;
	int lv_{ 0 };
	float chance_{ 0 };
	int time_{ 0 };
	int hp_{ 0 };
	int atk_{ 0 };
	int def_{ 0 };
	int matk_{ 0 };
	int mdef_{ 0 };
	int strength_{ 0 };
	int command_{ 0 };
	int intelligence_{ 0 };
	int min_safety_{ 0 };
};

