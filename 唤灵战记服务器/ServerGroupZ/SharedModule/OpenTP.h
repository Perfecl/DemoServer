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

	int m_nRewardExp{ 0 };      //��������
	int m_nRewardSilver{ 0 };    //������Ǯ
	int m_nRewardHonour{ 0 };    //��������
	int m_nRewardReputation{ 0 };//��������
	int m_nRewardGold{ 0 };    //������ȯ
	int m_nRewardStamina{ 0 };   //��������

	int m_nRewardItem1ID{ 0 };  //��������1 ID
	int m_nRewardItem1Num{ 0 }; //��������1 ����
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

