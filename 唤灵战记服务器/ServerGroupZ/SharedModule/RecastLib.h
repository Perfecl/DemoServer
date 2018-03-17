#pragma once
class CRecastLib
{
public:
	static void Load();
	static const CRecastLib* GetRecastLib(int nID);

private:
	static std::map<int, const CRecastLib*> ms_mapRecastLib;
public:
	CRecastLib();
	~CRecastLib();

	int GetEquipID() const { return m_nEquipID; }
	int GetStrengthenLV() const { return m_nStrengthenLV; }
	int GetMaterialEquipID() const { return m_nMaterialEquipID; }
	int GetDrawingID() const { return m_nDrawingID; }
	int GetDrawingNum() const { return m_nDrawingNum; }
	int GetMoney() const { return m_nMoney; }
	int GetMaterial1ID() const { return m_nMaterial1ID; }
	int GetMaterial1Num() const { return m_nMaterial1Num; }
	int GetMaterial2ID() const { return m_nMaterial2ID; }
	int GetMaterial2Num() const { return m_nMaterial2Num; }
	int GetMaterial3ID() const { return m_nMaterial3ID; }
	int GetMaterial3Num() const { return m_nMaterial3Num; }
	int GetMaterial4ID() const { return m_nMaterial4ID; }
	int GetMaterial4Num() const { return m_nMaterial4Num; }

private:
	int m_nEquipID{ 0 };				//��ƷID
	int m_nStrengthenLV{ 0 };			//����Ҫǿ���ȼ�
	int m_nMaterialEquipID{ 0 };		//����װ��ID
	int m_nDrawingID{ 0 };				//ͼֽID
	int m_nDrawingNum{ 0 };             //ͼֽ����
	int m_nMoney{ 0 };					//��������
	int m_nMaterial1ID{ 0 };			//���ϵ���һID
	int m_nMaterial1Num{ 0 };			//���ϵ���һ����
	int m_nMaterial2ID{ 0 };			//���ϵ��߶�ID
	int m_nMaterial2Num{ 0 };			//���ϵ��߶�����
	int m_nMaterial3ID{ 0 };			//���ϵ�����ID
	int m_nMaterial3Num{ 0 };			//���ϵ���������
	int m_nMaterial4ID{ 0 };			//���ϵ�����ID
	int m_nMaterial4Num{ 0 };			//���ϵ���������
};

