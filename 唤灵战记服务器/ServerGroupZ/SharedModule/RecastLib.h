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
	int m_nEquipID{ 0 };				//成品ID
	int m_nStrengthenLV{ 0 };			//所需要强化等级
	int m_nMaterialEquipID{ 0 };		//材料装备ID
	int m_nDrawingID{ 0 };				//图纸ID
	int m_nDrawingNum{ 0 };             //图纸数量
	int m_nMoney{ 0 };					//所需银币
	int m_nMaterial1ID{ 0 };			//材料道具一ID
	int m_nMaterial1Num{ 0 };			//材料道具一数量
	int m_nMaterial2ID{ 0 };			//材料道具二ID
	int m_nMaterial2Num{ 0 };			//材料道具二数量
	int m_nMaterial3ID{ 0 };			//材料道具三ID
	int m_nMaterial3Num{ 0 };			//材料道具三数量
	int m_nMaterial4ID{ 0 };			//材料道具四ID
	int m_nMaterial4Num{ 0 };			//材料道具四数量
};

