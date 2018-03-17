#pragma once
class CCustomBullet
{
public:
	CCustomBullet();
	~CCustomBullet();

	void	SetCustomBulletByProto(const pto_MAP_Bullet& ptoBullet);
	int		GetBulletID() const { return m_nBulletID; }
	int		GetPos() const { return m_nPos; }
	int		GetEffectValue() const { return m_nEffectValue; }
	int		GetCustomID() const { return m_nCustomID; }
	Force	GetForce() const { return m_enForce; }
	bool	GetInfinite() const { return m_bInfinite; }
	void	SetProtocol(pto_BATTLE_Struct_Bullet* pPtoGate);

private:
	int		m_nCustomID{ 0 };
	int		m_nBulletID{ 0 };
	int		m_nPos{ 0 };
	int		m_nEffectValue{ 0 };
	Force	m_enForce{ Force::UNKNOWN };
	bool	m_bInfinite{ false };
};
