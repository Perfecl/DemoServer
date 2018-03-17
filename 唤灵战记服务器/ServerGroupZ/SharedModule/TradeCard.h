#pragma once

class CTradeCard
{
public:
	static void Load();
	static const CTradeCard* GetTradeCard(int nID);

private:
	static std::map<int, const CTradeCard*> ms_mapTradeCard;

public:
	CTradeCard() = default;
	~CTradeCard() = default;

	int    GetRewardType() const { return m_nRewardType; }
	int    GetRewardBase() const { return m_nRewardBase; }
	int    GetRewardMul() const{ return m_nRewardMul; }

private:
	int    m_nID{ 0 };
	int    m_nRewardType{ 0 };  
	int    m_nRewardBase{ 0 };  //»ù´¡½±Àø
	int    m_nRewardMul{ 0 };   //½±ÀøÏµÊý
};

