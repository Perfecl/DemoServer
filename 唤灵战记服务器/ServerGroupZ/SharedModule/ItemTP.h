#pragma once

enum ITEM_TYPE
{
	IT_ITEM,
	IT_EQUIP,
};

enum ITEM_ATTRIBUTE
{
	IA_MATERIA = 1,
	IA_OPEN,
	IA_LOTTERY,
	IA_HeroExp,
};

class CItemTP
{
public:
	static void  Load();
	static const CItemTP* GetItemTP(int nID);

private:
	static std::map<int, const CItemTP*> ms_mapItemTPs;

public:
	CItemTP() = default;
	~CItemTP() = default;

	int GetMaxNum() const { return m_nMaxNum; }
	int	GetItemID() const { return m_nID; }
	int GetSellPrice() const { return m_nSellPrice; }
	int GetFunctional() const { return m_nFunctional; }
	int GetUseLevel() const { return m_nUseLevel; }
	ITEM_ATTRIBUTE GetType() const {return m_enType;}

private:
	int					m_nID{ 0 };
	std::string			m_nName;
	int					m_nLevel{ 0 };
	int					m_nIconID{ 0 };
	ITEM_ATTRIBUTE		m_enType{ IA_MATERIA };
	int					m_nFunctional{ 0 };
	int					m_nUseLevel{ 0 };
	int					m_nMaxNum{ 0 };
	int					m_nPrice{ 0 };
	int					m_nSellPrice{ 0 };
};

