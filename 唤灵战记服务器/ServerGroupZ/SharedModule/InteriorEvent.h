#pragma once

enum InteriorEventRewardType
{
	IERT_Null = 0,
	IERT_Silver,
	IERT_Gold,
	IERT_Honor,	
	IERT_Reputation,
};
class CInteriorEvent
{
public:
	CInteriorEvent() = default;
	~CInteriorEvent() = default;
	static void Load();
	static const CInteriorEvent* RandomProduceEvents();
	static const CInteriorEvent* RandomTrainEvents();
	static const CInteriorEvent* RandomSearchEvents();

	int GetID() const { return m_nID; }
	InteriorEventRewardType GetRewardType() const { return m_enRewardType; }
	int GetRewardBase() const { return m_nRewardBase; }
	int GetRewardAddition() const { return m_nRewardAddition; }

private:
	static std::map<int, const CInteriorEvent*> ms_mapProduceEvents;
	static std::map<int, const CInteriorEvent*> ms_mapTrainEvents;
	static std::map<int, const CInteriorEvent*> ms_mapSearchEvents;

	int		m_nID{ 0 };
	float	m_fChance{ 0 }; 
	InteriorEventRewardType m_enRewardType{ IERT_Null };
	int m_nRewardBase{ 0 };
	int m_nRewardAddition{ 0 };
};

