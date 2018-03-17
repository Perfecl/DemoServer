#pragma once
class CTownBox
{
public:
	CTownBox() = default;
	~CTownBox() = default;
	static void Load();
	static const CTownBox*  GetTownBox(int nTownID, int nDifficult);
	static const CTownBox*  GetTownBoxByStageLevel(int nStageLevel);

	int GetTownID() const { return m_nTownID;}
	int GetStageLevel() const { return m_nStageLevel;}
	int GetStageDifficulty() const { return m_nStageDifficulty;}
	int GetSilver() const { return m_nSilver;}
	int GetGold() const { return m_nGold;}
	int GetHonour() const { return m_nHonour;}
	int GetStamina() const { return m_nStamina;}

private:
	static std::vector<const CTownBox*> ms_vctTownBox;

	int m_nTownID{ 0 };
	int m_nStageLevel{ 0 };
	int m_nStageDifficulty{ 0 };
	int m_nSilver{ 0 };
	int m_nGold{ 0 };
	int m_nHonour{ 0 };
	int m_nStamina{ 0 };
};

