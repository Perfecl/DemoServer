#pragma once

//培养种类
enum TrainType
{
	kTrainStr,		//力量
	kTrainCmd,		//统率
	kTrainInt,		//智力
	kTrainCount		//数量
};

class CEquip;
class CHeroCard
{
	friend class CArmy;
	friend class CPlayerCache;
public:
	static const int kRuneNum{ 16 };

public:
	CHeroCard(int hero_id);
	~CHeroCard();

	inline int	hero_id() const { if (hero_template_) return hero_template_->GetID(); return 0; }
	inline int	hero_level() const { return level_; }
	inline int  hero_exp() const { return exp_; }
	inline int  quality() const { return quality_; }
	inline int	safety_times() const { return safety_times_; }

	inline int  ChangeLevel(int num){ return level_ += num; }
	inline int  ChangeExp(int num){ return exp_ += num; }
	inline void	SetExp(int num){ exp_ = num; }
	inline void SetLevel(int num){ level_ = num; }

	inline int	train_str() const { return  trains_[kTrainStr]; }
	inline int	train_cmd() const { return  trains_[kTrainCmd]; }
	inline int	train_int() const { return  trains_[kTrainInt]; }

	inline void	change_train_str(int value) { trains_[kTrainStr] += value; }
	inline void	change_train_cmd(int value) { trains_[kTrainCmd] += value; }
	inline void	change_train_int(int value) { trains_[kTrainInt] += value; }

	inline void	set_train_str(int value) { trains_[kTrainStr] = value; }
	inline void	set_train_cmd(int value) { trains_[kTrainCmd] = value; }
	inline void	set_train_int(int value) { trains_[kTrainInt] = value; }

	inline const CHeroTP* hero_template() const { return hero_template_; }

	inline std::array<unsigned, kRuneNum>* rune(){ return &runes_; }

	inline bool today_is_internal_affair() const { return today_is_internal_affair_; }
	inline void set_today_is_internal_affair(bool is){ today_is_internal_affair_ = is; }

	inline void set_quality(int num){ quality_ = num; }

	inline std::array<unsigned, kRuneNum> GetRunes() const { return runes_; }

private:
	const CHeroTP* const			hero_template_;				//英雄模板
	int								level_{ 1 };				//英雄等级
	int								exp_{ 0 };					//英雄经验
	int								quality_{ 1 };				//品质
	int								safety_times_{ 0 };			//保护卡次数

	std::array<int, kTrainCount>	trains_;					//英雄训练值
	std::array<unsigned, kRuneNum>	runes_;						//符文组

	bool today_is_internal_affair_{ false };					//今日是否使用过内政
};