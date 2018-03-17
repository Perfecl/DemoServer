#include "stdafx.h"
#include "Gate.h"

void CGate::SetGateByProto(const pto_MAP_Gate& ptoGate)
{
	m_nID = ptoGate.id();
	m_nPos = (float)ptoGate.pos();
	m_bOpen = ptoGate.open();
}

void  CGate::SetGateByGate(const CGate* pGate)
{
	m_nID = pGate->m_nID;
	m_nPos = pGate->m_nPos;
	m_bOpen = pGate->m_bOpen;
}

void CGate::SetProtocol(pto_BATTLE_Struct_Gate* pPtoGate)
{
	pPtoGate->set_id(m_nID);
	pPtoGate->set_pos((int)m_nPos);
	pPtoGate->set_state(m_bOpen);
}