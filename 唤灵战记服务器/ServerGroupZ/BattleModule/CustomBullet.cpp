#include "stdafx.h"
#include "CustomBullet.h"

CCustomBullet::CCustomBullet()
{

}

CCustomBullet::~CCustomBullet()
{

}

void CCustomBullet::SetCustomBulletByProto(const pto_MAP_Bullet& pto)
{
	m_nCustomID = pto.id();
	m_nBulletID = pto.bullet_id();
	m_nPos = pto.pos();
	m_nEffectValue = pto.effect_value();
	m_bInfinite = pto.infinite();
}

void CCustomBullet::SetProtocol(pto_BATTLE_Struct_Bullet* pto)
{

}
