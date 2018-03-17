#pragma once

//公会职务
enum class GuildPosition
{
	NOT_MEMBER = -1,					//不是会员							
	RANK_AND_FILE,						//普通成员
	OFFICER,							//官员
	VICE_CHAIRMAN,						//副会长
	CHAIRMAN							//会长
};

class CGuild
{
public:
	static const size_t kNoticeMaxLength{ 180 };

	static bool CheckGuildName(const std::string& str);

	static std::vector<size_t> member_num_max;

public:
	CGuild(int guild_id);
	~CGuild();

	void	SaveToDatabase();

	void	SendToAllPlayers(const std::string& content, unsigned char protocol_type, int protocol_id, GuildPosition position_least, int except_pid = 0);			//发送给所有玩家

	int		EditNotice(int pid, const std::string& str);							//修改公告
	bool	IsInApplyList(int pid);													//是否在申请名单中
	void	ApplyForJoin(int pid);													//申请加入公会
	void	AgreeApply(int pid, int target_pid, bool is_agree);						//同意
	int		ChangeAccess(int pid, int target_pid, bool is_up);						//改变权限
	bool	QuitGuild(int pid);														//退出公会
	void	Kick(int pid, int target_pid);											//踢出公会
	void	Cession(int pid, int target_pid);										//转让公会
	void	Contribute(int pid, int contribute, int fund);							//公会捐献

	void	ChangeExp(int num);														//改变经验

	void	SetProtocol(dat_STRUCT_GUILD* pto, int pid);							//设置协议

	GuildPosition				GetPosition(int pid);								//查找职务

	int							GetPositionNum(GuildPosition position);				//获得副会长人数

	inline int					guild_id() const { return guild_id_; }
	inline const std::string&	name() const { return name_; }
	inline int					level() const { return level_; }
	inline size_t				member_num() const { return member_.size(); }
	inline int					chairman() const { return chairman_id_; }
	inline const std::string&	notice() const { return notice_; }
	inline bool					is_dissolve() const { return is_dissolve_; }

private:
	int const							guild_id_;									//公会ID
	std::string							name_;										//公会名
	int									level_{ 0 };								//公会等级
	int									exp_{ 0 };									//公会经验
	int									fund_{ 0 };									//公会资金
	std::string							notice_;									//公会公告
	bool								is_dissolve_{ false };						//是否解散

	std::map<int, std::pair<GuildPosition, int>>	member_;						//公会成员(职位,捐献值)
	std::set<int>									apply_list_;					//申请入会

	CCriticalSectionZ					lock_;										//线程锁

	int									chairman_id_{ 0 };							//会长ID

	void	__LoadFromDatabase();
};