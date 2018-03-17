#pragma once

class CArmy
{
public:
	static int		CalculateHeroRank(const CHeroCard* hero);																//����Ӣ��Ʒ��

public:
	CArmy(CPlayer& player);
	~CArmy();

	void			ProcessMessage(const CMessage& msg);
	void			SaveToDatabase();
	void			SendInitialMessage();

	void			ResetHeroInternal();

	void			SetAllHeroesLevel(int level);

	void			AddHero(int hero_id, bool send_msg);					//���Ӣ��
	CHeroCard*		FindHero(int hero_id);									//Ѱ��Ӣ��
	void			GiveAllHero();											//�������е�Ӣ��

	void			AddSoldier(int soldier_id);								//���ʿ��
	void			GiveAllSoldier();										//��������

	void			OpenRunePage(int level);

	void			SetHeroesAndSoldiersProtocol(pto_BATTLE_STRUCT_Master_Ex *pto, bool is_fair);							//����Ӣ�ۺ�ʿ����ս��Э��

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

	std::map<int, SoldierData*>			soldiers_;							//ʿ��(ID,(���ܵȼ�,���ܲ���))
	std::array<int, kSoldierTechCount>	soldiers_technology_;				//ʿ���Ƽ�

	std::map<int, int>					heroes_intimacy_;					//Ӣ�����ܶ�
	std::map<int, CHeroCard*>			hero_cards_;						//Ӣ�ۿ���(hid,hero)
	
	std::array<int, kMaxHeroNum>		formation_heroes;					//����Ӣ��
	std::array<int, kMaxSoldierMun>		formation_soldiers;					//����ʿ��

	bool								is_in_practice_cd_{ false };		//Ӣ���Ƿ����ѵ��CD	
	int									practice_seconds_{ 0 };				//Ӣ���ۼ�ѵ��CDʱ��(��)
	time_t								last_practice_time_{ 0 };			//Ӣ�����ѵ��ʱ��

	int			 						rune_page_{ 0 };					//����ҳ���Ž���
	int									rune_energy_{ 0 };					//����ҳ����

	CCriticalSectionZ					lock_;								//�߳���

	std::array<int, 4>					hero_train_temp_;					//Ӣ��ѵ������

	void	__LoadFromDatabase();
	int		__RandomExp();
	void	__TrainHero(CHeroCard* hero, int ratio, int& train_str, int& train_cmd, int& train_int);		//ѵ��Ӣ��
	bool	__TrainValue(int nTrainLevel, int nRatio);														//ѵ��Ӣ��
	int		__GetTrainAdd(int nTrainLevel);																	//��ȡ����ֵ
	int		__GetTrainReduce(int nTrainLevel);																//��ȡ����ֵ

	void	__SaveFormation(const CMessage& msg);															//�������
	void	__TrainHero(const CMessage& msg);																//ѵ��Ӣ��
	void	__SaveTrainHero(const CMessage& msg);															//��������
	void	__RecruitHero(const CMessage& msg);																//��ļӢ��
	void	__UpgradeTechnology(const CMessage& msg);														//����ʿ���Ƽ�
	void	__BuyRunePage();																				//��ҿ�������ҳ
	void	__LockRune(const CMessage& msg);																//��������
	void	__FullRuneEnergy();																				//������������
	void	__WashRune(const CMessage& msg);																//ϴ������
	void	__IntimacyGame(const CMessage& msg);															//���ܶ���Ϸ
	void	__ExchangeHeroExp(const CMessage& msg);															//�һ�Ӣ�۾���
	void	__ClearExchangeCD();																			//��նһ�CD
	void	__UpgradeSoldierTrainLevel(const CMessage& msg);												//����ʿ��ѵ���ȼ�

	void    __CultivateHero(const CMessage& msg);															//����Ӣ��Ʒ��

	void	__SetHeroeProtocol(CHeroCard* hero_card, pto_BATTLE_STRUCT_Hero_Ex* pto_hero, bool is_fair);	//����Ӣ��Э��
	void	__GetSuitAddtion(CHeroCard* hero_card, SuitAddition* suit_addition);

	int		__GetTrainLevel(CHeroCard* hero, TrainValueType type);
};
