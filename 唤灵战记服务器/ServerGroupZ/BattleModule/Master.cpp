#include "stdafx.h"
#include "Master.h"
#include "Battle.h"
#include "Hero.h"
#include "BulletTP.h"
#include "Bullet.h"
#include "Building.h"

CMaster::CMaster(CBattle *pBattle) :
m_pBattle{ pBattle }
{
	for (auto &it : m_arrSoldiers)
	{
		it.m_nCardID = 0;
		it.m_nCoolDown = 0;
		it.m_nCanUseNum = 0;
	}
	for (size_t i = 0; i < 5; i++)
		technology_[i] = 0;
	for (size_t i = 0; i < 5; i++)
		soldier_ex_[i] = 0;
	for (size_t i = 0; i < 3; i++)
		interal_value_[i] = 0;
}

CMaster::~CMaster()
{

}
 
const CMaster* CMaster::CreateCustomMaster(const MasterInfoDat& info)
{
	CMaster* pMaster = new CMaster{ nullptr };

	pMaster->m_strName = info.name();
	pMaster->m_fResIncMul = info.ressource_override();

	for (size_t i = 0; i < 5; i++)
		pMaster->soldier_ex_[i] = 0;

	pMaster->interal_value_[0] = info.soldier_str();
	pMaster->interal_value_[1] = info.soldier_cmd();
	pMaster->interal_value_[2] = info.soldier_int();

	pMaster->technology_[0] = CAddition::GetSoldierAddition(info.soldier_atk_lv(), AdditionType::kAdditionTypeUnitAtk);
	pMaster->technology_[1] = CAddition::GetSoldierAddition(info.soldier_matk_lv(), AdditionType::kAdditionTypeUnitMAtk);
	pMaster->technology_[2] = CAddition::GetSoldierAddition(info.soldier_def_lv(), AdditionType::kAdditionTypeUnitDef);
	pMaster->technology_[3] = CAddition::GetSoldierAddition(info.soldier_mdef_lv(), AdditionType::kAdditionTypeUnitMDef);
	pMaster->technology_[4] = CAddition::GetSoldierAddition(info.soldier_hp_lv(), AdditionType::kAdditionTypeUnitHP);

	pMaster->m_arrSoldiers[0].m_nCardID = info.soldier_card1();
	{
		const CSoldierTP *pSoldierTP = CSoldierTP::GetSoldierTP(info.soldier_card1());
		if (pSoldierTP)
			pMaster->m_arrSoldiers[0].m_nCanUseNum = pSoldierTP->GetCanUseNumMax();
	}

	pMaster->m_arrSoldiers[1].m_nCardID = info.soldier_card2();
	{
		const CSoldierTP *pSoldierTP = CSoldierTP::GetSoldierTP(info.soldier_card2());
		if (pSoldierTP)
			pMaster->m_arrSoldiers[1].m_nCanUseNum = pSoldierTP->GetCanUseNumMax();
	}

	pMaster->m_arrSoldiers[2].m_nCardID = info.soldier_card3();
	{
		const CSoldierTP *pSoldierTP = CSoldierTP::GetSoldierTP(info.soldier_card3());
		if (pSoldierTP)
			pMaster->m_arrSoldiers[2].m_nCanUseNum = pSoldierTP->GetCanUseNumMax();
	}

	pMaster->m_arrSoldiers[3].m_nCardID = info.soldier_card4();
	{
		const CSoldierTP *pSoldierTP = CSoldierTP::GetSoldierTP(info.soldier_card4());
		if (pSoldierTP)
			pMaster->m_arrSoldiers[3].m_nCanUseNum = pSoldierTP->GetCanUseNumMax();
	}

	pMaster->m_arrSoldiers[4].m_nCardID = info.soldier_card5();
	{
		const CSoldierTP *pSoldierTP = CSoldierTP::GetSoldierTP(info.soldier_card5());
		if (pSoldierTP)
			pMaster->m_arrSoldiers[4].m_nCanUseNum = pSoldierTP->GetCanUseNumMax();
	}

	pMaster->m_arrSoldiers[5].m_nCardID = info.soldier_card6();
	{
		const CSoldierTP *pSoldierTP = CSoldierTP::GetSoldierTP(info.soldier_card6());
		if (pSoldierTP)
			pMaster->m_arrSoldiers[5].m_nCanUseNum = pSoldierTP->GetCanUseNumMax();
	}

	pMaster->m_arrHeroes[0].m_pHeroTP = CHeroTP::GetHeroTP(info.hero_card1());
	pMaster->m_arrHeroes[1].m_pHeroTP = CHeroTP::GetHeroTP(info.hero_card2());
	pMaster->m_arrHeroes[2].m_pHeroTP = CHeroTP::GetHeroTP(info.hero_card3());

	pMaster->m_nAIType = info.ai_type();
	if (info.has_population())
		pMaster->m_nPopulationMax = info.population();
	else
		pMaster->m_nPopulationMax = 12;

	pMaster->m_nPlayerLevel = info.lv();
	pMaster->m_bShow = info.show_npc();
	pMaster->m_nModelID = info.model_id();

	pMaster->castle_upgrade_[0] = int(((CInteriorLib::castle_atk_mul() * CUnitTP::GetBaseATK()) +
		(CInteriorLib::castle_atk_mul() * CAddition::GetSoldierAddition(info.lv(), AdditionType::kAdditionTypeUnitAtk))));
	pMaster->castle_upgrade_[1] = int(((CInteriorLib::castle_atk_mul() * CUnitTP::GetBaseATK()) +
		(CInteriorLib::castle_atk_mul() * CAddition::GetSoldierAddition(info.lv(), AdditionType::kAdditionTypeUnitMAtk))));

	pMaster->m_nResourceMax = 2000;

	return pMaster;
}

int	CMaster::GetHeroNum() const
{
	int nNum{ 0 };

	for (auto &it : m_arrHeroes)
	{
		if (it.m_pHeroTP)
			++nNum;
	}

	return nNum;
}

int	CMaster::GetCanUseHeroNum() const
{
	int nNum{ 0 };

	for (auto &it : m_arrHeroes)
	{
		if (it.m_pHeroTP && false == it.m_bUsed)
			++nNum;
	}

	return nNum;
}

int	CMaster::GetSoldierNum() const
{
	int nNum{ 0 };

	for (auto &it : m_arrSoldiers)
	{
		if (it.m_nCardID != 0)
			++nNum;
	}

	return nNum;
}

int	CMaster::GetMarkNum() const
{
	int nNum{ 0 };

	if (m_nMark & 1)
		nNum++;
	if (m_nMark & 2)
		nNum++;
	if (m_nMark & 4)
		nNum++;
	if (m_nMark & 8)
		nNum++;
	if (m_nMark & 16)
		nNum++;
	if (m_nMark & 32)
		nNum++;

	return nNum;
}

void CMaster::SetMasterByPto(const pto_BATTLE_STRUCT_Master_Ex* pPtoMaster, Force force)
{
	m_nPID = pPtoMaster->pid();
	m_nSID = pPtoMaster->sid();
	m_nPlayerLevel = pPtoMaster->level();
	m_strName = UTF8_to_ANSI(pPtoMaster->name());
	m_enForce = force;
	m_bSex = pPtoMaster->sex();
	title_id_ = pPtoMaster->title_id();
	m_nModelID = pPtoMaster->clothes();

	multi_stage_clear_ = pPtoMaster->multi_stage_clear();

	for (int i = 0; i < pPtoMaster->heroes_size() && i < MAX_HERO_NUM; i++)
	{
		auto itHero = pPtoMaster->heroes(i);

		m_arrHeroes[i].m_pHeroTP = CHeroTP::GetHeroTP(itHero.hero_id());

		m_arrHeroes[i].level_ = itHero.level();
		m_arrHeroes[i].color_ = itHero.color();
		m_arrHeroes[i].atk_ = itHero.atk();
		m_arrHeroes[i].matk_ = itHero.matk();
		m_arrHeroes[i].def_ = itHero.def();
		m_arrHeroes[i].mdef_ = itHero.mdef();
		m_arrHeroes[i].hp_ = itHero.hp();
		m_arrHeroes[i].move_speed_ = itHero.move_speed();
		m_arrHeroes[i].prev_atk_ = itHero.prev_atk();
		m_arrHeroes[i].atk_interval_ = itHero.atk_interval();
		m_arrHeroes[i].exsits_time_ = itHero.exsits_time() + m_arrHeroes[i].m_pHeroTP->GetPassSkillExTime(m_arrHeroes[i].level_);
		m_arrHeroes[i].crit_odds_ = itHero.crit_odds();
		m_arrHeroes[i].crit_value_ = itHero.crit_value();
		m_arrHeroes[i].crit_resistance_ = itHero.crit_resistance();
		m_arrHeroes[i].strength_ = itHero.strength();
		m_arrHeroes[i].command_ = itHero.command();
		m_arrHeroes[i].intelligence_ = itHero.intelligence();
		m_arrHeroes[i].building_atk_ = itHero.building_atk();
		m_arrHeroes[i].suit_effect_ = itHero.suit_effect();
	}

	for (int i = 0; i < pPtoMaster->using_soldiers_size() && i < MAX_SOLDIER_NUM; i++)
	{
		m_arrSoldiers[i].m_nCardID = pPtoMaster->using_soldiers(i);
		const CSoldierTP *pSoldierTP = CSoldierTP::GetSoldierTP(m_arrSoldiers[i].m_nCardID);
		if (!pSoldierTP) continue;	//未携带的卡牌ID， 客户端可能是脱机外挂
		m_arrSoldiers[i].m_nCanUseNum = pSoldierTP->GetCanUseNumMax();
	}

	for (int i = 0; i < pPtoMaster->technology_size() && i < 5; i++)
	{
		technology_[i] = pPtoMaster->technology(i);
	}

	for (int i = 0; i < pPtoMaster->soldeir_ex_size() && i < 5; i++)
	{
		soldier_ex_[i] = pPtoMaster->soldeir_ex(i);
	}

	castle_upgrade_[0] = pPtoMaster->castle_upgrade(0);
	castle_upgrade_[1] = pPtoMaster->castle_upgrade(1);
	castle_upgrade_[2] = pPtoMaster->castle_upgrade(2);

	for (int i = 0; i < pPtoMaster->soldiers_size(); i++)
	{
		auto itSoldier = pPtoMaster->soldiers(i);

		const CSoldierTP *pSoldierTP = CSoldierTP::GetSoldierTP(itSoldier.soldier_id());
		if (!pSoldierTP) continue;	//未携带的卡牌ID， 客户端可能是脱机外挂
		SoldierSoul soldier_soul;
		soldier_soul.soldier_id_ = itSoldier.soldier_id();
		soldier_soul.soul_exp_ = itSoldier.soul_exp();
		soldier_soul.soul_level_ = itSoldier.soul_level();
		soldier_soul.train_level_ = itSoldier.train_level();

		soldiers_soul_.push_back(soldier_soul);
	}
}

void CMaster::SetArenaAI(BattleMode enMode)
{
	m_nPID = 0;
	m_nSID = 0;
	m_nPlayerLevel = 15;
	m_enForce = Force::DEF;
	m_bSex = true;
	m_strName = UTF8_to_ANSI(CNameLibrary::GetRandomName(m_bSex));
	m_bIsAI = true;
	m_nAIType = 3;
	m_fResIncMul = 0.6f;

	if (BattleMode::ARENA3v3Easy != enMode)
	{
		SetHeroCard(8, 0);
		SetHeroCard(2, 1);

		if (BattleMode::ARENA1v1Normal == enMode)
			SetHeroCard(3, 2);
	}

	if (BattleMode::ARENA3v3Easy == enMode || BattleMode::ARENA3v3Normal == enMode)
	{
		int group = GetRandom(1, 5);
		int hero_1{ 0 };
		int hero_2{ 0 };
		int hero_3{ 0 };

		switch (group)
		{
		case 1:
			hero_1 = 8;
			hero_2 = 2;
			hero_3 = 3;
			break;
		case 2:
			hero_1 = 2;
			hero_2 = 14;
			hero_3 = 3;
			break;
		case 3:
			hero_1 = 2;
			hero_2 = 8;
			hero_3 = 10;
			break;
		case 4:
			hero_1 = 10;
			hero_2 = 2;
			hero_3 = 3;
			break;
		case 5:
			hero_1 = 10;
			hero_2 = 14;
			hero_3 = 8;
			break;
		default:
			break;
		}
		SetHeroCard(hero_1, 0);
		SetHeroCard(hero_2, 1);
		SetHeroCard(hero_3, 2);
	}

	m_arrSoldiers[0].m_nCardID = 1;
	const CSoldierTP *pSoldierTP0 = CSoldierTP::GetSoldierTP(m_arrSoldiers[0].m_nCardID);
	if (pSoldierTP0)
		m_arrSoldiers[0].m_nCanUseNum = pSoldierTP0->GetCanUseNumMax();

	m_arrSoldiers[1].m_nCardID = 2;
	const CSoldierTP *pSoldierTP1 = CSoldierTP::GetSoldierTP(m_arrSoldiers[1].m_nCardID);
	if (pSoldierTP1)
		m_arrSoldiers[1].m_nCanUseNum = pSoldierTP1->GetCanUseNumMax();

	m_arrSoldiers[2].m_nCardID = 3;
	const CSoldierTP *pSoldierTP2 = CSoldierTP::GetSoldierTP(m_arrSoldiers[2].m_nCardID);
	if (pSoldierTP2)
		m_arrSoldiers[2].m_nCanUseNum = pSoldierTP2->GetCanUseNumMax();

	m_arrSoldiers[3].m_nCardID = 4;
	const CSoldierTP *pSoldierTP3 = CSoldierTP::GetSoldierTP(m_arrSoldiers[3].m_nCardID);
	if (pSoldierTP3)
		m_arrSoldiers[3].m_nCanUseNum = pSoldierTP3->GetCanUseNumMax();

	m_arrSoldiers[4].m_nCardID = 12;
	const CSoldierTP *pSoldierTP4 = CSoldierTP::GetSoldierTP(m_arrSoldiers[4].m_nCardID);
	if (pSoldierTP4)
		m_arrSoldiers[4].m_nCanUseNum = pSoldierTP4->GetCanUseNumMax();

	if (BattleMode::ARENA3v3Easy == enMode || BattleMode::ARENA3v3Normal == enMode)
	{
		m_arrSoldiers[5].m_nCardID = 5;
		const CSoldierTP *pSoldierTP5 = CSoldierTP::GetSoldierTP(m_arrSoldiers[5].m_nCardID);
		if (pSoldierTP5)
			m_arrSoldiers[5].m_nCanUseNum = pSoldierTP5->GetCanUseNumMax();
	}

	technology_[0] = CAddition::fair_soldier_atk();
	technology_[1] = CAddition::fair_soldier_matk();
	technology_[2] = CAddition::fair_soldier_def();
	technology_[3] = CAddition::fair_soldier_mdef();
	technology_[4] = CAddition::fair_soldier_hp();
	castle_upgrade_[0] = castle_upgrade_[1] = (int((CInteriorLib::castle_atk_mul() * CUnitTP::GetBaseATK()) + (CInteriorLib::castle_atk_mul() * CAddition::GetSoldierAddition(50, AdditionType::kAdditionTypeUnitAtk))));
	castle_upgrade_[2] = 0;
	interal_value_[0] = interal_value_[1] = interal_value_[2] = 150;
	m_nPopulationMax = 12;
}

void CMaster::SetHeroCard(int hero_id, int index) //AI英雄
{
	if (0 > index || 2 < index)
		return;

	HeroCard* card = &m_arrHeroes[index];
	const CHeroTP* tp = CHeroTP::GetHeroTP(hero_id);

	if (!tp)
		return;

	float hp_mul = CHeroTP::GetHeroTypeAddition(tp->GetType(), 0);
	float atk_mul = CHeroTP::GetHeroTypeAddition(tp->GetType(), 1);
	float def_mul = CHeroTP::GetHeroTypeAddition(tp->GetType(), 2);
	float matk_mul = CHeroTP::GetHeroTypeAddition(tp->GetType(), 3);
	float mdef_mul = CHeroTP::GetHeroTypeAddition(tp->GetType(), 4);

	card->m_pHeroTP = tp;

	card->level_ = 20;
	card->color_ = 1;
	card->atk_ = static_cast<int>((CHeroTP::GetBaseATK() + CAddition::fair_equip_atk()) * atk_mul);
	card->matk_ = static_cast<int>((CHeroTP::GetBaseMATK() + CAddition::fair_equip_atk()) * matk_mul);
	card->def_ = static_cast<int>((CHeroTP::GetBaseDEF() + CAddition::fair_equip_def()) * def_mul);
	card->mdef_ = static_cast<int>((CHeroTP::GetBaseMDEF() + CAddition::fair_equip_def()) * mdef_mul);
	card->hp_ = static_cast<int>((CHeroTP::GetBaseHP() + CAddition::fair_equip_hp()) * hp_mul);
	card->move_speed_ = tp->move_speed();
	card->prev_atk_ = tp->atk_prev();
	card->atk_interval_ = tp->atk_interval();
	card->exsits_time_ = tp->GetPassSkillExTime(card->level_) + 6000;
	card->crit_odds_ = tp->crit();
	card->crit_value_ = tp->crit_value();
	card->crit_resistance_ = 0;
	card->strength_ = 150;
	card->command_ = 150;
	card->intelligence_ = 150;
	card->building_atk_ = tp->GetAtkBuildingDamage();
	card->suit_effect_ = 0;
	card->exsits_time_ = 60000;
}

void CMaster::SetProtocol(pto_BATTLE_Struct_Master *pPtoMaster)
{
	pPtoMaster->set_nid(m_nID);
	pPtoMaster->set_szname(ANSI_to_UTF8(m_strName));
	pPtoMaster->set_enforce(m_enForce);
	pPtoMaster->set_nplayerid(m_nPID);
	pPtoMaster->set_nlv(military_level_);
	pPtoMaster->set_ncanuseherocounts(GetCanUseHeroNum());
	pPtoMaster->set_bheroactive(false);
	pPtoMaster->set_nheroexisttimemax(0);
	pPtoMaster->set_nheroexisttime(0);
	pPtoMaster->set_nherocardid(0);
	pPtoMaster->set_nherounitid(0);
	pPtoMaster->set_nresourcemax(m_nResourceMax);
	pPtoMaster->set_npopulationmax(m_nPopulationMax);
	pPtoMaster->set_bisai(m_bIsAI);
	pPtoMaster->set_sex(m_bSex);
	pPtoMaster->set_bisshow(m_bShow);
	pPtoMaster->set_model_id(m_nModelID);
	pPtoMaster->set_nplayerlv(m_nPlayerLevel);
	pPtoMaster->set_nservid(m_nSID);
	pPtoMaster->set_title_id(title_id_);

	for (auto &it : m_arrHeroes)
	{
		if (it.m_pHeroTP)
		{
			auto ptoHero = pPtoMaster->add_vctherolist();
			ptoHero->set_suit_id(it.suit_effect_);
			it.m_pHeroTP->SetProtocol(ptoHero, it.color_, it.level_);

			auto ptoCard = ptoHero->mutable_data();

			ptoCard->set_natk(it.atk_);
			ptoCard->set_ndef(it.def_);
			ptoCard->set_nmatk(it.matk_);
			ptoCard->set_nmdef(it.mdef_);
			ptoCard->set_nmaxhp(it.hp_);

			ptoHero->set_nheroexisttimemax(it.exsits_time_);
			ptoCard->set_fcrit(it.crit_odds_);
			ptoCard->set_fmovespeed(static_cast<float>(it.move_speed_));
			ptoCard->set_nattackintervaltime(it.atk_interval_);
			ptoCard->set_nattackpretime(it.prev_atk_);

			ptoCard->set_nstr(it.strength_);
			ptoCard->set_ncmd(it.command_);
			ptoCard->set_nint(it.intelligence_);
		}
	}
	for (auto &it : m_arrSoldiers)
	{
		const CSoldierTP* pSoldierTP = CSoldierTP::GetSoldierTP(it.m_nCardID);
		if (pSoldierTP)
		{
			auto ptoSoldier = pPtoMaster->add_vctsoldierlist();
			pSoldierTP->SetProtocol(ptoSoldier);

			auto ptoCard = ptoSoldier->mutable_data();
			ptoCard->set_natk(int((CSoldierTP::GetBaseATK() + technology_[0] + soldier_ex_[0]) * pSoldierTP->GetATKMul()));
			ptoCard->set_nmatk(int((CSoldierTP::GetBaseMATK() + technology_[1] + soldier_ex_[1]) * pSoldierTP->GetMATKMul()));
			ptoCard->set_ndef(int((CSoldierTP::GetBaseDEF() + technology_[2] + soldier_ex_[2]) * pSoldierTP->GetDEFMul()));
			ptoCard->set_nmdef(int((CSoldierTP::GetBaseMDEF() + technology_[3] + soldier_ex_[3]) * pSoldierTP->GetMDEFMul()));
			ptoCard->set_nmaxhp(int((CSoldierTP::GetBaseHP() + technology_[4] + soldier_ex_[4]) * pSoldierTP->GetHPMul()));
		}
	}
}

void CMaster::Loop()
{
	if (using_base_gun_)
		BaseGunTimer();

	if (m_bIsLeave)
		return;

	if (m_bIsAI)
		__AILoop();

	if (m_pBattle->TimeCounts(1500))
		__IncreaseResources();

	__CalculateHeroExistTime();
	__CalculateSoldierCD();
}

bool CMaster::ClickHeroCard(int nIndex, float fPos)
{
	if (m_bHeroLock)
		return false;

	if (nIndex >= GetHeroNum())
		return false;

	HeroCard* pHeroCard = &m_arrHeroes[nIndex];

	int nResult{ 0 };

	if (!pHeroCard)						//未携带的卡牌ID， 客户端可能是脱机外挂
		nResult = 2;
	else if (pHeroCard->m_bUsed)		//已经是用过
		nResult = 3;
	else if (m_pUsingHero)				//已经有一个武将在场
		nResult = 4;

	pto_BATTLE_S2C_RES_ClickHeroCard pto;
	pto.set_nresult(nResult);
	pto.set_ncardindex(nIndex);
	std::string strPto;
	pto.SerializeToString(&strPto);
	m_pBattle->SendToMaster(this, strPto, MSG_S2C, BATTLE_S2C_RES_ClickHeroCard);

	if (nResult)
		return false;

	pHeroCard->m_bUsed = true;
	m_nHeroDurationTime = 0;

	m_pUsingHero = new CHero{ m_pBattle };
	m_pUsingHero->InitHero(pHeroCard, this);
	const CSuitTP* suit = CSuitTP::GetSuitTP(pHeroCard->suit_effect_);
	if (suit)
		m_pUsingHero->SetSuitSkill(suit->skill_id());

	float fHitPos = m_pBattle->FindSoldiersLinePos(m_enForce, false, true, true);
	if (Force::ATK == m_enForce)
		fHitPos = fPos < fHitPos ? fPos : fHitPos;
	else if (Force::DEF == m_enForce)
		fHitPos = fPos > fHitPos ? fPos : fHitPos;
	m_pUsingHero->SetPos(fHitPos);
	m_pUsingHero->SetIsAI(m_bIsAI);
	m_pUsingHero->SetPosIndex(nIndex);

	if (m_pBattle->AddObject(m_pUsingHero))
	{
		pto_BATTLE_Struct_ClickHeroCard ptoClick;
		ptoClick.set_nmaster(m_nID);
		ptoClick.set_ncardindex(nIndex);
		ptoClick.set_nunitid(m_pUsingHero->GetID());
		ptoClick.set_nrank(pHeroCard->color_);
		ptoClick.set_suit_id(pHeroCard->suit_effect_);

		m_pBattle->GetProtoPackage(Force::ATK)->ClickHeroCard(&ptoClick);
		m_pBattle->GetProtoPackage(Force::DEF)->ClickHeroCard(&ptoClick);

		m_pUsingHero->SetBuff(1, nullptr);

		return true;
	}
	else
	{
		RECORD_WARNING("插入英雄失败");
		delete m_pUsingHero;

		return false;
	}
}

bool CMaster::ClickSoldierCard(int nIndex, int fPos)
{
	if (m_bSoldierLock)
		return false;

	if ((size_t)nIndex >= m_arrSoldiers.size())
		return false;

	int nSoldierID = m_arrSoldiers[nIndex].m_nCardID;

	const CSoldierTP *pSoldierTP = CSoldierTP::GetSoldierTP(nSoldierID);
	float fHitPos = m_pBattle->FindSoldiersLinePos(m_enForce, false, true, true);

	int nResult{ 0 };

	if (!pSoldierTP)																				//未携带的卡牌ID， 客户端可能是脱机外挂
		nResult = 2;
	else if (pSoldierTP->GetCanUseNumMax() != 0 && m_arrSoldiers.at(nIndex).m_nCanUseNum == 0)		//卡牌可使用数量不足
		nResult = 4;
	else if (m_arrSoldiers[nIndex].m_nCoolDown > 0)													//CD未到错误，客户端可能受到加速外挂影响
		nResult = 1;
	else if (m_nResource < pSoldierTP->GetPrice())													//资源不足
		nResult = 3;
	else if (m_nPopulation + pSoldierTP->GetPopulation() > m_nPopulationMax)						//人口不足 在人口数字上亮红光
		nResult = 5;
	else if (pSoldierTP->GetType() == SOLDIER_TYPE::STRATEGY && !pSoldierTP->cross_line() && Force::ATK == m_enForce && fPos > fHitPos)				//超出释放范围
		nResult = 6;
	else if (pSoldierTP->GetType() == SOLDIER_TYPE::STRATEGY && !pSoldierTP->cross_line() && Force::DEF == m_enForce && fPos < fHitPos)				//超出释放范围
		nResult = 6;

	pto_BATTLE_S2C_RES_ClickSoldierCard ptoClickSoldierCard;
	ptoClickSoldierCard.set_ncardindex(nIndex);
	ptoClickSoldierCard.set_nresult(nResult);
	std::string str;
	ptoClickSoldierCard.SerializeToString(&str);
	m_pBattle->SendToMaster(this, str, MSG_S2C, BATTLE_S2C_RES_ClickSoldierCard);

	if (nResult)
		return false;

	if (pSoldierTP->GetCanUseNumMax() != 0)
	{
		if (!IsAI())
			m_arrSoldiers[nIndex].m_nCanUseNum--;
		if (IsAI()
			&& m_pBattle->GetBattleMode() != BattleMode::ELITESTAGE
			&& m_pBattle->GetBattleMode() != BattleMode::SPEED
			&& m_pBattle->GetBattleMode() != BattleMode::MULTISTAGE
			&& m_pBattle->GetBattleMode() != BattleMode::STAGE)
			m_arrSoldiers[nIndex].m_nCanUseNum--;
	}

	m_nAllHitSoldier++;
	m_arrSoldiers[nIndex].m_nCoolDown = pSoldierTP->GetCD();

	if (pSoldierTP->GetType() == SOLDIER_TYPE::STRATEGY)
	{
		if (UseStrategySoldier(pSoldierTP, fPos))
		{
			ChangeResource(-pSoldierTP->GetPrice());
			return true;
		}
		else
		{
			RECORD_WARNING("使用策略失败");
			return false;
		}
	}

	else
	{
		CUnit *pUnit = new CUnit{ m_pBattle };
		pUnit->InitUnit(pSoldierTP, this);
		pUnit->SetPosIndex(nIndex);
		pUnit->SetPos(m_pBattle->GetWarpGate(m_enForce));

		if (m_pBattle->AddObject(pUnit))
		{
			ChangeResource(-pSoldierTP->GetPrice());
			ChangePopulation(pSoldierTP->GetPopulation());
			pUnit->InitBrithBuff(pSoldierTP);

			return true;
		}
		else
		{
			RECORD_WARNING("插入单位失败");
			delete pUnit;

			return false;
		}
	}
}

void CMaster::UpgradeLevel()
{
	if (military_level_ >= 5)
		return;

	int nNeedResource = m_nResourceMax / 2;
	//switch (military_level_)
	//{
	//case 0:nNeedResource = 500; break;
	//case 1:nNeedResource = 750; break;
	//case 2:nNeedResource = 1000; break;
	//case 3:nNeedResource = 1250; break;
	//case 4:nNeedResource = 1500; break;
	//default:
	//	break;
	//}

	if (m_nResource < nNeedResource)
		return;

	military_level_++;
	economy_level_++;

	m_nResourceMax = 1000 + (economy_level_ * 250);
	//switch (m_nLV)
	//{
	//case 1:m_nResourceMax = 1000; break;
	//case 2:m_nResourceMax = 1500; break;
	//case 3:m_nResourceMax = 2000; break;
	//case 4:m_nResourceMax = 2500; break;
	//case 5:m_nResourceMax = 3000; break;
	//default:m_nResourceMax = 1500; break;
	//}

	//m_nResIncBase = 30 /*+ (economy_level_ * 3)*/;
	/*switch (m_nLV)
	{
	case 1:m_nResIncBase = 30; break;
	case 2:m_nResIncBase = 40; break;
	case 3:m_nResIncBase = 50; break;
	default:m_nResIncBase = 0; break;
	}*/

	pto_BATTLE_S2C_NTF_Upgradelevel  ptoUpgradeLevel;
	ptoUpgradeLevel.set_nmasterid(m_nID);
	ptoUpgradeLevel.set_nlv(military_level_);
	ptoUpgradeLevel.set_nresourcemax(m_nResourceMax);
	std::string strPto;
	ptoUpgradeLevel.SerializeToString(&strPto);
	m_pBattle->SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_Upgradelevel);

	ChangeResource(-nNeedResource);

	m_nPopulationMax += POPULATION_INCREASE_BASE;
	pto_BATTLE_S2C_NTF_IncreasePopulationMax ptoPopulaion;
	ptoPopulaion.set_nmaxnum(m_nPopulationMax);
	strPto.clear();
	ptoPopulaion.SerializeToString(&strPto);
	m_pBattle->SendToMaster(this, strPto, MSG_S2C, BATTLE_S2C_NTF_IncreasePopulationMax);
}

void CMaster::ChangeLevel(int level)
{
	military_level_ = level;
	economy_level_ = level;
	m_nResourceMax = 1000 + (economy_level_ * 250);

	pto_BATTLE_S2C_NTF_Upgradelevel  ptoUpgradeLevel;
	ptoUpgradeLevel.set_nmasterid(m_nID);
	ptoUpgradeLevel.set_nlv(military_level_);
	ptoUpgradeLevel.set_nresourcemax(m_nResourceMax);
	std::string strPto;
	ptoUpgradeLevel.SerializeToString(&strPto);
	m_pBattle->SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_Upgradelevel);

	m_nPopulationMax = POPULATION_INCREASE_BASE * military_level_ + MAX_POPULATION_BASE;
	pto_BATTLE_S2C_NTF_IncreasePopulationMax ptoPopulaion;
	ptoPopulaion.set_nmaxnum(m_nPopulationMax);
	strPto.clear();
	ptoPopulaion.SerializeToString(&strPto);
	m_pBattle->SendToMaster(this, strPto, MSG_S2C, BATTLE_S2C_NTF_IncreasePopulationMax);
}

void CMaster::ChangeResource(int nNum)
{
	m_nResource += nNum;

	if (m_nResource > m_nResourceMax)
		m_nResource = m_nResourceMax;
	if (m_nResource < 0)
		m_nResource = 0;

	if (nNum >= 0)
	{
		m_nAllResource += nNum;

		pto_BATTLE_S2C_NTF_IncreaseResource ptoIncreaseRes;
		ptoIncreaseRes.set_nmyresource(m_nResource);
		ptoIncreaseRes.set_ninc(abs(nNum));
		std::string strPto;
		ptoIncreaseRes.SerializeToString(&strPto);
		m_pBattle->SendToMaster(this, strPto, MSG_S2C, BATTLE_S2C_NTF_IncreaseResource);
	}
	else
	{
		pto_BATTLE_S2C_NTF_ReduceResource ptoReduceResource;
		ptoReduceResource.set_nmyresource(m_nResource);
		ptoReduceResource.set_nreduce(abs(nNum));
		std::string strPto;
		ptoReduceResource.SerializeToString(&strPto);
		m_pBattle->SendToMaster(this, strPto, MSG_S2C, BATTLE_S2C_NTF_ReduceResource);
	}
}

void CMaster::ChangePopulation(int nNum)
{
	m_nPopulation += nNum;

	if (m_nPopulation > m_nPopulationMax)
		m_nPopulation = m_nPopulationMax;
	if (m_nPopulation < 0)
		m_nPopulation = 0;

	pto_BATTLE_S2C_NTF_SendPopulation pto;
	pto.set_nnownum(m_nPopulation);
	std::string strPto;
	pto.SerializeToString(&strPto);
	m_pBattle->SendToMaster(this, strPto, MSG_S2C, BATTLE_S2C_NTF_SendPopulation);
}

void CMaster::ClearSoldierCardCD(int nIndex /*= -1*/)
{
	if (nIndex < 0)
	{
		for (auto &it : m_arrSoldiers)
			it.m_nCoolDown = 0;
	}
	else if (nIndex < (int)m_arrSoldiers.size())
	{
		m_arrSoldiers[nIndex].m_nCoolDown = 0;
	}
}

void CMaster::ResetHero()
{
	for (auto &it : m_arrHeroes)
		it.m_bUsed = false;
	m_pBattle->SendToMaster(this, "", MSG_S2C, BATTLE_S2C_NTF_ResetHero);
}

bool CMaster::HeroUnavailable()
{
	if (nullptr != m_pUsingHero)
		return false;

	for (auto &it : m_arrHeroes)
		if (false == it.m_bUsed && nullptr != it.m_pHeroTP)
			return false;

	return true;
}

CMaster* CMaster::Clone(CBattle *pBattle) const
{
	CMaster *pMaster = new CMaster(*this);
	pMaster->m_pBattle = pBattle;

	return pMaster;
}

bool CMaster::UsingHero()
{
	if (m_pUsingHero == nullptr)
		return false;
	return true;
}

bool CMaster::UseSkill(const CSkill* pSkill, CHero* pHero, float& nUseSkillPos)
{
	if (!pSkill)
		return false;

	bool bResult = false;
	float fTempPos = -1;

	switch (pSkill->GetAIRule())
	{
	case 1://技能范围内有人就放
		fTempPos = m_pBattle->FindSkillSoldierLinePos(pHero);
		if (Force::ATK == pHero->GetForce() &&
			fTempPos > 0 &&
			pHero->GetPos() + __GetSkillRange(pSkill) >= fTempPos)
		{
			nUseSkillPos = fTempPos;
			bResult = true;
		}
		else if (Force::DEF == pHero->GetForce() &&
			fTempPos > 0 &&
			pHero->GetPos() - __GetSkillRange(pSkill) <= fTempPos)
		{
			nUseSkillPos = fTempPos;
			bResult = true;
		}
		break;

	case 2://自己血量低于80%就放(治疗)
		if (pHero->GetHP() < (0.8*pHero->GetMaxHP()))
			bResult = true;
		break;

	case 3://自己受到攻击就放(金刚护体之类)
		if (pHero->GetBeDamaged())
			bResult = true;
		break;

	case 4://技能范围内有英雄就放(类似黄忠那种打英雄的技能)
		fTempPos = m_pBattle->FindSkillHeroLinePos(pHero);
		if (Force::ATK == pHero->GetForce() &&
			fTempPos > 0 &&
			pHero->GetPos() + __GetSkillRange(pSkill) >= fTempPos)
		{
			nUseSkillPos = fTempPos;
			bResult = true;
		}

		if (Force::DEF == pHero->GetForce() &&
			fTempPos > 0 &&
			pHero->GetPos() - __GetSkillRange(pSkill) <= fTempPos)
		{
			nUseSkillPos = fTempPos;
			bResult = true;
		}
		break;

	case 5://敌人使用引导技能时就放(打断技能)
		nUseSkillPos = m_pBattle->BreakHeroSkill(__GetSkillRange(pSkill), pHero);
		if (nUseSkillPos != -1)
		{
			bResult = true;
		}
		break;

	case 6://对着建筑放
		fTempPos = m_pBattle->FindSkillBulidingLinePos(pHero);
		if (Force::ATK == pHero->GetForce() &&
			fTempPos > 0 &&
			pHero->GetPos() + __GetSkillRange(pSkill) >= fTempPos)
		{
			nUseSkillPos = fTempPos;
			bResult = true;
		}

		if (Force::DEF == pHero->GetForce() &&
			fTempPos > 0 &&
			pHero->GetPos() - __GetSkillRange(pSkill) <= fTempPos)
		{
			nUseSkillPos = fTempPos;
			bResult = true;
		}
		break;

		//	case 7://位移(就跳最近的目标)
		//	break;

	case 8://技能CD一好就放(获得资源类)
		bResult = true;
		break;

	case 9://士兵CD超过10秒就放(清士兵CD)
		for (auto it : m_arrSoldiers)
		{
			if (it.m_nCoolDown > 10)
				bResult = true;
		}
		break;

	case 10://范围内超过3个敌人就发动
		if (3 <= __GetEnemyInRange(pSkill, pHero))
		{
			nUseSkillPos = m_pBattle->FindSkillHeroLinePos(pHero);
			bResult = true;
		}
		break;
	default:
		break;
	}

	if (!pSkill->IsNeedClick())
	{
		nUseSkillPos = pHero->GetPos();
	}
	//if (Force::DEF == pHero->GetForce() ||
	//	nUseSkillPos < pHero->GetPos() - pSkill->GetCastRange())
	//	nUseSkillPos = pHero->GetPos() - pSkill->GetCastRange();
	//else if (Force::ATK == pHero->GetForce() ||
	//	nUseSkillPos > pHero->GetPos() + pSkill->GetCastRange())
	//	nUseSkillPos = pHero->GetPos() + pSkill->GetCastRange();

	return bResult;
}

void CMaster::__IncreaseResources()
{
	if (IsAI() && m_nPopulation >= m_nPopulationMax)
		return;

	int nPlayerNum = m_pBattle->GetMasterNum(m_enForce, false, false);
	int nResIncBase = static_cast<int>(((m_nResIncBase + 5 * nPlayerNum) * m_fResIncMul) / nPlayerNum);

	if (IsAI())
		nResIncBase = int((RESOURCE_INCREASE_BASE + 5) * m_fResIncMul);

	nResIncBase += __HeroPassSkillAddResources();

	m_nResource += nResIncBase;

	if (m_nResource > m_nResourceMax)
		m_nResource = m_nResourceMax;
	if (m_nResource < 0)
		m_nResource = 0;

	m_nAllResource += nResIncBase;

	pto_BATTLE_S2C_NTF_IncreaseResource ptoIncreaseRes;
	ptoIncreaseRes.set_nmyresource(m_nResource);
	ptoIncreaseRes.set_ninc(nResIncBase);
	std::string str;
	ptoIncreaseRes.SerializeToString(&str);

	m_pBattle->SendToMaster(this, str, MSG_S2C, BATTLE_S2C_NTF_IncreaseResource);
}

void CMaster::__CalculateHeroExistTime()
{
	if (!m_pUsingHero)
		return;

	m_nHeroDurationTime += BATTLE_LOOP_TIME;

	pto_BATTLE_S2C_NTF_HeroExsitTime ptoHeroExsitTime;
	ptoHeroExsitTime.set_nmasterid(m_nID);
	ptoHeroExsitTime.set_nexsittime(m_nHeroDurationTime);
	std::string str;
	ptoHeroExsitTime.SerializeToString(&str);

	if (m_pUsingHero->GetMaxTime() != 0 && m_nHeroDurationTime >= m_pUsingHero->GetMaxTime())
	{
		m_pBattle->SendToAllMaster(str, MSG_S2C, BATTLE_S2C_NTF_HeroExsitTime);

		m_pUsingHero->Disappear();

		m_nHeroDurationTime = 0;
		m_pUsingHero = nullptr;
	}
	else if (m_pUsingHero->GetMaxTime() != 0)
	{
		if (false == m_pUsingHero->IsBoss() && m_pBattle->TimeCounts(500))
			m_pBattle->SendToAllMaster(str, MSG_S2C, BATTLE_S2C_NTF_HeroExsitTime);

		if (m_pBattle->TimeCounts(500) && m_pUsingHero->GetMaxHP() > m_pUsingHero->GetHP() && false == m_pUsingHero->IsBoss())
		{
			if (0 >= m_pUsingHero->IsOutCombat())
				m_pUsingHero->Heal((int)(m_pUsingHero->GetMaxHP() * m_pUsingHero->GetAutoHeal() / 10));
			else
				m_pUsingHero->SetOutCombat(m_pUsingHero->IsOutCombat() - 500);
		}	
	}
}

void CMaster::__CalculateSoldierCD()
{
	for (auto &it : m_arrSoldiers)
	{
		it.m_nCoolDown -= BATTLE_LOOP_TIME;
		if (it.m_nCoolDown < 0)
			it.m_nCoolDown = 0;
	}
}

void CMaster::__AILoop()
{
	if (m_pBattle->TimeCounts(500))
	{
		switch (m_nAIType)
		{
		case 1:__AI1(); break;
		case 2:__AI2(); break;
		case 3:__AI3(); break;
		}

		/*	if (m_pUsingHero != nullptr)
			{
			for (size_t i = 0; i < m_pUsingHero->GetSkills()->size(); i++)
			{
			if (m_pUsingHero->GetSkills()->at(i).second>0)
			continue;

			float fUseSkillPos = m_pUsingHero->GetPos();

			if (UseSkill(m_pUsingHero->GetSkills()->at(i).first, m_pUsingHero, fUseSkillPos))
			{
			m_pUsingHero->UseSkill(i, m_pUsingHero->GetPos(), fUseSkillPos);
			return;
			}
			}
			m_pUsingHero->SetBeDamaged(false);
			}*/

	}
}

void CMaster::__AI1()
{
	ClickSoldierCard(0, 0);
}

void CMaster::__AI2()
{
	bool base_gun{ false };
	if (m_enForce == Force::ATK)
		base_gun = m_pBattle->atk_base_gun();
	else if (m_enForce == Force::DEF)
		base_gun = m_pBattle->def_base_gun();

	if (base_gun)
	{
		if (remain_base_gun_ >= 2 && m_pBattle->DefCastleHP(0.6f) && m_nID == 4)
		{
			remain_base_gun_--;
			BaseGunActivate();
		}

		//if (remain_base_gun_ >= 2 && m_pBattle->PrisonHP(Force::ATK, 0.6f) && m_nID == 4)
		//{
		//	remain_base_gun_--;
		//	BaseGunActivate();
		//}

		if (remain_base_gun_ >= 1 && m_pBattle->DefCastleHP(0.4f) && m_nID == 4)
		{
			remain_base_gun_--;
			BaseGunActivate();
		}
	}

	//if (remain_base_gun_ >= 1 && m_pBattle->PrisonHP(Force::ATK, 0.4f) && m_nID == 4)
	//{
	//	remain_base_gun_--;
	//	BaseGunActivate();
	//}

	if (m_nNextClickSoldierIndex >= 0)
	{
		if (ClickSoldierCard(m_nNextClickSoldierIndex, 0))
			m_nNextClickSoldierIndex = -1;

		return;
	}

	std::vector<int> vctCloseSoldiers;
	std::vector<int> vctRangeSoldiers;
	int nCloseSoldier{ -1 }, nRangeSoldier{ -1 };

	for (size_t i = 0; i < m_arrSoldiers.size(); i++)
	{
		if (m_arrSoldiers[i].m_nCoolDown)
			continue;

		const CSoldierTP* pSoldierTP = CSoldierTP::GetSoldierTP(m_arrSoldiers[i].m_nCardID);
		if (!pSoldierTP)
			continue;

		if (pSoldierTP->GetCanUseNumMax() != 0 && m_arrSoldiers[i].m_nCanUseNum == 0)
			continue;

		if (pSoldierTP->IsRange())
			vctRangeSoldiers.push_back(i);
		else
			vctCloseSoldiers.push_back(i);
	}

	if (false == vctCloseSoldiers.empty())
		nCloseSoldier = vctCloseSoldiers[GetRandom((unsigned)0, vctCloseSoldiers.size() - 1)];
	if (false == vctRangeSoldiers.empty())
		nRangeSoldier = vctRangeSoldiers[GetRandom((unsigned)0, vctRangeSoldiers.size() - 1)];

	if (-1 != nCloseSoldier && -1 != nRangeSoldier)
	{
		if (m_pBattle->GetUnitNum(m_enForce) < 3)
			m_nNextClickSoldierIndex = nCloseSoldier;
		else if (m_pBattle->GetRangeUnitRatio(m_enForce) > 0.4f)
			m_nNextClickSoldierIndex = nCloseSoldier;
		else
			m_nNextClickSoldierIndex = nRangeSoldier;
	}
	else if (-1 != nCloseSoldier)
	{
		m_nNextClickSoldierIndex = nCloseSoldier;
	}
	else if (-1 != nRangeSoldier)
	{
		m_nNextClickSoldierIndex = nRangeSoldier;
	}
}

void CMaster::__AI3()
{
	__AI2();
	int nUnit = 0;
	Force enEnemyForce;
	if (m_enForce == Force::ATK)
	{
		nUnit = m_pBattle->GetDefUnitNum() - m_pBattle->GetAtkUnitNum();
		enEnemyForce = Force::DEF;
	}
	else if (m_enForce == Force::DEF)
	{
		nUnit = m_pBattle->GetAtkUnitNum() - m_pBattle->GetDefUnitNum();
		enEnemyForce = Force::ATK;
	}

	if (__EnemyUsingHero() || nUnit >= 5 || m_pBattle->DefCastleHP(0.9f))
		__AIClickHeroCard();

	if (m_pUsingHero)
	{
		float fEnemyHeroPos = m_pBattle->FindHeroLinePos(enEnemyForce);
		if (m_enForce == Force::ATK &&
			fEnemyHeroPos < m_pUsingHero->GetPos())
		{
			m_pUsingHero->Retreat();
		}
		else if (m_enForce == Force::ATK &&
			fEnemyHeroPos >= m_pUsingHero->GetPos() &&
			m_pUsingHero->GetAIType() == AIType::AI_RETREAT)
		{
			m_pUsingHero->StopRetreat();
		}
		else if (m_enForce == Force::DEF &&
			fEnemyHeroPos > m_pUsingHero->GetPos())
		{
			m_pUsingHero->Retreat();
		}
		else if (m_enForce == Force::DEF &&
			fEnemyHeroPos <= m_pUsingHero->GetPos() &&
			m_pUsingHero->GetAIType() == AIType::AI_RETREAT)
		{
			m_pUsingHero->StopRetreat();
		}
	}

}

void CMaster::__SetFairMasterValue()
{
	/*m_nEquip[0] = CAdditionLibrary::__CalculateCustomMasterEquipAtk(50);
	m_nEquip[1] = CAdditionLibrary::__CalculateCustomMasterEquipAtk(50);
	m_nEquip[2] = CAdditionLibrary::__CalculateCustomMasterEquipDef(50);
	m_nEquip[3] = CAdditionLibrary::__CalculateCustomMasterEquipDef(50);
	m_nEquip[4] = CAdditionLibrary::__CalculateCustomMasterEquipHP(50);

	m_nTechnology[0] = CAdditionLibrary::__CalculateSoldierAddition(50, SOLDIER_TECH::ATK);
	m_nTechnology[1] = CAdditionLibrary::__CalculateSoldierAddition(50, SOLDIER_TECH::MATK);
	m_nTechnology[2] = CAdditionLibrary::__CalculateSoldierAddition(50, SOLDIER_TECH::DEF);
	m_nTechnology[3] = CAdditionLibrary::__CalculateSoldierAddition(50, SOLDIER_TECH::MDEF);
	m_nTechnology[4] = CAdditionLibrary::__CalculateSoldierAddition(50, SOLDIER_TECH::HP);

	m_nInteralValue[0] = m_nInteralValue[1] = m_nInteralValue[2] = 100;*/
}

float CMaster::__GetSkillRange(const CSkill* pSkill)
{
	float fSkillCastRange = pSkill->GetCastRange();
	float fResult = fSkillCastRange;
	for (auto it : *pSkill->GetBullets())
	{
		const CBulletTP* pBullet = CBulletTP::GetBulletTP(it);
		float fTemp = pBullet->GetVolumn() + pBullet->GetPosOffset() + pBullet->GetMaxMoveDistance() + fSkillCastRange;
		if (fTemp >= fResult)
		{
			fResult = fTemp;
		}
	}
	return fResult;
}

int CMaster::__GetEnemyInRange(const CSkill* pSkill, CHero* pHero)
{
	float nRange = __GetSkillRange(pSkill);
	return m_pBattle->GetSkillEnemyNum(nRange, pHero);
}

int	CMaster::__HeroPassSkillAddResources()
{
	int nResult = 0;
	if (nullptr == m_pUsingHero)
		return nResult;
	const CHeroTP* pHero = m_pUsingHero->GetHeroTP();
	if (nullptr == pHero)
		return nResult;
	for (int i = 0; i < pHero->GetPassSkillSize(); i++)
	{
		const CPassSkill* pSkill = CPassSkill::GetPassSkill(pHero->GetPassSkill(i));
		if (nullptr != pSkill)
		{
			for (int i = 0; i < pSkill->GetBaseSize(); i++)
			{
				if (TriggerTime_Always == pSkill->GetTriggerTime() &&
					BaseType_Resources == pSkill->GetBaseType(i))
				{
					nResult += int(pSkill->GetBaseValue(i));
				}
			}
		}
	}
	return nResult;
}

void CMaster::__AIClickHeroCard()
{
	for (size_t i = 0; i < 3; i++)
	{
		HeroCard* pHeroCard = &m_arrHeroes[i];
		if (nullptr != pHeroCard && nullptr != pHeroCard->m_pHeroTP &&
			!pHeroCard->m_bUsed &&
			!m_pUsingHero &&
			pHeroCard->m_pHeroTP)
		{
			pHeroCard->m_bUsed = true;
			m_nHeroDurationTime = 0;

			m_pUsingHero = new CHero{ m_pBattle };
			m_pUsingHero->InitHero(pHeroCard, this);

			float fHitPos = m_pBattle->FindSoldiersLinePos(m_enForce, false, true, true);

			if (m_enForce == Force::ATK)
			{
				float fEnemyHeroPos = m_pBattle->FindHeroLinePos(Force::DEF);
				fHitPos = fEnemyHeroPos < fHitPos ? fEnemyHeroPos : fHitPos;
				fHitPos = fHitPos - pHeroCard->m_pHeroTP->GetAtkDistance();
				fHitPos = 0 > fHitPos ? 0 : fHitPos;
			}

			else if (m_enForce == Force::DEF)
			{
				float fEnemyHeroPos = m_pBattle->FindHeroLinePos(Force::ATK);
				fHitPos = fEnemyHeroPos > fHitPos ? fEnemyHeroPos : fHitPos;
				fHitPos = fHitPos + pHeroCard->m_pHeroTP->GetAtkDistance();
				fHitPos = fHitPos > 6300 ? 6300 : fHitPos;
			}

			m_pUsingHero->SetPos(fHitPos);
			m_pUsingHero->SetIsAI(m_bIsAI);
			m_pUsingHero->SetPosIndex(i);

			if (m_pBattle->AddObject(m_pUsingHero))
			{
				pto_BATTLE_Struct_ClickHeroCard ptoClick;
				ptoClick.set_nmaster(m_nID);
				ptoClick.set_ncardindex(i);
				ptoClick.set_nunitid(m_pUsingHero->GetID());
				ptoClick.set_nrank(pHeroCard->color_);

				m_pBattle->GetProtoPackage(Force::ATK)->ClickHeroCard(&ptoClick);
				m_pBattle->GetProtoPackage(Force::DEF)->ClickHeroCard(&ptoClick);

				m_pUsingHero->SetBuff(1, nullptr);
			}
		}
	}
}

bool CMaster::__EnemyUsingHero()
{
	Force enForce = Force::UNKNOWN;
	if (m_enForce == Force::ATK)
		enForce = Force::DEF;
	if (m_enForce == Force::DEF)
		enForce = Force::ATK;

	for (size_t i = 1; i < 6; i++)
	{
		CMaster* pMaster = m_pBattle->FindMasterByMID(i);
		if (pMaster)
		{
			if (pMaster->GetForce() == enForce &&
				pMaster->UsingHero())
				return true;
		}
	}
	return false;
}

void CMaster::GetStageLoot(int id, int num)
{
	auto it = get_stage_loot_.find(id);
	if (it == get_stage_loot_.cend())
	{
		get_stage_loot_.insert(make_pair(id, num));
	}
	else
	{
		it->second += num;
	}
}

void CMaster::SetStageLootPto(pto_BATTLE_S2C_NTF_Game_Over* pto_game_over)
{
	for (auto it : get_stage_loot_)
	{
		dat_BATTLE_STRUCT_StageLoot* stage_loot = pto_game_over->add_stage_loot();
		stage_loot->set_nitemid(it.first);
		stage_loot->set_nitemnum(it.second);
	}
}

void CMaster::SetStageLootDeletePto(dat_BATTLE_STRUCT_StageLootEx* stage_loot_ex)
{
	stage_loot_ex->set_pid(m_nPID);
	for (auto &it : get_stage_loot_)
	{
		dat_BATTLE_STRUCT_StageLoot* stage_loot = stage_loot_ex->add_stage_loot();
		stage_loot->set_nitemid(it.first);
		stage_loot->set_nitemnum(it.second);
	}
}

SoldierSoul* CMaster::FindSoldierSoul(int soldier_id)
{
	for (auto &it : soldiers_soul_)
	{
		if (it.soldier_id_ == soldier_id)
			return &it;
	}

	return nullptr;
}

void CMaster::UseBaseGun()
{
	pto_BATTLE_S2C_RES_UseBaseGun pto_res;
	CBuilding* base{ nullptr };
	bool base_gun{ false };
	if (m_enForce == Force::ATK)
	{
		base = m_pBattle->GetAtkCastle();
		base_gun = m_pBattle->atk_base_gun();
	}
	else if (m_enForce == Force::DEF)
	{
		base = m_pBattle->GetDefCastle();
		base_gun = m_pBattle->def_base_gun();
	}

	if (remain_base_gun_ <= 0 || !base_gun || using_base_gun_ || !base || (m_nID != 1 && m_nID != 4))
	{
		pto_res.set_res(1);
		pto_res.set_remain(remain_base_gun_);
	}
	else
	{
		remain_base_gun_--;
		pto_res.set_res(0);
		pto_res.set_remain(remain_base_gun_);
	}

	std::string str_pto;
	pto_res.SerializeToString(&str_pto);
	m_pBattle->SendToMaster(this, str_pto, MSG_S2C, BATTLE_S2C_RES_UseBaseGun);

	if (!pto_res.res())
	{
		BaseGunActivate();
	}
}

void CMaster::BaseGunActivate()
{
	base_gun_timer_ = 0;
	using_base_gun_ = true;

	pto_BATTLE_S2C_NTF_UseBaseGun pto_ntf;
	pto_ntf.set_master_id(m_nID);
	std::string str_ntf;
	pto_ntf.SerializeToString(&str_ntf);
	m_pBattle->SendToAllMaster(str_ntf, MSG_S2C, BATTLE_S2C_NTF_UseBaseGun);
}

void CMaster::BaseGunFire()
{
	using_base_gun_ = false;
	base_gun_timer_ = 0;
	m_pBattle->ProduceBaseGunBullet(this);
}

void CMaster::BaseGunTimer()
{
	base_gun_timer_ += BATTLE_LOOP_TIME;
	if (base_gun_timer_ >= base_gun_active_time)
		BaseGunFire();
}

void CMaster::GetExtraBlueprint()
{
	if (m_pBattle->GetBattleMode() != BattleMode::MULTISTAGE)
		return;
	if (multi_stage_clear_)
		return;

	CEliteStage* stage;
	if (m_pBattle->GetBattleMode() == BattleMode::MULTISTAGE)
		stage = (CEliteStage*)CEliteStage::GetMultiStageByLevel(m_pBattle->GetStageID());
	else if (m_pBattle->GetBattleMode() == BattleMode::ELITESTAGE)
		stage = (CEliteStage*)CEliteStage::GetEliteStageByLevel(m_pBattle->GetStageID());
	else
		return;

	if (!stage)
		return;

	const CItemTP* item = CItemTP::GetItemTP(stage->RewardBox());
	if (!item)
		return;

	const CLottery* lottery = CLottery::GetLottery(item->GetFunctional());
	if (!lottery)
		return;

	int blueprint_id = lottery->GetCertainLootID(GetRandom(0, 5));
	const CItemTP* blueprint = CItemTP::GetItemTP(blueprint_id);
	if (!blueprint)
		return;

	get_stage_loot_.insert(std::make_pair(blueprint_id, 1));
}

bool CMaster::UseStrategySoldier(const CSoldierTP* soldier, int pos)
{
	if (!soldier)
		return false;

	const CSkill* skill = CSkill::GetSkill(soldier->strategy_id());
	if (!skill)
		return true;

	SKILL_SEPECIAL_EFFECT enSSE = skill->GetSepecialEffect();

	if (skill->GetSepecialEffect() == SSE_AMBUSH)
		pos = (int)m_pBattle->FindSoldiersLinePos(m_enForce, false, true, true);

	for (size_t i = 0; i < skill->GetBulletSize(); i++)
	{
		const CBulletTP* bullet_tp = CBulletTP::GetBulletTP(skill->GetBulletID(i));
		if (bullet_tp)
		{
			CBullet *pBullet = new CBullet(m_pBattle);

			pBullet->InitBullet(bullet_tp, m_enForce, (float)pos);
			pBullet->SetEffectValueEx(1000000);
			pBullet->SetInfinite(false);
			pBullet->SetMaster(this);

			m_pBattle->AddObject(pBullet, true);
		}
	}

	if (enSSE == SSE_CLEAR_SOLDIER_CARD_CD)
	{
		this->ClearSoldierCardCD();
	}
	else if (enSSE == SSE_ADD_RESOURCE_300)
	{
		this->ChangeResource(300);
	}
	else if (enSSE == SSE_ADD_RESOURCE_25)
	{
		this->ChangeResource(25);
	}
	else if (enSSE == SSE_ADD_ELITE_NUM)
		this->AddEliteNum();
	else if (enSSE == SSE_ADD_COMMON_NUM)
		this->AddCommonNum();
	else if (enSSE == SSE_RESET_HERO)
		this->ResetHero();
	else if (enSSE == SSE_HERO_DISAPPEAR)
	{
		if (m_pUsingHero)
			SetHeroDurationTime(m_pUsingHero->GetMaxTime());
	}
	else if (enSSE == SSE_REPAIR_BASE)
	{
		if (m_enForce == Force::ATK && m_pBattle->GetAtkCastle())
			m_pBattle->GetAtkCastle()->Repair(1500);
		else if (m_pBattle->GetDefCastle())
			m_pBattle->GetDefCastle()->Repair(1500);
	}
	return true;
}

void CMaster::AddCommonNum()
{
	for (auto &it : m_arrSoldiers)
	{
		const CSoldierTP* soldier_tp = CSoldierTP::GetSoldierTP(it.m_nCardID);
		if (soldier_tp)
		{
			if (soldier_tp->GetType() == SOLDIER_TYPE::NORMAL)
			{
				it.m_nCanUseNum++;
			}
		}
	}
	UpdateSoldierCanUseNum();
}

void CMaster::AddEliteNum()
{
	for (auto it : m_arrSoldiers)
	{
		const CSoldierTP* soldier_tp = CSoldierTP::GetSoldierTP(it.m_nCardID);
		if (soldier_tp)
		{
			if (soldier_tp->GetType() == SOLDIER_TYPE::ELITE)
			{
				it.m_nCanUseNum++;
			}
		}
	}
	UpdateSoldierCanUseNum();
}

void CMaster::UpdateSoldierCanUseNum()
{
	pto_BATTLE_S2C_NTF_UpdateSoldierCanUseNum pto_ntf;
	for (size_t i = 0; i < MAX_SOLDIER_NUM; i++)
	{
		const CSoldierTP* soldier_tp = CSoldierTP::GetSoldierTP(m_arrSoldiers[i].m_nCardID);
		if (soldier_tp)
		{
			dat_BATTLE_STRUCT_SoldierCanUseNum* pto_struct = pto_ntf.add_soldier_can_use_num();
			pto_struct->set_card_index(i);
			pto_struct->set_num(m_arrSoldiers[i].m_nCanUseNum);
		}
	}

	std::string str_pto;
	pto_ntf.SerializeToString(&str_pto);

	m_pBattle->SendToMaster(this, str_pto, MSG_S2C, BATTLE_S2C_NTF_UpdateSoldierCanUseNum);
}

void CMaster::SetTryHero(int hero_id, int hero_level)
{
	if (IsAI())
		return;

	const CHeroTP* hero_tp = CHeroTP::GetHeroTP(hero_id);
	if (!hero_tp)
		return;

	float hp_mul = CHeroTP::GetHeroTypeAddition(hero_tp->GetType(), 0);
	float atk_mul = CHeroTP::GetHeroTypeAddition(hero_tp->GetType(), 1);
	float def_mul = CHeroTP::GetHeroTypeAddition(hero_tp->GetType(), 2);
	float matk_mul = CHeroTP::GetHeroTypeAddition(hero_tp->GetType(), 3);
	float mdef_mul = CHeroTP::GetHeroTypeAddition(hero_tp->GetType(), 4);

	bool has_hero{ false };
	if (m_arrHeroes[0].m_pHeroTP)
	{
		has_hero = true;
	}

	m_arrHeroes[0].m_pHeroTP = hero_tp;

	m_arrHeroes[0].level_ = hero_level;
	m_arrHeroes[0].color_ = 5;
	m_arrHeroes[0].atk_ = static_cast<int>(((CAddition::GetCustomMasterEquipAtk(hero_level) + hero_tp->GetBaseATK() * atk_mul)) + (hero_tp->GetGrowupAtk() * hero_level));
	m_arrHeroes[0].matk_ = static_cast<int>(((CAddition::GetCustomMasterEquipAtk(hero_level) + hero_tp->GetBaseMATK() * matk_mul)) + (hero_tp->GetGrowupMAtk() * hero_level));
	m_arrHeroes[0].def_ = static_cast<int>(((CAddition::GetCustomMasterEquipDef(hero_level) + hero_tp->GetBaseDEF() * def_mul)) + (hero_tp->GetGrowupDef() * hero_level));
	m_arrHeroes[0].mdef_ = static_cast<int>(((CAddition::GetCustomMasterEquipDef(hero_level) + hero_tp->GetBaseMDEF() * mdef_mul)) + (hero_tp->GetGrowupMDef() * hero_level));
	m_arrHeroes[0].hp_ = static_cast<int>(((CAddition::GetCustomMasterEquipHP(hero_level) + hero_tp->GetBaseHP()) * hp_mul) + (hero_tp->GetGrowupHP() * hero_level));
	m_arrHeroes[0].exsits_time_ = 0;

	if (hero_level >= 40)
	{
		m_arrHeroes[0].strength_ = hero_tp->GetStr() + (hero_level * 2 - 40);
		m_arrHeroes[0].command_ = hero_tp->GetCmd() + (hero_level * 2 - 40);
		m_arrHeroes[0].intelligence_ = hero_tp->GetInt() + (hero_level * 2 - 40);
	}
	else
	{
		m_arrHeroes[0].strength_ = hero_tp->GetStr();
		m_arrHeroes[0].command_ = hero_tp->GetCmd();
		m_arrHeroes[0].intelligence_ = hero_tp->GetInt();
	}


	const CHeroQuality* quality = CHeroQuality::GetHeroQuality(5);
	if (quality)
	{
		//m_arrHeroes[0].exsits_time_ += quality->time();
		m_arrHeroes[0].strength_ += quality->strength();
		m_arrHeroes[0].command_ += quality->command();
		m_arrHeroes[0].intelligence_ += quality->intelligence();
	}

	if (!has_hero)
	{
		m_arrHeroes[0].move_speed_ = hero_tp->move_speed();
		m_arrHeroes[0].prev_atk_ = hero_tp->atk_prev();
		m_arrHeroes[0].atk_interval_ = hero_tp->atk_interval();
		m_arrHeroes[0].crit_odds_ = hero_tp->crit();
		m_arrHeroes[0].crit_value_ = hero_tp->crit_value();
		m_arrHeroes[0].crit_resistance_ = 0;
		m_arrHeroes[0].building_atk_ = hero_tp->GetAtkBuildingDamage();
	}
}