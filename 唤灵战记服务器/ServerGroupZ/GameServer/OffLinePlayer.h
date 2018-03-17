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
	int									m_nPID;							//��ɫID
	std::string							m_strName;						//��ɫ��
	bool								m_bSex{ false };				//�Ա�
	int									m_nDressID{ 0 };				//ʱװID

	short								m_nLevel{ 0 };					//�ȼ�

	std::array<const CHeroCard*, 3>		heroes;							//Ӣ��
	std::map<const CEquip*, int>		equips;							//װ��I

	int									m_nBodyguard{ 0 };				//����ID
	int									m_nDartCarLevel{ 0 };			//�ڳ��ȼ�
};
