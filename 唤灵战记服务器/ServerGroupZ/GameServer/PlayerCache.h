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

#pragma region ��ϸ��Ϣ
	bool			has_detail_{ false };

	std::array<CHeroCard*, 3>						heroes_;				//Ӣ��ID
	std::map<CEquip*, int>							equips_;				//װ��ID
	std::array<int, kSoldierTechCount>				technology_;			//ʿ���Ƽ�
	std::array<BattleRecord, kBattleRecordCount>	battle_record_;			//ս����¼

	time_t			offline_battle_last_time_{ 0 };							//����ս��CD

	int				bodygurand_{ 0 };										//������߱���
	int				offline_battle_win_streak_{ 0 };						//������ʤ����

	int				exercise_time_{ 0 };									//����̨������ȡ����

	int				speed_best_rank_{ 0 };									//��������óɼ�
	int				clothes_id_{ 0 };										//��װID

	int				week_war_times_{ 0 };									//������ս������
	int				week_1v1_times_{ 0 };									//��1v1ս������
	int				week_3v3_times_{ 0 };									//��3v3ս������

	void __LoadDetailInfoFromDatabase();
#pragma endregion
};

