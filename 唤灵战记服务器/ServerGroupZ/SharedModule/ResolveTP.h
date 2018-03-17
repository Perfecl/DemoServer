#pragma once
class CResolveTP
{
public:
	CResolveTP();
	~CResolveTP();
	static void Load();
	static const CResolveTP* GetResolveTP(int quality);

	float enchant_chance() const { return enchant_chance_; }
	int enchant_min() const { return enchant_min_; }
	int enchant_max() const { return enchant_max_; }
	float catalyze_chance() const { return catalyze_chance_; }
	int catalyze_min() const { return catalyze_min_; }
	int catalyze_max() const { return catalyze_max_; }

private:
	static std::map<int, const CResolveTP*> resolve_library_;
	int quality_{ 0 };
	float enchant_chance_{ 0 };
	int enchant_min_{ 0 };
	int enchant_max_{ 0 };
	float catalyze_chance_{ 0 };
	int catalyze_min_{ 0 };
	int catalyze_max_{ 0 };
};

