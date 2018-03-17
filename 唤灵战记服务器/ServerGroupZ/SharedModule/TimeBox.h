#pragma once
class CTimeBox
{
public:
	CTimeBox();
	~CTimeBox();
	static void	Load();
	static const CTimeBox* GetTimeBox(int nID);
	int GetTime() const { return m_nTime; }
	int GetSilver() const { return m_nSilver; }
	int GetHonour() const { return m_nHonour; }

private:
	static std::map<int, const CTimeBox*> ms_mapTimeBox;
	int m_nID{ 0 };
	int m_nTime{ 0 };
	int m_nSilver{ 0 };
	int m_nHonour{ 0 };
};

