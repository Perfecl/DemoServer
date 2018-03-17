#pragma once

class CTown
{
public:
	CTown(int id);
	~CTown();

	ENUM_EnterTownResult	EnterTown(SPPlayer player);																								//进入城镇
	void					LeaveTown(int pid);																										//离开城镇
	void					TownMove(int pid, int x, int y);																						//城镇移动

	SPPlayer				FindPlayerInTown(int pid);																								//查找城镇中的玩家

	void					SendAllPlayersInTown(const std::string& content, unsigned char protocol_type, int protocol_id, int except_pid);			//发送给所有城镇中玩家

	void					SetAllPlayersToProtocol(pto_TOWN_S2C_RES_EnterTown& pto);																//把所有玩家填入协议

	void					SetTop10Protocol(pto_PLAYER_S2C_RES_UpdateTownStageRank* pto);															//设置排行榜协议
	bool					AddTop10Player(int pid);

	inline void main_progress(int progress){ main_progress_ = progress; }
	inline int	town_id() const { return town_id_; }

private:
	const int	town_id_;											//城镇ID
	int			main_progress_{ 0 };								//主线进度

	std::map<int, SPPlayer>					players_in_town_;		//城镇中的玩家
	CCriticalSectionZ						lock_;					//线程锁

	std::vector<std::pair<int, time_t>>		pass_ranking_list_;		//过关排行榜

	void __LoadFromDatabase();										//从数据库中读取
};

