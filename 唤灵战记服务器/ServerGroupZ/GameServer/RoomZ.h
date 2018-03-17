#pragma once

//����״̬
enum class RoomState
{
	WAITING,			//�ȴ���
	MATCHING,			//ƥ����
	BATTLING			//ս����
};


struct CRoomSlot
{
	int		solt_id_{ 0 };
	int		pid_{ 0 };
	bool	is_enable_{ true };
	bool	is_ready_{ false };
};

class CRoomZ
{
public:
	CRoomZ(int room_id, BattleMode mode, int owner_pid);
	~CRoomZ();

	int								JoinRoom(int pid, std::string password);								//���뷿��
	void							QuitRoom(int pid);

	int								GetMaxNum();															//��÷����������
	int								GetPlayerNum();															//��÷�����������
	void							KickPlayer(int pid, int target_index);									//�ߵ����

	void							SetRoomIsLock(bool is_lock, const std::string& password, int pid);		//���÷����Ƿ�����
	void							Ready(int pid, bool is_ready);											//׼��
	void							ChangePos(int pid, int target_index);									//����λ��
	void							CancelAllReady();														//ȡ������׼��

	void							Start(int pid);															//��ʼ

	bool							IsOwner(int pid);														//�ǲ��Ƿ���

	inline int						room_id() const { return room_id_; }
	inline BattleMode				room_mode() const { return room_mode_; }
	inline const std::string&		password() const { return room_password_; }

	inline void						name(const char* name){ room_name_.assign(name); }
	inline void						password(const char* password){ room_password_.assign(password); }
	inline void						map_id(int map_id){ room_map_id_ = map_id; }
	inline int						map_id() const { return room_map_id_; }

	inline bool						IsWaiting() const { return room_state_ == RoomState::WAITING; }

	inline void						SetRoomState(RoomState state){ room_state_ = state; }											//���÷���״̬

	void	SetProtocol(pto_ROOM_STRUCT_RoomInfo* pto);																				//���÷�����ϢЭ��
	void	SetProtocol(pto_ROOM_STRUCT_RoomSimpleInfo* pto);																		//���÷������ϢЭ��

	void	SendToAllPlayerInRoom(const std::string& content, unsigned char protocol_type, int protocol_id, int except_pid);		//������Ϣ�����з����е���

private:
	std::array<CRoomSlot, 6>	slots_;
	const int					room_id_;
	const BattleMode			room_mode_;									//ս��ģʽ
	int							room_map_id_{ 0 };							//��ͼID
	std::string					room_name_;									//������
	std::string					room_password_;								//��������
	RoomState					room_state_{ RoomState::WAITING };			//����״̬

	size_t						owner_slot_id_{ 0 };						//����λ��

	CCriticalSectionZ			lock_;										//�߳���

	CRoomSlot*					__FindSlotByPID(int pid);					//������λ
	CRoomSlot*					__FindCellByIndex(size_t index);			//������λ

	void						__StartBattle(int pid);						//��ʼս��
	void						__MatchBattle(int pid);						//��ʼƥ��
};

