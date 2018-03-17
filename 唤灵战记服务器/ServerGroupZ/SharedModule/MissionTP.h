#pragma once

enum EMissionTargetType
{
	MTT_TalkToNpc = 1,		//1.��NPC�Ի�
	MTT_CompleteMap,		//2.��ɹؿ�
	MTT_EquipLvUpTimes,		//3.ǿ��װ������
	MTT_UseHero,			//4.�佫����
	MTT_UseEquip,			//5.����װ��
	MTT_EquipLvUpLv,		//6.ǿ��װ���ȼ�
	MTT_UseSoldier,			//7.ʿ������
	MTT_UnitTechnology,		//8.ǿ��ʿ��
	MTT_TrainHero,			//9.�佫����
	MTT_Enchanting,			//10.װ������
	MTT_RewardMission,		//11.�������
	MTT_UseProduceCell,		//12.��ɸ���
	MTT_UseBarracksCell,	//13.���ѵ��
	MTT_UseSearchCell,		//14.���̽��
	MTT_CreateOrJoinGuild,	//15.��������빫��
	MTT_GuildContribute,	//16.ļ������
	MTT_RecruitHero,		//17.��ļ�佫
	MTT_DIYLvUp,			//18.�ֶ�����
	MTT_TechnologyLv,		//19.����ĳ��Ƽ���XX��
	MTT_PetLevel,			//20.�������ﵽN��
	MTT_OfflineBattle,		//21.�μ����ݾ�����N��
	MTT_EliteStage,			//22.��ɾ�Ӣ�ؿ�
	MTT_MultiStage,			//23.��ɶ��˹ؿ�
	MTT_SpeedStage,			//24.�μӾ�����
	MTT_FairArena,			//25.�μӿ��������
	MTT_Escort,				//26.�̶�ս����һ��
	MTT_RobEscort,			//27.�̶�ս����һ��
	MTT_AllEquipLevel,		//28.ǿ������װ����N��
	MTT_AllTechnologyLevel,	//29.ǿ�����пƼ���N��
	MTT_GetTownBox,			//30.��ȡ�ؿ�����
	MTT_TrainHeroRank,		//31.ָ��Ӣ���������׼�
	MTT_HeroLevel,			//32.�佫����NN��
	MTT_WashRune,			//33.����ϴ��
	MTT_HeroLevelUp,		//34.�佫����
	MTT_UseLevy,			//35,�������
	MTT_ExchangeFashion,	//36,��ʱװ
	MTT_Shopping,			//37,����
	MTT_Trade,				//38,��
	MTT_War,				//39,����
	MTT_Security,			//40,�����ΰ�
	MTT_AC,
};

class CMissionTP
{
	friend class CMission;
public:
	static void Load();
	static const CMissionTP* GetMission(int nID);
	static const CMissionTP* GetMissionByMainProgress(int nProgress);

private:
	static std::map<int, const CMissionTP*> m_mapMissions;

public:
	CMissionTP() = default;
	~CMissionTP() = default;

	int GetMissionID() const { return m_nID; }
	int GetNeedTargetNum() const { return m_nTargetNeedNum; }
	int GetTargetType()const { return m_nTargetType; }
	int GetTargetID() const { return m_nTargetID; }
	int GetTargetIDEx() const { return m_nTargetIDEx; }
	int GetNeedMainProgress() const { return m_nNeedMainProgress; }
	int GetType() const { return m_nType; }
	int GetNeedLevel() const { return m_nNeedLevel; }

	static void AvailableBranchList(std::vector<int>* branch, int player_level, int main_progress);

private:
	int m_nID{ 0 };							//����ID
	int m_nType{ 0 };						//�������� 1���� 2֧�� 3�ճ� 4����
	int m_nNeedMainProgress{ 0 };			//�������߽���
	int m_nNeedLevel{ 0 };					//����ȼ�
	int m_nFollowMisssionID{ 0 };			//��������ID 

	int m_nAcceptNPC{ 0 };					//������NPC ID
	int m_nCommitNPC{ 0 };					//������NPC ID

	int m_nTargetType{ 0 };					//Ŀ������ 1��NPC�Ի���2��ɹؿ���3��ɸ�����4��ɶ��˸�����5���뾺������6Ӯ�þ�����,7ָ��������
	int m_nTargetID{ 0 };					//Ŀ��ID���Ի�NPC ID,��ɵ�ͼID�� 
	int m_nTargetIDEx{ 0 };					//Ŀ��ID����ֵ���ؿ��Ѷȣ�            
	int m_nTargetNeedNum{ 0 };				//Ŀ������       

	long long m_nRewardExp{ 0 };			//��������
	long long m_nRewardSilver{ 0 };			//������Ǯ
	int m_nRewardHonor{ 0 };				//��������
	int m_nRewardReputation{ 0 };			//��������
	int m_nRewardGold{ 0 };					//������ȯ
	int m_nRewardStamina{ 0 };				//��������

	int m_nRewardItem1{ 0 };				//��������1 ID
	int m_nRewardItem1Num{ 0 };				//��������1 ����
	int m_nRewardItem2{ 0 };				//��������1 ID
	int m_nRewardItem2Num{ 0 };				//��������1 ����
	int m_nRewardItem3{ 0 };				//��������1 ID
	int m_nRewardItem3Num{ 0 };				//��������1 ����
	int m_nRewardItem4{ 0 };				//��������1 ID
	int m_nRewardItem4Num{ 0 };				//��������1 ����
	int m_nRewardItem5{ 0 };				//��������1 ID
	int m_nRewardItem5Num{ 0 };				//��������1 ����
	int m_nRewardItem6{ 0 };				//��������1 ID
	int m_nRewardItem6Num{ 0 };				//��������1 ����

	int m_nRewardEquip1{ 0 };
	int m_nRewardEquip2{ 0 };
	int m_nRewardEquip3{ 0 };
	int m_nRewardEquip4{ 0 };

	int m_nRewardHeroID{ 0 };				//����Ӣ��ID
	int m_nRewardSoldier{ 0 };				//����ʿ��ID

	int m_nBeginStoryID{ 0 };				//�ؿ�ǰ����ID
	int m_nFinishStoryID{ 0 };				//�ؿ������ID
	int m_nOpenFunction{ 0 };				//��������

	int m_nOfferSilver{ 0 };				//�������ṩ����
	int m_nOfferHonor{ 0 };
	int m_nOfferReputation{ 0 };
	int m_nOfferStamina{ 0 };
	int m_nOfferEquip1{ 0 };
	int m_nOfferEquip2{ 0 };
	int m_nOfferSoldier{ 0 };
	int m_nOfferHero{ 0 };
	int m_nOfferEx{ 0 };
};
