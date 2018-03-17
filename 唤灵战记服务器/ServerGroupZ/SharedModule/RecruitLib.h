#pragma once

class CRecruitLib
{
public:
	static void  Load();
	static const CRecruitLib* GetRecruitLib(int nID);

private:
	static std::map<int, const CRecruitLib*> ms_mapRecruitLib;
	static std::map<int, int> ms_mapGiftLib;
	static std::map<int, int> ms_mapCellLib;

public:
	CRecruitLib() = default;
	~CRecruitLib() = default;

	static int GetGiftRatio(int nGiftID);
	static int GetGiftID(int nCellID);
	__int64 GetSilver() const { return m_nSilver; }
	int GetNeedLevel() const { return m_nNeedLevel; }
	int GetReputation() const { return m_nReputation; }

	static int ProduceCellID();

private:
	int		m_nHeroID{ 0 };
	int		m_nReputation{ 0 };
	int		m_nNeedLevel{ 0 };
	__int64 m_nSilver{ 0 };
};
