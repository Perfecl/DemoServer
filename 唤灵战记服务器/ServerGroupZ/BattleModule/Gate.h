#pragma once

#define GateVolume 50.0f

class CGate
{
public:
	CGate() = default;
	~CGate() = default;

	void	SetGateByProto(const pto_MAP_Gate& ptoGate);
	void	SetGateByGate(const CGate* pGate);
	int		GetID(){ return m_nID; }
	bool	IsOpen(){ return m_bOpen; }
	float	GetPos(){ return m_nPos; }
	void	SetProtocol(pto_BATTLE_Struct_Gate* pPtoGate);
	void	OpenGate(){ m_bOpen = true; }

private:
	int		m_nID{ 0 };
	float	m_nPos{ 0 };
	bool	m_bOpen{ false };
};

