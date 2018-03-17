#pragma once

class CArmy
{
public:
	static int		CalculateHeroRank(const CHeroCard* hero);																//计算英雄品质

public:
	CArmy(CPlayer& player);
	~CArmy();

	void			ProcessMessage(const CMessage& msg);
	void			SaveToDatabase();
	void			SendInitialMessage();

	void			ResetHeroInternal();

	void			SetAllHeroesLevel(int level);

	void			AddHero(int hero_id, bool send_msg);					//添加英雄
	CHeroCard*		FindHero(int hero_id);									//寻找英雄
	void			GiveAllHero();											//给与所有的英雄

	void			AddSoldier(int soldier_id);								//添加士兵
	void			GiveAllSoldier();										//给与所有

	void			OpenRunePage(int level);

	void			SetHeroesAndSoldiersProtocol(pto_BATTLE_STRUCT_Master_Ex *pto, bool is_fair);							//设置英雄和士兵的战斗协议

	inline int		soldier_technology(SoldierTechnology type) const { return soldiers_technology_[type]; }

	void			SetAllTechnology(int level);

	std::array<int, kMaxHeroNum>		GetFormationHero() const { return formation_heroes; }
	std::array<int, kMaxSoldierMun>		GetFormationSoldier() const { return formation_soldiers; }
	std::array<int, kSoldierTechCount>	GetTechnology() const { return soldiers_technology_; }

	void			GiveAllRunes(int rune_id);

	inline int		practice_seconds() const { return practice_seconds_; }
	inline time_t	last_practice_time() const { return last_practice_time_; }

	

private:
	CPlayer&							player_;

	std::map<int, SoldierData*>			soldiers_;							//士兵(ID,(灵能等级,灵能层数))
	std::array<int, kSoldierTechCount>	soldiers_technology_;				//士兵科技

	std::map<int, int>					heroes_intimacy_;					//英雄亲密度
	std::map<int, CHeroCard*>			hero_cards_;						//英雄卡牌(hid,hero)
	
	std::array<int, kMaxHeroNum>		formation_heroes;					//军阵英雄
	std::array<int, kMaxSoldierMun>		formation_soldiers;					//军阵士兵

	bool								is_in_practice_cd_{ false };		//英雄是否进入训练CD	
	int									practice_seconds_{ 0 };				//英雄累计训练CD时间(秒)
	time_t								last_practice_time_{ 0 };			//英雄最后训练时间

	int			 						rune_page_{ 0 };					//符文页开放进度
	int									rune_energy_{ 0 };					//符文页能量

	CCriticalSectionZ					lock_;								//线程锁

	std::array<int, 4>					hero_train_temp_;					//英雄训练缓存

	void	__LoadFromDatabase();
	int		__RandomExp();
	void	__TrainHero(CHeroCard* hero, int ratio, int& train_str, int& train_cmd, int& train_int);		//训练英雄
	bool	__TrainValue(int nTrainLevel, int nRatio);														//训练英雄
	int		__GetTrainAdd(int nTrainLevel);																	//获取增加值
	int		__GetTrainReduce(int nTrainLevel);																//获取减少值

	void	__SaveFormation(const CMessage& msg);															//保存军阵
	void	__TrainHero(const CMessage& msg);																//训练英雄
	void	__SaveTrainHero(const CMessage& msg);															//保存培养
	void	__RecruitHero(const CMessage& msg);																//招募英雄
	void	__UpgradeTechnology(const CMessage& msg);														//升级士兵科技
	void	__BuyRunePage();																				//金币开启符文页
	void	__LockRune(const CMessage& msg);																//锁定符文
	void	__FullRuneEnergy();																				//充满符文能量
	void	__WashRune(const CMessage& msg);																//洗练符文
	void	__IntimacyGame(const CMessage& msg);															//亲密度游戏
	void	__ExchangeHeroExp(const CMessage& msg);															//兑换英雄经验
	void	__ClearExchangeCD();																			//清空兑换CD
	void	__UpgradeSoldierTrainLevel(const CMessage& msg);												//升级士兵训练等级

	void    __CultivateHero(const CMessage& msg);															//培养英雄品质

	void	__SetHeroeProtocol(CHeroCard* hero_card, pto_BATTLE_STRUCT_Hero_Ex* pto_hero, bool is_fair);	//设置英雄协议
	void	__GetSuitAddtion(CHeroCard* hero_card, SuitAddition* suit_addition);

	int		__GetTrainLevel(CHeroCard* hero, TrainValueType type);
};
