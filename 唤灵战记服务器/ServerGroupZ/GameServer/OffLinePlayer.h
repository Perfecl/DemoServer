#pragma once

class CHeroCard;
struct OffLineHeroRune
{
	std::array<unsigned, 16> m_vctRunes;
};

class COffLinePlayer
{
public:
	static  void ProduceOfflineBattleAI();
	static  void InsertAIIntoRanking();
	static	COffLinePlayer* FindOffLinePlayerByPID(int nPID);
	static  void DeleteOfflineAI();

private:
	static  std::map<int, COffLinePlayer*> ms_mapOfflineBattleAI;

public:
	COffLinePlayer();
	~COffLinePlayer();
	COffLinePlayer(int nPID);

	void	SetOffLinePlayerByPlayer(CPlayer* pPlayer);
	void	SetOffLinePlayerByData(SPIPlayerData pData);

	int		GetPID()const{ return m_nPID; }
	std::string	GetName()const{ return m_strName; }
	bool	GetSex()const{ return m_bSex; }
	short	GetLevel()const{ return m_nLevel; }
	int     GetBodyguard(){ return m_nBodyguard; }
	int     GetDartCarLevel(){ return m_nDartCarLevel; }

	int			GetDressID(){ return m_nDressID; }
	const CHeroCard*	GerHeroCard(int index){ return heroes[index]; }
	std::map<const CEquip*, int>* GetAllEquip() { return &equips; }

private:
	int									m_nPID;							//角色ID
	std::string							m_strName;						//角色名
	bool								m_bSex{ false };				//性别
	int									m_nDressID{ 0 };				//时装ID

	short								m_nLevel{ 0 };					//等级

	std::array<const CHeroCard*, 3>		heroes;							//英雄
	std::map<const CEquip*, int>		equips;							//装备I

	int									m_nBodyguard{ 0 };				//保镖ID
	int									m_nDartCarLevel{ 0 };			//镖车等级
};
