#pragma once

class CProtoPackage
{
public:
	CProtoPackage();
	~CProtoPackage();

	void NewPackage();					//新建协议包

	void Clear();						//清空协议包

	const std::string& GetMsgStr();			//获取协议包字符串

	bool IsNull();						//判断协议包是不是空的来节约带宽。

	void UseSkillComplete(int nUnitID, int nSkillIndex);
	void FallBack(int UnitID, float fX);
	void BreakHide(int UnitID);
	void BeDamage(int ObjID, int nValue, int nTargetType, int nSrcID = 0, int nSrcType = 0, bool bIsCrit = false, int nDamageEffectID = 0, bool isNormalAttack = false);
	void Heal(int nUnitID, int nValue, int nHealerID, int nHealerType, int nHealEffectID, int nTargetType, bool bIsCrit);
	void BulletMove(int nID, int nBulletType, bool bIsMove);
	void BulletDead(int nID, int nBulletType, int nDeathByWho);
	void BulletMoveComplete(int nID, int nBulletType, bool IsComplete);
	void CreateBullet(pto_BATTLE_Struct_Bullet* ptoBullet);
	void SetBuff(int nID, int  unit_id, int buff_id);
	void EndBuff(int nID, int  unit_id);
	void StartAttack(int nUnitID, int nTargetID, int TargetType, float fX);
	void UnitStartMove(int nUnitID, bool bDirection);
	void StopMove(int nID, int nValue, float fX);
	void StartUseSkill(int nUnitID, int nSkillIndex, int nSkillType, float fX);
	void UnitDead(int nUnitID);
	void ClickHeroCard(pto_BATTLE_Struct_ClickHeroCard* pPtoClickHero);
	void AddUnit(pto_BATTLE_Struct_Unit* pPtoUnit);
	void RemoveUnit(int nID);

private:
	pto_BATTLE_S2C_NTF_PerPackage			m_PerLoopSender;

	pto_BATTLE_S2C_NTF_ClickHeroCard*		m_pClickHeroCard{ nullptr };
	pto_BATTLE_S2C_NTF_AddUnit*				m_pAddUnit{ nullptr };
	pto_BATTLE_Struct_PerPackage*			m_pPerPackage{ nullptr };
	pto_BATTLE_S2C_NTF_BeDamage*			m_pDamagedProto{ nullptr };
	pto_BATTLE_S2C_NTF_Heal*				m_pHealProto{ nullptr };
	pto_BATTLE_S2C_NTF_FallBack*			m_pFallBackProto{ nullptr };
	pto_BATTLE_S2C_NTF_UseSkillComplete*	m_pUseSkillCompleteProto{ nullptr };
	pto_BATTLE_S2C_NTF_BreakHide*			m_pBreakHideProto{ nullptr };
	pto_BATTLE_S2C_NTF_BulletMove*			m_pBulletMoveProto{ nullptr };
	pto_BATTLE_S2C_NTF_BulletDead*			m_pBulletDeadProto{ nullptr };
	pto_BATTLE_S2C_NTF_BulletMoveComplete*	m_pBulletBulletMoveCompleteProto{ nullptr };
	pto_BATTLE_S2C_NTF_CreateBullet*		m_pCreateBulletProto{ nullptr };
	pto_BATTLE_S2C_NTF_UnitDead*			m_pUnitDeadPtoro{ nullptr };
	pto_BATTLE_S2C_NTF_StartAttack*			m_pStartAttackPtoro{ nullptr };
	pto_BATTLE_S2C_NTF_StartMove*			m_pStartMoveProto{ nullptr };
	pto_BATTLE_S2C_NTF_StopMove*			m_pStopMoveProto{ nullptr };
	pto_BATTLE_S2C_NTF_StartUseSkill*		m_pStartUseSkillProto{ nullptr };
	pto_BATTLE_S2C_NTF_BuffStart*			m_pBuffStartProto{ nullptr };
	pto_BATTLE_S2C_NTF_BuffEnd*				m_pBuffEndProto{ nullptr };
	pto_BATTLE_S2C_NTF_RemoveUnit*			m_pRemoveUnit{ nullptr };

	bool __InsertPto(int nUnitID, ::google::protobuf::Message* pto);

	map<int, ::google::protobuf::Message*> m_mapUnitMsg;

	string m_strData;

	bool m_bIsNull{ true };
};
