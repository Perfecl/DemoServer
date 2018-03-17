#pragma once

enum MailModelID
{
	MMI_FairArena = 1,		//1	�����������������
	MMI_Escort,				//2	�̶�ս��������
	MMI_EscortProtect,		//3	�̶�ս������������
	MMI_SpeedStage,			//4	��������������
	MMI_TerritoryBattle,	//5	��������������
	MMI_OfflinBattle,		//6	����ͨ�ؽ�������
	MMI_GuildBattle,		//7	����ս��������
	MMI_BossBattle,			//8	����boss��������
	MMI_TownStageRank,		//9	�������ǰʮ����
	MMI_DayWinAward,		//10��ƽ������ʤ
	MMI_Recharge,			//11��ֵ����
};

class CMail
{
	friend class CKnapsack;
public:
	static void InsertToDatabase(SPMail mail, int pid);

public:
	CMail();
	~CMail();

	void		GetReward(SPPlayer player);												//��ȡ����
	void		SetProtocol(dat_PLAYER_STRUCT_Mail *pto);								//�����ʼ�Э��

	void		SetModelID(int nModelID){ model_id_ = nModelID; }

	void		GetRward(SPPlayer SPPlayer);

	void		SetIsRead(bool bIsRead){ is_read_ = bIsRead; }
	bool		IsRead(){ return is_read_; }

	void		SetRewardHeroID(int nHeroID){ reward_hero_id_ = nHeroID; }
	void		SetRewardSoldierID(int nSoldierID){ reward_soldier_id_ = nSoldierID; }
	void		AddRewardItem(int nID, __int64 nNum){ reward_items_.push_back(std::make_pair(nID, nNum)); }
	void		AddRewardEquip(int nEquipID){ reward_equips_.push_back(nEquipID); }
	void		SetRewardTitle(int title){ reward_title_ = title; }

	void		SetName(std::string strName){ name_ = strName; }
	const char* GetName() const { return name_.c_str(); }

	void		SetParam0(int nParam){ param0_ = nParam; }
	int			GetParam0() const { return param0_; }

	void		SetParam1(int nParam){ param1_ = nParam; }
	int			GetParam1()const{ return param1_; }

private:
	int											id_{ 0 };							//�ʼ�ID
	int											model_id_{ 1 };						//����ģ��ID
	int											param0_{ 0 };						//����1
	int											param1_{ 0 };						//����2
	std::string									name_;								//����
	time_t										recv_time_{ time(0) };				//����ʱ��
	bool										is_read_{ false };					//�Ƿ��ȡ��
	int											reward_hero_id_{ 0 };				//�����佫
	int											reward_soldier_id_{ 0 };			//����ʿ��
	std::vector<std::pair<int, __int64>>		reward_items_;						//��������
	std::vector<int>							reward_equips_;						//����װ��
	int											reward_title_{ 0 };					//�����ƺ�
};

