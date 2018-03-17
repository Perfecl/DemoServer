#include "stdafx.h"
#include "ProtoPackage.h"
#include "Bullet.h"
#include "Battle.h"

CProtoPackage::CProtoPackage()
{

}

CProtoPackage::~CProtoPackage()
{

}

void CProtoPackage::NewPackage()
{
	m_pPerPackage = m_PerLoopSender.add_datas();

	m_pPerPackage->set_ntime(time(0));

	m_pClickHeroCard = m_pPerPackage->mutable_click_hero_card();
	m_pAddUnit = m_pPerPackage->mutable_add_unit();
	m_pCreateBulletProto = m_pPerPackage->mutable_percreatebullet();
	m_pBulletMoveProto = m_pPerPackage->mutable_perbulletmove();
	m_pBulletDeadProto = m_pPerPackage->mutable_perbulletdead();
	m_pBulletBulletMoveCompleteProto = m_pPerPackage->mutable_perbulletmovecomplete();
	m_pDamagedProto = m_pPerPackage->mutable_perbedamage();
	m_pHealProto = m_pPerPackage->mutable_perheal();
	m_pStartAttackPtoro = m_pPerPackage->mutable_perstartattack();
	m_pStartMoveProto = m_pPerPackage->mutable_perstartmove();
	m_pStopMoveProto = m_pPerPackage->mutable_perstopmove();
	m_pFallBackProto = m_pPerPackage->mutable_perfallback();
	m_pStartUseSkillProto = m_pPerPackage->mutable_perstartuseskill();
	m_pUseSkillCompleteProto = m_pPerPackage->mutable_peruseskillcomplete();
	m_pBreakHideProto = m_pPerPackage->mutable_perbreakhide();
	m_pUnitDeadPtoro = m_pPerPackage->mutable_perunitdead();
	m_pBuffStartProto = m_pPerPackage->mutable_perbuffstart();
	m_pBuffEndProto = m_pPerPackage->mutable_perbuffend();
	m_pRemoveUnit = m_pPerPackage->mutable_removeunit();
}

bool CProtoPackage::IsNull()
{
	return m_bIsNull;
}

void CProtoPackage::Clear()
{
	m_PerLoopSender.Clear();

	for (auto it : m_mapUnitMsg)
		delete it.second;

	m_mapUnitMsg.clear();

	m_bIsNull = true;
}

void CProtoPackage::UseSkillComplete(int nUnitID, int nSkillIndex)
{
	m_bIsNull = false;
	auto ptoUseSkillComplete = m_pUseSkillCompleteProto->add_sinfo();
	ptoUseSkillComplete->set_nid(nUnitID);
	ptoUseSkillComplete->set_nskillid(nSkillIndex);
}

void CProtoPackage::FallBack(int UnitID, float fX)
{
	m_bIsNull = false;
	auto ptoFallback = m_pFallBackProto->add_sinfo();
	ptoFallback->set_nid(UnitID);
	ptoFallback->set_nx(fX);
}

void CProtoPackage::BreakHide(int UnitID)
{
	m_bIsNull = false;
	m_pBreakHideProto->add_nid(UnitID);
}

void CProtoPackage::ClickHeroCard(pto_BATTLE_Struct_ClickHeroCard* pBSCHC)
{
	m_bIsNull = false;
	auto ptoClickHeroCard = m_pClickHeroCard->add_click_hero_card();
	ptoClickHeroCard->set_ncardindex(pBSCHC->ncardindex());
	ptoClickHeroCard->set_nmaster(pBSCHC->nmaster());
	ptoClickHeroCard->set_nunitid(pBSCHC->nunitid());
	ptoClickHeroCard->set_nrank(pBSCHC->nrank());
	ptoClickHeroCard->set_suit_id(pBSCHC->suit_id());
}

void CProtoPackage::AddUnit(pto_BATTLE_Struct_Unit* pUnit)
{
	m_bIsNull = false;
	auto ptoUnit = m_pAddUnit->add_soldier();
	ptoUnit->CopyFrom(*pUnit);
}

void CProtoPackage::BeDamage(int ObjID, int nValue, int nTargetType, int nSrcID, int nSrcType, bool bIsCrit, int nDamageEffectID, bool isNormalAttack)
{
	m_bIsNull = false;
	auto pDamageInfo = m_pDamagedProto->add_sdamage();
	pDamageInfo->set_nid(ObjID);
	pDamageInfo->set_ndamage(nValue);
	pDamageInfo->set_nattackerid(nSrcID);
	pDamageInfo->set_nattackertype(nSrcType);
	pDamageInfo->set_ndamageeffectid(nDamageEffectID);
	pDamageInfo->set_ntargettype(nTargetType);
	pDamageInfo->set_biscrit(bIsCrit);
	pDamageInfo->set_bisnormalattack(isNormalAttack);
}

void CProtoPackage::Heal(int nUnitID, int nValue, int nHealerID, int nHealerType, int nHealEffectID, int nTargetType, bool bIsCrit)
{
	m_bIsNull = false;
	auto pHealInfo = m_pHealProto->add_sheal();
	pHealInfo->set_nid(nUnitID);
	pHealInfo->set_nheal(nValue);
	pHealInfo->set_nhealid(nHealerID);
	pHealInfo->set_nhealertype(nHealerType);
	pHealInfo->set_nhealeffectid(nHealEffectID);
	pHealInfo->set_ntargettype(nTargetType);
	pHealInfo->set_biscrit(bIsCrit);
}

void CProtoPackage::BulletMove(int nID, int nBulletType, bool bIsMove)
{
	m_bIsNull = false;
	auto ptoBulletMove = m_pBulletMoveProto->add_sbulletmove();
	ptoBulletMove->set_nid(nID);
	ptoBulletMove->set_ntype(nBulletType);
	ptoBulletMove->set_bismove(bIsMove);
}

void CProtoPackage::BulletDead(int nID, int nBulletType, int nDeathByWho)
{
	m_bIsNull = false;
	auto ptoBulletDead = m_pBulletDeadProto->add_sbulletdead();
	ptoBulletDead->set_nid(nID);
	ptoBulletDead->set_ntype(nBulletType);
	ptoBulletDead->set_nbywhoid(nDeathByWho);
}

void CProtoPackage::BulletMoveComplete(int nID, int nBulletType, bool IsComplete)
{
	m_bIsNull = false;
	auto ptoBulletMoveComplete = m_pBulletBulletMoveCompleteProto->add_smovecomplete();
	ptoBulletMoveComplete->set_nid(nID);
	ptoBulletMoveComplete->set_ntype(nBulletType);
	ptoBulletMoveComplete->set_biscomplete(IsComplete);
}

void CProtoPackage::CreateBullet(pto_BATTLE_Struct_Bullet* ptoBullet)
{
	m_bIsNull = false;
	auto ptoBulletDest = m_pCreateBulletProto->add_data();
	ptoBulletDest->CopyFrom(*ptoBullet);
}

void CProtoPackage::UnitDead(int nUnitID)
{
	m_bIsNull = false;
	m_pUnitDeadPtoro->add_nid(nUnitID);
}

void CProtoPackage::SetBuff(int nID, int  unit_id, int buff_id)
{
	m_bIsNull = false;
	auto pto = m_pBuffStartProto->add_infos();
	pto->set_id(nID);
	pto->set_unit_id(unit_id);
	pto->set_buff_id(buff_id);
}

void CProtoPackage::EndBuff(int  nID, int unit_id)
{
	m_bIsNull = false;
	auto pto = m_pBuffEndProto->add_infos();
	pto->set_id(nID);
	pto->set_unit_id(unit_id);
}

const std::string& CProtoPackage::GetMsgStr()
{
	for (auto &it : m_mapUnitMsg)
	{
		if (dynamic_cast<pto_BATTLE_Struct_StartAttack*>(it.second))
		{
			auto  pto = m_pStartAttackPtoro->add_sinfo();
			pto->CopyFrom(*it.second);
		}
		else if (dynamic_cast<pto_BATTLE_Struct_StartMove*>(it.second))
		{
			auto  pto = m_pStartMoveProto->add_sinfo();
			pto->CopyFrom(*it.second);
		}
		else if (dynamic_cast<pto_BATTLE_Struct_StopMove*>(it.second))
		{
			auto  pto = m_pStopMoveProto->add_sinfo();
			pto->CopyFrom(*it.second);
		}
		else if (dynamic_cast<pto_BATTLE_Struct_StartUseSkill*>(it.second))
		{
			auto  pto = m_pStartUseSkillProto->add_sinfo();
			pto->CopyFrom(*it.second);
		}
	}

	m_strData.clear();

	m_PerLoopSender.SerializeToString(&m_strData);

	return m_strData;
}

void CProtoPackage::StartAttack(int nUnitID, int nTargetID, int TargetType, float fX)
{
	m_bIsNull = false;
	pto_BATTLE_Struct_StartAttack* pto = new pto_BATTLE_Struct_StartAttack;
	pto->set_nid(nUnitID);
	pto->set_ntargetid(nTargetID);
	pto->set_ntargettype(TargetType);
	pto->set_nx(fX);

	__InsertPto(nUnitID, pto);
}

void CProtoPackage::UnitStartMove(int nUnitID, bool bDirection)
{
	m_bIsNull = false;
	pto_BATTLE_Struct_StartMove* pto = new pto_BATTLE_Struct_StartMove;
	pto->set_nid(nUnitID);
	pto->set_bdirection(bDirection);

	__InsertPto(nUnitID, pto);
}

void CProtoPackage::StopMove(int nUnitID, int nValue, float fX)
{
	m_bIsNull = false;
	pto_BATTLE_Struct_StopMove* pto = new pto_BATTLE_Struct_StopMove;
	pto->set_nid(nUnitID);
	pto->set_nx(fX);

	__InsertPto(nUnitID, pto);
}

void CProtoPackage::StartUseSkill(int nUnitID, int nSkillIndex, int nSkillType, float fX)
{
	m_bIsNull = false;
	pto_BATTLE_Struct_StartUseSkill* pto = new pto_BATTLE_Struct_StartUseSkill;

	pto->set_nid(nUnitID);
	pto->set_nskillid(nSkillIndex);
	pto->set_nskilltype(nSkillType);
	pto->set_nx(fX);

	__InsertPto(nUnitID, pto);
}

void CProtoPackage::RemoveUnit(int nID)
{
	m_bIsNull = false;
	m_pRemoveUnit->add_id(nID);
}

bool CProtoPackage::__InsertPto(int nUnitID, ::google::protobuf::Message* pto)
{
	auto it = m_mapUnitMsg.find(nUnitID);

	bool flag{ false };

	if (it == m_mapUnitMsg.cend())
	{
		m_mapUnitMsg.insert(std::make_pair(nUnitID, pto));
		flag = true;
	}
	else
	{
		delete it->second;
		it->second = pto;
		flag = false;
	}

	return flag;
}
