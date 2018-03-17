#pragma once

//����ְ��
enum class GuildPosition
{
	NOT_MEMBER = -1,					//���ǻ�Ա							
	RANK_AND_FILE,						//��ͨ��Ա
	OFFICER,							//��Ա
	VICE_CHAIRMAN,						//���᳤
	CHAIRMAN							//�᳤
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

	void	SendToAllPlayers(const std::string& content, unsigned char protocol_type, int protocol_id, GuildPosition position_least, int except_pid = 0);			//���͸��������

	int		EditNotice(int pid, const std::string& str);							//�޸Ĺ���
	bool	IsInApplyList(int pid);													//�Ƿ�������������
	void	ApplyForJoin(int pid);													//������빫��
	void	AgreeApply(int pid, int target_pid, bool is_agree);						//ͬ��
	int		ChangeAccess(int pid, int target_pid, bool is_up);						//�ı�Ȩ��
	bool	QuitGuild(int pid);														//�˳�����
	void	Kick(int pid, int target_pid);											//�߳�����
	void	Cession(int pid, int target_pid);										//ת�ù���
	void	Contribute(int pid, int contribute, int fund);							//�������

	void	ChangeExp(int num);														//�ı侭��

	void	SetProtocol(dat_STRUCT_GUILD* pto, int pid);							//����Э��

	GuildPosition				GetPosition(int pid);								//����ְ��

	int							GetPositionNum(GuildPosition position);				//��ø��᳤����

	inline int					guild_id() const { return guild_id_; }
	inline const std::string&	name() const { return name_; }
	inline int					level() const { return level_; }
	inline size_t				member_num() const { return member_.size(); }
	inline int					chairman() const { return chairman_id_; }
	inline const std::string&	notice() const { return notice_; }
	inline bool					is_dissolve() const { return is_dissolve_; }

private:
	int const							guild_id_;									//����ID
	std::string							name_;										//������
	int									level_{ 0 };								//����ȼ�
	int									exp_{ 0 };									//���ᾭ��
	int									fund_{ 0 };									//�����ʽ�
	std::string							notice_;									//���ṫ��
	bool								is_dissolve_{ false };						//�Ƿ��ɢ

	std::map<int, std::pair<GuildPosition, int>>	member_;						//�����Ա(ְλ,����ֵ)
	std::set<int>									apply_list_;					//�������

	CCriticalSectionZ					lock_;										//�߳���

	int									chairman_id_{ 0 };							//�᳤ID

	void	__LoadFromDatabase();
};