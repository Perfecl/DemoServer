#pragma once
#include <boost\pool\object_pool.hpp>

enum ItemLocation
{
	IL_HERO = -1,			//负的就是英雄ID
	IL_BODY,				//身上
	IL_KNAPSACK,			//背包
	IL_STROEROOM,			//临时仓库
	IL_NULL					//无
};

struct CFashion
{
	int			id{ 0 };								//时装ID
	time_t		get_time{ 0 };							//获得时间
	time_t		due_time{ 0 };							//到期时间
};

class CKnapsack
{
public:
	CKnapsack(CPlayer& player);
	~CKnapsack();

	void		ProcessMessage(const CMessage& msg) ;
	void		SaveToDatabase();
	void		SendInitialMessage();

	CItem*		FindItem(int item_id, ItemLocation location);							//寻找物品
	void		DeleteItem(int item_id, ItemLocation location);							//删除物品

	CEquip*		FindEquip(ItemLocation location, EquipPart part);						//寻找装备
	CEquip*		FindEquip(int unique_id);												//寻找装备
	std::vector<const CEquip*> FindAllEquip();											//寻找所有装备

	void		GiveNewEquip(int equip_id, int quality);								//给予新的装备
	void		GiveNewItem(int item_id, int num, bool send_msg = true);				//给予新的道具
	void		GiveSuit(int suit_id);

	void		AddMail(SPMail mail, bool send_msg = true);								//添加邮件
	SPMail		FindMail(int mail_id);													//寻找邮件
	bool		DeleteMail(int mail_id);												//删除邮件
	bool		HasUnreadMails();														//是否有未读邮件

	std::vector<const CEquip*>	GetHeroEquip(int hero_id);								//获取英雄装备

	bool		IsFull();																//判断背包是否已满

	bool		MoveItem(bool is_equip, int id, int num, ItemLocation src, ItemLocation dest);

	int			clothes_id() const;

	void		AddFashion(int id);
	CFashion*	body_fashion(){ return body_fashion_; }

	void		SendUseItem(int item_id, int num);

private:
	CPlayer&							player_;										
	boost::object_pool<CItem>			item_pool_;										//item内存池

	std::map<int, CItem*>				items_;											//物品栏(物品ID,物品)
	std::map<int, CItem*>				temporary_items_;								//临时物品栏(物品ID,物品)

	std::map<CEquip*, ItemLocation>		equips_;										//装备拦(装备,英雄ID(0代表在背包中，-1代表临时背包))

	CFashion*							body_fashion_{ nullptr };						//身上的时装
	std::map<int, CFashion*>			fashions_;										//包里的时装

	std::map<int, SPMail>				mails_;											//邮箱

	int									equip_unique_id_allcoator_{ 1 };				//装备唯一ID分配
	int									mail_unique_id_allcoator_{ 1 };					//邮件唯一ID分配

	bool								is_in_strengthen_cd_{ false };					//是否进入强化CD	
	int									strengthen_seconds_{ 0 };						//累计强化CD时间(秒)
	time_t								last_strengthen_time_{ 0 };						//最后强化时间

	CCriticalSectionZ					lock_;

	std::array<TempLottery, 6>			m_arrTempLottery;								//精英宝箱临时数据
	int									m_nChooseLotteryTimes{ 0 };						//精英宝箱选择次数

	int		__CalculateKnapsackSpace();
	void	__SendItemProtocol(int item_id, int num, ItemLocation location);

	void	__GetOpenReward(const COpenTP* open);

	void	__PutItemToKnapsack(const CMessage& msg);						//物品放入仓库
	void	__DeleteItem(const CMessage& msg);								//删除物品
	void	__SellItem(const CMessage& msg);								//出售物品
	void	__UseItem(const CMessage& msg);									//使用物品
	void	__ExchangeEquip(const CMessage& msg);							//换装备
	void	__UpgradeEquip(const CMessage& msg);							//升级装备
	void	__ReadMail(const CMessage& msg);								//阅读邮件
	void	__GetMailReward(const CMessage& msg);							//获取邮件奖励
	void	__DeleteMail(const CMessage& msg);								//删除邮件
	void	__GetMailList(const CMessage& msg);								//获取邮件列表

	void	__MakeEquip(const CMessage& msg);								//制作装备

	void	__CloseLottery();												//关闭精英宝箱
	void	__ChooseLottery(const CMessage& msg);							//选择精英宝箱

	void	__StartLottery(int nID);										//开始抽取
	void	__ProduceLotteryResult(const CLottery* pLottery);				//产生抽取结果
	void	__RestartLottery();												//重置宝箱
	void	__RandomLottery();												//打乱宝箱
	void	__SendLottery();												//发送宝箱
	bool	__InTempVct(int nNum, std::vector<int>* pVct);					//宝箱位置已复制

	void	__ActivateEquipEnchanting(const CMessage& msg);					//激活装备精炼槽
	void	__Enchanting(const CMessage& msg);								//精炼
	void	__Enchanting(int& type, float& value, CEquip* equip);

	void	__RecastEquip(const CMessage& msg);								//重铸装备
	void    __ResolveEquip(const CMessage& msg);							//分解装备

	void	__ExchangeFashion(const CMessage& msg);
	void	__BuyFashion(const CMessage& msg);
	void	__GetFashion(const CMessage& msg);

	void	__BuyInMall(const CMessage& msg);
	void	__Inherit(const CMessage& msg);

	void	__BatchSell(const CMessage& msg);								//批量出售

	void	__LoadFromDatabase();
};
