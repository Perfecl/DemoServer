#pragma once

//房间状态
enum class RoomState
{
	WAITING,			//等待中
	MATCHING,			//匹配中
	BATTLING			//战斗中
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

	int								JoinRoom(int pid, std::string password);								//加入房间
	void							QuitRoom(int pid);

	int								GetMaxNum();															//获得房间最大人数
	int								GetPlayerNum();															//获得房间现在人数
	void							KickPlayer(int pid, int target_index);									//踢掉玩家

	void							SetRoomIsLock(bool is_lock, const std::string& password, int pid);		//设置房间是否上锁
	void							Ready(int pid, bool is_ready);											//准备
	void							ChangePos(int pid, int target_index);									//交换位置
	void							CancelAllReady();														//取消所有准备

	void							Start(int pid);															//开始

	bool							IsOwner(int pid);														//是不是房主

	inline int						room_id() const { return room_id_; }
	inline BattleMode				room_mode() const { return room_mode_; }
	inline const std::string&		password() const { return room_password_; }

	inline void						name(const char* name){ room_name_.assign(name); }
	inline void						password(const char* password){ room_password_.assign(password); }
	inline void						map_id(int map_id){ room_map_id_ = map_id; }
	inline int						map_id() const { return room_map_id_; }

	inline bool						IsWaiting() const { return room_state_ == RoomState::WAITING; }

	inline void						SetRoomState(RoomState state){ room_state_ = state; }											//设置房间状态

	void	SetProtocol(pto_ROOM_STRUCT_RoomInfo* pto);																				//设置房间信息协议
	void	SetProtocol(pto_ROOM_STRUCT_RoomSimpleInfo* pto);																		//设置房间简单信息协议

	void	SendToAllPlayerInRoom(const std::string& content, unsigned char protocol_type, int protocol_id, int except_pid);		//发送消息给所有房间中的人

private:
	std::array<CRoomSlot, 6>	slots_;
	const int					room_id_;
	const BattleMode			room_mode_;									//战斗模式
	int							room_map_id_{ 0 };							//地图ID
	std::string					room_name_;									//房间名
	std::string					room_password_;								//房间密码
	RoomState					room_state_{ RoomState::WAITING };			//房间状态

	size_t						owner_slot_id_{ 0 };						//房主位置

	CCriticalSectionZ			lock_;										//线程锁

	CRoomSlot*					__FindSlotByPID(int pid);					//查找栏位
	CRoomSlot*					__FindCellByIndex(size_t index);			//查找栏位

	void						__StartBattle(int pid);						//开始战斗
	void						__MatchBattle(int pid);						//开始匹配
};

