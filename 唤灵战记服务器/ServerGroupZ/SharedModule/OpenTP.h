#pragma once

class COpenTP
{
public:
	COpenTP();
	~COpenTP(); 
	static void Load();
	static const COpenTP* GetOpenTP(int nID);

private:
	static std::map<int, const COpenTP*> ms_mapOpenTPs;
public:
	int m_nOpenID{ 0 };

	int m_nRewardExp{ 0 };      //奖励经验
	int m_nRewardSilver{ 0 };    //奖励金钱
	int m_nRewardHonour{ 0 };    //奖励军功
	int m_nRewardReputation{ 0 };//奖励声望
	int m_nRewardGold{ 0 };    //奖励点券
	int m_nRewardStamina{ 0 };   //奖励体力

	int m_nRewardItem1ID{ 0 };  //奖励道具1 ID
	int m_nRewardItem1Num{ 0 }; //奖励道具1 数量
	int m_nRewardItem2ID{ 0 };
	int m_nRewardItem2Num{ 0 };
	int m_nRewardItem3ID{ 0 };
	int m_nRewardItem3Num{ 0 };
	int m_nRewardItem4ID{ 0 };
	int m_nRewardItem4Num{ 0 };
	int m_nRewardItem5ID{ 0 };
	int m_nRewardItem5Num{ 0 };
	int m_nRewardItem6ID{ 0 };
	int m_nRewardItem6Num{ 0 };

	int m_nRewardEquip1ID{ 0 };
	int m_nRewardEquip2ID{ 0 };
	int m_nRewardEquip3ID{ 0 };
	int m_nRewardEquip4ID{ 0 };
};

