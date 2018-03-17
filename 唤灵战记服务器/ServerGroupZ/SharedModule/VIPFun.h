#pragma once

enum VIPFunType
{
	VFT_VIPLevel,					//�ȼ�
	VFT_Recharge,					//��Ҫ��Ǯ
	VFT_BuyStaminaTimes,			//�����������
	VFT_GoldPointing,				//������
	VFT_Bag,						//���ⱳ������
	VFT_EscortProtectTimes,			//���Ᵽ������
	VFT_BuyRewardTimes,				//�������ʹ���
	VFT_MeditationTime,				//ڤ��ʱ��
	VFT_DonationGold,				//������״���
	VFT_BuyOfflinebattleTimes,		//���ݹ������
	VFT_VipPackage,					//VIP���
	VFT_OpenRandCard,				//ˢ����
	VFT_TrainPet,					//����������
	VFT_RelievedInteriorCD,			//������CD
	VFT_OpenTrainPlatinum,			//�׽�����
	VFT_FullRuneEngery,				//��������
	VFT_RelievedHeroInteriorCD,		//Ӣ������������Ϣ
	VFT_SkipOfflinebattle,			//��������ս��
	VFT_UpgradeEquipCrit,			//ǿ������
	VFT_ExLottery,					//��������ȡ
	VFT_QuickTrainPet,				//����һ���������
	VFT_OpenTrainExtreme,			//��������
	VFT_RelievedOfflinebattleCD,	//���߾�������CD
	VFT_LowerTechnologyPrice,		//�Ƽ������ۿ�15%
	VFT_QuickChange,				//һ����Ҷһ�10��
	VFT_GoldReward,					//��������ֻ����ɫ
	VFT_AutoMeltingRune,			//����Զ�����
	VFT_BuyExercisePlatformTimes,	//��������̨����
};

class CVIPFun
{
public:
	CVIPFun();
	~CVIPFun();
	static void Load();
	static const CVIPFun* GetVIPFun(int nLV);
	static int GetVIPFun(int nVIPLevel, VIPFunType enType);
	static int GetVIPLevel(int money);

	int  GetVIPLevel() const { return m_nVIPLevel; }
	int  GetRecharge() const { return m_nRecharge; }
	int  GetBuyStaminaTimes() const { return m_nBuyStaminaTimes; }
	int  GetGoldPointing() const { return m_nGoldPointing; }
	int  GetBag() const { return m_nBag; }
	int  GetEscortProtectTimes() const { return m_nEscortProtectTimes; }
	int  GetBuyReward() const { return m_nBuyRewardTimes; }
	int  GetMeditationTime() const { return m_nMeditationTime; }
	int  GetDonationGold() const { return m_nDonationGold; }
	int  GetBuyOfflinebattleTimes() const { return m_nBuyOfflinebattleTimes; }
	bool VipPackage() const { return m_bVipPackage; }
	bool OpenRandCard() const { return m_bOpenRandCard; }
	bool TrainPet() const { return m_bTrainPet; }
	bool RelievedInteriorCD() const { return m_bRelievedInteriorCD; }
	bool OpenTrainPlatinum() const { return m_bOpenTrainPlatinum; }
	bool FullRuneEngery() const { return m_bFullRuneEngery; }
	bool RelievedHeroInteriorCD() const { return m_bRelievedHeroInteriorCD; }
	bool SkipOfflinebattle() const { return m_bSkipOfflinebattle; }
	bool UpgradeEquipCrit() const { return m_bUpgradeEquipCrit; }
	bool ExLottery() const { return m_bExLottery; }
	bool QuickTrainPet() const { return m_bQuickTrainPet; }
	bool OpenTrainExtreme() const { return m_bOpenTrainExtreme; }
	bool RelievedOfflinebattleCD() const { return m_bRelievedOfflinebattleCD; }
	bool LowerTechnologyPrice() const { return m_bLowerTechnologyPrice; }
	bool QuickChange() const { return m_bQuickChange; }
	bool GoldReward() const { return m_bGoldReward; }
	bool AutoMeltingRune() const { return m_bAutoMeltingRune; }

private:
	static std::map<int, const CVIPFun*> ms_mapVIPFun;
	int  m_nVIPLevel{ 0 };					//�ȼ�
	int  m_nRecharge{ 0 };					//��Ҫ��Ǯ
	int  m_nBuyStaminaTimes{ 0 };			//�����������
	int  m_nGoldPointing{ 0 };				//������
	int  m_nBag{ 0 };						//���ⱳ������
	int  m_nEscortProtectTimes{ 0 };		//���Ᵽ������
	int  m_nBuyRewardTimes{ 0 };			//�������ʹ���
	int  m_nMeditationTime{ 0 };			//ڤ��ʱ��
	int  m_nDonationGold{ 0 };				//������״���
	int  m_nBuyOfflinebattleTimes{ 0 };		//���ݹ������
	bool m_bVipPackage{ 0 };				//VIP���
	bool m_bOpenRandCard{ 0 };				//ˢ����
	bool m_bTrainPet{ 0 };					//����������
	bool m_bRelievedInteriorCD{ 0 };		//������CD
	bool m_bOpenTrainPlatinum{ 0 };			//�׽�����
	bool m_bFullRuneEngery{ 0 };			//��������
	bool m_bRelievedHeroInteriorCD{ 0 };	//Ӣ������������Ϣ
	bool m_bSkipOfflinebattle{ 0 };			//��������ս��
	bool m_bUpgradeEquipCrit{ 0 };			//ǿ������
	bool m_bExLottery{ 0 };					//��������ȡ
	bool m_bQuickTrainPet{ 0 };				//����һ���������
	bool m_bOpenTrainExtreme{ 0 };			//��������
	bool m_bRelievedOfflinebattleCD{ 0 };	//���߾�������CD
	bool m_bLowerTechnologyPrice{ 0 };		//�Ƽ������ۿ�15%
	bool m_bQuickChange{ 0 };				//һ����Ҷһ�10��
	bool m_bGoldReward{ 0 };				//��������ֻ����ɫ
	bool m_bAutoMeltingRune{ 0 };			//����Զ�����
	int	 buy_exercise_platform_times_{ 0 };	//��������̨����
};
