#pragma once
#include "IPlayerData.h"

class CPlayerCache : public IPlayerData
{
public:
	CPlayerCache(int pid);
	~CPlayerCache();

	virtual inline  int					pid()  override{ return pid_; }
	virtual inline  int					sid()  override{ return sid_; }
	virtual inline  const char*			name()  override{ return name_.c_str(); }
	virtual inline  bool				sex()  override{ return sex_; }
	virtual inline  int					level()  override{ return level_; }
	virtual inline  int					town_id()  override{ return 0; }
	virtual inline  int					guild_id()  override{ return guild_id_; }
	virtual inline  int					using_title() override { return using_title_; }
	virtual inline  int					reputation()  override { return reputatuin_; }
	virtual inline  time_t				offline_time() override { return offline_time_; }

	virtual int							exercise_time()  override;
	virtual int							clothes()  override;

	virtual int							bodygurand() override;

	virtual std::string					guild_name() override;
	virtual int							guild_postion() override;

	virtual const CHeroCard*			hero(int index) override;
	virtual std::vector<const CEquip*>	hero_equip(int hero_id) override;
	virtual int							technology(int index) override;
	virtual int							speed_best_rank() override;
	virtual const BattleRecord&			battle_record(BattleRecordType type) override;
	virtual int							offline_battle_win_streak() override;
	virtual time_t						offline_last_time() override;
	virtual int							battle_week_times(BattleRecordType type) override;

	virtual inline void					guild_id(int guild_id) override;
	
private:
	int const		pid_;
	int				sid_;
	std::string		name_;
	bool			sex_{ false };
	int				level_{ 0 };
	int				guild_id_{ 0 };
	int				using_title_{ 0 };
	int				reputatuin_{ 0 };
	time_t			offline_time_{ 0 };

	void __LoadBaseInfoFromDatabase();

#pragma region 详细信息
	bool			has_detail_{ false };

	std::array<CHeroCard*, 3>						heroes_;				//英雄ID
	std::map<CEquip*, int>							equips_;				//装备ID
	std::array<int, kSoldierTechCount>				technology_;			//士兵科技
	std::array<BattleRecord, kBattleRecordCount>	battle_record_;			//战斗记录

	time_t			offline_battle_last_time_{ 0 };							//离线战斗CD

	int				bodygurand_{ 0 };										//玩家离线保镖
	int				offline_battle_win_streak_{ 0 };						//离线连胜次数

	int				exercise_time_{ 0 };									//练功台经验领取次数

	int				speed_best_rank_{ 0 };									//竞速赛最好成绩
	int				clothes_id_{ 0 };										//服装ID

	int				week_war_times_{ 0 };									//周争霸战斗次数
	int				week_1v1_times_{ 0 };									//周1v1战斗次数
	int				week_3v3_times_{ 0 };									//周3v3战斗次数

	void __LoadDetailInfoFromDatabase();
#pragma endregion
};

