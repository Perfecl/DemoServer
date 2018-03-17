#pragma once
#include <boost\pool\object_pool.hpp>

enum ItemLocation
{
	IL_HERO = -1,			//���ľ���Ӣ��ID
	IL_BODY,				//����
	IL_KNAPSACK,			//����
	IL_STROEROOM,			//��ʱ�ֿ�
	IL_NULL					//��
};

struct CFashion
{
	int			id{ 0 };								//ʱװID
	time_t		get_time{ 0 };							//���ʱ��
	time_t		due_time{ 0 };							//����ʱ��
};

class CKnapsack
{
public:
	CKnapsack(CPlayer& player);
	~CKnapsack();

	void		ProcessMessage(const CMessage& msg) ;
	void		SaveToDatabase();
	void		SendInitialMessage();

	CItem*		FindItem(int item_id, ItemLocation location);							//Ѱ����Ʒ
	void		DeleteItem(int item_id, ItemLocation location);							//ɾ����Ʒ

	CEquip*		FindEquip(ItemLocation location, EquipPart part);						//Ѱ��װ��
	CEquip*		FindEquip(int unique_id);												//Ѱ��װ��
	std::vector<const CEquip*> FindAllEquip();											//Ѱ������װ��

	void		GiveNewEquip(int equip_id, int quality);								//�����µ�װ��
	void		GiveNewItem(int item_id, int num, bool send_msg = true);				//�����µĵ���
	void		GiveSuit(int suit_id);

	void		AddMail(SPMail mail, bool send_msg = true);								//����ʼ�
	SPMail		FindMail(int mail_id);													//Ѱ���ʼ�
	bool		DeleteMail(int mail_id);												//ɾ���ʼ�
	bool		HasUnreadMails();														//�Ƿ���δ���ʼ�

	std::vector<const CEquip*>	GetHeroEquip(int hero_id);								//��ȡӢ��װ��

	bool		IsFull();																//�жϱ����Ƿ�����

	bool		MoveItem(bool is_equip, int id, int num, ItemLocation src, ItemLocation dest);

	int			clothes_id() const;

	void		AddFashion(int id);
	CFashion*	body_fashion(){ return body_fashion_; }

	void		SendUseItem(int item_id, int num);

private:
	CPlayer&							player_;										
	boost::object_pool<CItem>			item_pool_;										//item�ڴ��

	std::map<int, CItem*>				items_;											//��Ʒ��(��ƷID,��Ʒ)
	std::map<int, CItem*>				temporary_items_;								//��ʱ��Ʒ��(��ƷID,��Ʒ)

	std::map<CEquip*, ItemLocation>		equips_;										//װ����(װ��,Ӣ��ID(0�����ڱ����У�-1������ʱ����))

	CFashion*							body_fashion_{ nullptr };						//���ϵ�ʱװ
	std::map<int, CFashion*>			fashions_;										//�����ʱװ

	std::map<int, SPMail>				mails_;											//����

	int									equip_unique_id_allcoator_{ 1 };				//װ��ΨһID����
	int									mail_unique_id_allcoator_{ 1 };					//�ʼ�ΨһID����

	bool								is_in_strengthen_cd_{ false };					//�Ƿ����ǿ��CD	
	int									strengthen_seconds_{ 0 };						//�ۼ�ǿ��CDʱ��(��)
	time_t								last_strengthen_time_{ 0 };						//���ǿ��ʱ��

	CCriticalSectionZ					lock_;

	std::array<TempLottery, 6>			m_arrTempLottery;								//��Ӣ������ʱ����
	int									m_nChooseLotteryTimes{ 0 };						//��Ӣ����ѡ�����

	int		__CalculateKnapsackSpace();
	void	__SendItemProtocol(int item_id, int num, ItemLocation location);

	void	__GetOpenReward(const COpenTP* open);

	void	__PutItemToKnapsack(const CMessage& msg);						//��Ʒ����ֿ�
	void	__DeleteItem(const CMessage& msg);								//ɾ����Ʒ
	void	__SellItem(const CMessage& msg);								//������Ʒ
	void	__UseItem(const CMessage& msg);									//ʹ����Ʒ
	void	__ExchangeEquip(const CMessage& msg);							//��װ��
	void	__UpgradeEquip(const CMessage& msg);							//����װ��
	void	__ReadMail(const CMessage& msg);								//�Ķ��ʼ�
	void	__GetMailReward(const CMessage& msg);							//��ȡ�ʼ�����
	void	__DeleteMail(const CMessage& msg);								//ɾ���ʼ�
	void	__GetMailList(const CMessage& msg);								//��ȡ�ʼ��б�

	void	__MakeEquip(const CMessage& msg);								//����װ��

	void	__CloseLottery();												//�رվ�Ӣ����
	void	__ChooseLottery(const CMessage& msg);							//ѡ��Ӣ����

	void	__StartLottery(int nID);										//��ʼ��ȡ
	void	__ProduceLotteryResult(const CLottery* pLottery);				//������ȡ���
	void	__RestartLottery();												//���ñ���
	void	__RandomLottery();												//���ұ���
	void	__SendLottery();												//���ͱ���
	bool	__InTempVct(int nNum, std::vector<int>* pVct);					//����λ���Ѹ���

	void	__ActivateEquipEnchanting(const CMessage& msg);					//����װ��������
	void	__Enchanting(const CMessage& msg);								//����
	void	__Enchanting(int& type, float& value, CEquip* equip);

	void	__RecastEquip(const CMessage& msg);								//����װ��
	void    __ResolveEquip(const CMessage& msg);							//�ֽ�װ��

	void	__ExchangeFashion(const CMessage& msg);
	void	__BuyFashion(const CMessage& msg);
	void	__GetFashion(const CMessage& msg);

	void	__BuyInMall(const CMessage& msg);
	void	__Inherit(const CMessage& msg);

	void	__BatchSell(const CMessage& msg);								//��������

	void	__LoadFromDatabase();
};
