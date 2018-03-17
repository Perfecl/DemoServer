#include "stdafx.h"
#include "BuffTP.h"

std::map<int, const CBuffTP*> CBuffTP::ms_mapBuffs;

void CBuffTP::Load()
{
	Library_Buff datBuffLib;
	datBuffLib.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Buff.txt"));

	for (int i = 0; i < datBuffLib.buff_size(); i++)
	{
		Model_Buff ptoBuff = datBuffLib.buff(i);

		CBuffTP *pBuffTP = new CBuffTP;
		pBuffTP->m_nID = ptoBuff.id();
		pBuffTP->m_strName = ptoBuff.name();
		pBuffTP->m_nIcon = ptoBuff.icon_id();

		if (ptoBuff.functions_size() != ptoBuff.effect_size())
			int a = pBuffTP->m_nID;

		for (int i = 0; i < ptoBuff.functions_size(); i++)
		{
			if (ptoBuff.functions(i) >= 1000)
				pBuffTP->m_vctAdvanceEffect.push_back(ptoBuff.functions(i));
			else
				pBuffTP->m_vctEffects.push_back(std::make_pair(ptoBuff.functions(i), ptoBuff.effect(i)));
		}

		pBuffTP->m_bIsDebuff = ptoBuff.isdebuff();
		pBuffTP->m_nInterval = ptoBuff.interval();
		pBuffTP->m_nTime = ptoBuff.time();

		auto it = ms_mapBuffs.insert(std::make_pair(pBuffTP->m_nID, pBuffTP));
		if (false == it.second)
			delete pBuffTP;
	}
}

const CBuffTP* CBuffTP::GetBuffTP(int nID)
{
	auto it = ms_mapBuffs.find(nID);
	if (ms_mapBuffs.cend() == it)
		return nullptr;
	else
		return it->second;
}
