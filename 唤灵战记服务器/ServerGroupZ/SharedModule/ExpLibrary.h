#pragma once

class CExpLibrary
{
public:
	static void		Load();
	static __int64	GetSoldierExp(int nLV, SoldierTechnology type);
	static const CExpLibrary* GetExpLibrary(int nLV);
	
private:
	static std::map<int, const CExpLibrary*> ms_mapExpLibrary;

	static float ms_nSoldierAllExp;		//�ܿƼ�
	static float ms_nSoldierAtkExp;		//ʿ������
	static float ms_nSoldierMAtkExp;	//ʿ��ħ��
	static float ms_nSoldierDefExp;		//ʿ������
	static float ms_nSoldierMDefExp;	//ʿ��ħ��
	static float ms_nSoldierHPExp;	    //ʿ��Ѫ��

public:
	CExpLibrary() = default;
	~CExpLibrary() = default;

	__int64		GetNeedExp() const { return m_nExp; }
	int			GetNeedStage() const { return m_nStage; }

private:
	int			m_nLevel{ 0 };				//�����ȼ�
	int			m_nStage{ 0 };
	__int64		m_nExp{ 0 };				//�������辭��
	__int64		m_nSoldierExp{ 0 };
};

