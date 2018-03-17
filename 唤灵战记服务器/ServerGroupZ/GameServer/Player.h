#pragma once
#include "IPlayerData.h"

class CPlayer : public IPlayerData
{
public:
	enum PlayerAccess{ kAccessNormal, kAccessGM };								//Ȩ��

	static const int kMaxStamina{ 200 };										//�������ֵ
	static const int kMaxVIPLevel{ 12 };										//���vip�ȼ�
	static const int kMaxLevel{ 200 };											//�����ߵȼ�

	static const int kRecentsMaxNum{ 30 };										//�����ϵ������

public:
	CPlayer(int pid, networkz::ISession* session);
	~CPlayer();

	void			SendToAgent(const ::google::protobuf::Message& pto, int protocol_id, unsigned char protocol_type = MSG_S2C);		//���͵����������

	void			ProcessMessage(const CMessage& msg);																				//������Ϣ

	void			SaveToDatabase(bool is_offline);																					//���浽���ݿ�

	void			ChangeUser(networkz::ISession* session);																			//�����û��Ự
	void			ReadyToOffline();																									//׼������

	void			CheckOrder();																										//����ֵ����

	void			OnGameOver(pto_BATTLE_S2C_NTF_Game_Over& pto);																		//��ս������ʱ
	bool			GetBoxState(int town_id, StageType type);																			//ȡ�ؿ�����״̬

	std::vector<int>	GetFriendList();
	bool			IsFreind(int pid);

	void			ChangeVIPValue(int num);																							//�ı�VIPֵ
	void			ChangeSilver(__int64 num);																							//�ı�����
	void			ChangeHonor(__int64 num);																							//�ı�����
	void			ChangeGold(int num,int change_pos);																					//�ı���
	void			ChangeReputation(int num);																							//�ı�����
	void			ChangeExp(__int64 num);																								//�ı侭��
	void			ChangeStamina(int num);																								//�ı�����
	void			ItemChangeStamina(int num);
	bool			IsOpenGuide(int guild_id);																							//�Ƿ�ﵽ��������

	bool			GiveNewTitle(int title_id);																							//�����µĳƺ�

	void			AddRecents(int contact_pid);																						//��������ϵ��

	void			ImplementMission(EMissionTargetType target_type, int target_id, int target_id_ex);									//�������
	void			ImplementRewardMission(RewardMissionType type, int target_id, int num);												//������������ж�

	void			LevelUp();																											//����
	void			SetLevel(int level);																								//���õȼ�
	void			SetMaxLevel(int max_level);																							//�������ȼ�

	void			SetBattleMasterProto(pto_BATTLE_STRUCT_Master_Ex *pto, bool is_fair);												//����MasterЭ��
	void			StageBattleWin(const CStage* stage);																				//�ؿ�ʤ��
	void			EliteStageBattleWin(int box_num, const CEliteStage* stage);															//��Ӣ�ؿ�ʤ��
	void			MultiStageBattleWin(int box_num, const CEliteStage* stage);															//���˹ؿ�ʤ��
	void			SpeedStageWin(int time);																							//������ʤ��

	void			OnRoomKick();

	virtual inline  int			pid() override{ return pid_; }
	virtual inline  int			sid() override{ return sid_; }
	virtual inline  const char* name() override{ return name_.c_str(); }
	virtual inline  bool		sex() override{ return sex_; }
	virtual inline  int			town_id() override{ return town_id_; }
	virtual inline  int			level() override{ return level_; }
	virtual inline  int			guild_id() override{ return guild_id_; }
	virtual inline  void		set_speed_best_rank(int rank){ if (rank < speed_stage_record_) speed_stage_record_ = rank; }
	virtual time_t				offline_time() override { return last_offline_time_; }

	virtual inline	int			exercise_time() override;
	virtual int					clothes() override;

	virtual std::string			guild_name() override;
	virtual int					guild_postion() override;

	virtual inline int			reputation() override{ return reputation_; }

	virtual inline int			offline_battle_win_streak() override{ return 0; }
	virtual int					bodygurand() override;

	virtual CHeroCard*			hero(int index) override;
	virtual std::vector<const CEquip*> hero_equip(int hero_id) override;
	virtual int					technology(int index) override;
	virtual inline int			speed_best_rank() override{ return speed_stage_record_; }
	virtual inline time_t		offline_last_time() override;
	virtual inline const BattleRecord&	battle_record(BattleRecordType type) override{ return battle_record_[type]; }
	virtual int					battle_week_times(BattleRecordType type) override;

	virtual inline int			using_title() override { return using_title_; }

	virtual inline  void		guild_id(int guild_id) override { guild_id_ = guild_id; };

	inline __int64				exp() const { return exp_; }
	inline __int64				silver() const { return silver_; }
	inline __int64				honor() const { return honor_; }
	inline int					stamina() const { return stamina_; }
	inline int					room_id() const{ return room_id_; }
	inline bool					is_in_battle() const { return PlayerState::BATTLE == state_; }
	inline int					vip_level() const { return vip_level_; }
	inline int					gold() const{ return gold_; }
	inline int					max_level() const { return max_level_; }
	inline PlayerState			state() const { return state_; }

	inline void					state(PlayerState p_state){ state_ = p_state; }
	inline CNeedReset*			need_reset(){ return need_reset_; }
	inline CKnapsack*			knapsack(){ return knapsack_; }
	inline CArmy*				army() { return army_; }
	inline COffline*			offline() { return offline_; }
	inline CMission*			mission() { return mission_; }
	inline CPlayerExercise*		exercise(){ return exercise_; }

	inline int					town_x() const { return town_x_; }
	inline int					town_y() const { return town_y_; }

	inline int					speed_stage_best_rank() const { return speed_stage_record_; }

	inline void					room_id(int room_id){ room_id_ = room_id; }
	inline void					SetTownID(int town_id){ town_id_ = town_id; }
	inline void					SetTownPosition(int x, int y){ town_x_ = x; town_y_ = y; }

	inline int 					point_war() const { return battle_record_[0].point; }
	inline int 					ponit1v1() const { return battle_record_[1].point; }
	inline int 					ponit3v3() const { return battle_record_[2].point; }
	inline bool					Is1v1FisrBattle() const { return 0 == (battle_record_[1].win + battle_record_[1].lose); }

	inline int					pet_id(){ return 0; }
	inline int					pet_level(){ return 0; }
	int							main_progress();

	inline networkz::ISession*	session() const { return agent_session_; }

private:
	const int						pid_;										//���ID
	int								sid_;										//ServerID
	std::string						name_;										//����
	bool							sex_{ false };								//�Ա�
	int								level_{ 0 };								//�ȼ�
	__int64							exp_{ 0 };									//����
	int								max_level_{ 20 };							//�ȼ�����
	int								vip_level_{ 0 };							//VIP�ȼ�
	int								vip_value_{ 0 };							//VIPֵ
	__int64							silver_{ 0 };								//����
	int								gold_{ 0 };									//���
	__int64							honor_{ 0 };								//����
	int								reputation_{ 0 };							//����
	int								stamina_{ 0 };								//����
	int								using_title_{ 0 };							//ʹ�õĳƺ�
	std::set<int>					have_titles_;								//ӵ�еĳƺ�
	CCriticalSectionZ				title_lock_;								//�ƺ��߳���
	PlayerAccess					access_{ PlayerAccess::kAccessNormal };		//Ȩ��
	time_t							last_offline_time_{ 0 };					//�ϴ�����ʱ��
	const time_t					login_time_;								//���ε�½ʱ��

	CCriticalSectionZ				order_lock_;
	std::set <std::string>			orders_;									//����

	int								town_id_{ 0 };								//����ID
	int								town_x_{ 0 };								//����x����
	int								town_y_{ 0 };								//����y����

	int								guild_id_{ 0 };								//����ID							

	time_t							meditation_time_{ 0 };						//�ϴ�ڤ��ʱ��

	int								online_day_award_{ 0 };						//��������������ȡ����
	std::bitset<32>					vip_level_award_{ 0 };						//vip�ȼ�������ȡ���� (0.�׳� 1.vip1 2.vip2 ....)

	int								box_normal_progress_{ 0 };					//ͨ����ͨ������ȡ����
	int								box_hard_progress_{ 0 };					//ͨ�����ѱ�����ȡ����
	int								box_nightmare_progress_{ 0 };				//ͨ��ج�α�����ȡ����

	time_t							sweep_elite_stage_time_{ 0 };				//ɨ����Ӣ�ؿ�ʱ��

	time_t							last_world_chat_time_{ 0 };					//���ȫ������

	networkz::ISession*				agent_session_;								//Agent�Ự

	PlayerState						state_{ PlayerState::NORMAL };				//���״̬

	std::array<BattleRecord, kBattleRecordCount> battle_record_;				//ս����¼

	int								speed_stage_record_;						//��������ü�¼(����)

	std::string						proto_buffer_;								//Э�鷢�ͻ���
	CCriticalSectionZ				proto_buffer_lock_;							//Э�鷢����

	void __LoadFromDatabase();													//�����ݿ��ȡ����
	void __LoadAwards();														//��ȡ������
	void __LoadRecord();														//��ȡս����¼

	void __SaveAwards();														//���潱����
	void __SaveRecord();														//����ս����¼
	void __SaveOrder();															//���涩��

	void __EnterTown(const CMessage& msg);										//�������
	void __LeaveTown();															//�뿪����
	void __TownMove(const CMessage& msg);										//�����ƶ�

	void __GetMeditation();														//��ȡڤ��

	void __InitMaxLevel();														//��ʼ����ߵȼ�

	void __GetAllTitle();														//��ȡ���гƺ�
	void __UseTitle(const CMessage& msg);										//ʹ�óƺ�

	void __OnPlayerInfo();														//��ȡ��ҳ�ʼ��Ϣ
	void __SendOnlineBoxInfo();													//�������߱�����Ϣ
	void __SendStageInfo();														//���͹ؿ���Ϣ

	void __OnPlayerBase(const CMessage& msg);									//��ȡ��һ�����Ϣ
	void __OnPlayerBaseEx(const CMessage& msg);									//��ȡ��һ�����Ϣ��չ
	void __OnPLayerDetailInfo(const CMessage& msg);								//��ȡ�����ϸ��Ϣ

	void __GetTownBox(const CMessage& msg);										//��ȡ��ؽ���

	void __SetBoxState(int town_id, StageType type, bool is_get);				//���ùؿ�����״̬
	void __UpdateTownTop10(const CMessage& msg);								//���³�������

	void __StartStage(const CMessage& msg);										//��ʼ�ؿ�
	void __StartEliteStage(const CMessage &msg);								//��ʼ��Ӣ�ؿ�
	void __StartSpeedStage();													//��ʼ���ٹؿ�
	void __SweepStage(const CMessage& msg);										//ɨ���ؿ�
	void __SweepEliteStage(const CMessage &msg);								//ɨ����Ӣ�ؿ�
	void __StartSweepEliteStage(const CMessage &msg);							//��ʼɨ����Ӣ��
	void __UpdateSpeedStage();													//���¾��ٹؿ�
	void __CheckSpeedFormation(const CMessage &msg);							//��龺��������
	void __QuitBattle();														//�˳�ս��
	void __GetDayWinAward();													//��ÿ��ս����
	void __GetVIPAward(const CMessage &msg);									//���vip�ȼ�����
	void __GetOnlineAward(const CMessage &msg);									//���������������

	void __SpeedRankList(const CMessage &msg);									//����������
	void __UpdateBattleRecord();												//����ս����¼

	void __SetProtobuff(dat_PLAYER_STRUCT_PlayerData* pto);						//����Э��

	void __TestCommand(const CMessage& msg);									//��������
	void __GMCommand(std::string& message);										//GM����

	void __GetGift(const CMessage& msg);										//��ȡ���

	void __GMRecharge(int pid, int rmb);										//gm��Ǯ
	void __GmGiveHero(int pid, int hero_id);
	void __GmImplementMission(int pid);

	void __KickPlayer(int pid);													//������
	void __FrozenPlayer(int pid, int hour);										//�������

	void __GiveArenaSingleBonus(bool bIsWin,int nTime);							//�����������

	void SendResetServer(int time);


#pragma region ���
	CNeedReset*						need_reset_{ nullptr };						//��Ҫ����
	CKnapsack*						knapsack_{ nullptr };						//����
	CArmy*							army_{ nullptr };							//����
	COffline*						offline_{ nullptr };						//����
	CMission*						mission_{ nullptr };						//����
	CPlayerExercise*				exercise_{ nullptr };						//����̨
#pragma endregion

#pragma region �罻
	std::set<int>					friends_list_;								//�����б�	
	std::set<int>					black_list_;								//������
	std::list<int>					recents_list_;								//�����ϵ��
	CCriticalSectionZ				social_lock_;								//�罻�߳���

	void __LoadFriends();														//��ȡ���ѱ�
	void __SendFriendInfo();													//���ͺ�����Ϣ
	void __AddFriendsOrBlack(const CMessage& msg);								//��Ӻ��ѻ������
	void __DeleteFriendsOrBlack(const CMessage& msg);							//ɾ�����ѻ������
	void __SetContactsProtocol(int pid, pto_SOCIALStruct_FriendInfo* pto);		//������ϵ��Э��
	void __ChatMessage(const CMessage& msg);									//������Ϣ

	void __CreateGuild(const CMessage& msg);									//��������
	void __GetGuildInfo();														//��ȡ������Ϣ
	void __FindGuild(const CMessage& msg);										//Ѱ�ҹ���
	void __ApplyGuild(const CMessage& msg);										//���빫��
	void __AgreeApply(const CMessage& msg);										//ͬ����빫��
	void __ChangeGuildAccess(const CMessage& msg);								//�ı乫��ְλ
	void __QuitGuild();															//�˳�����
	void __GuildKick(const CMessage& msg);										//��������
	void __CessionGuide(const CMessage& msg);									//ת�ù���
	void __GetGuildName(const CMessage& msg);									//��ȡ������
	void __GuildContribute();													//�������
	void __GuildEditNotification(const CMessage& msg);							//��ͨ��
#pragma endregion

#pragma region ����
	int	 room_id_{ 0 };															//����ID

	void __RoomCreate(const CMessage& msg);										//��������
	void __RoomList(const CMessage& msg);										//��ʾ�����б�
	void __RoomJoin(const CMessage& msg);										//���뷿��
	void __RoomQuit();															//�˳�����
	void __RoomLock(const CMessage& msg);										//��ס����
	void __RoomKick(const CMessage& msg);										//��������
	void __RoomReady(const CMessage& msg);										//׼��
	void __RoomChangePos(const CMessage& msg);									//����λ��
	void __RoomStart();															//���俪ʼս��
	void __StartWar();															//��ʼ����ս
#pragma endregion
};
