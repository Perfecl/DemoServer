#pragma once

class CMission
{
public:
	static const int kRewardMissionNum{ 5 };

public:
	CMission(CPlayer& player);
	~CMission();

	void					ProcessMessage(const CMessage& msg);

	void					SendInitialMessage();

	void					SaveToDatabase();

	void					ImplementMission(EMissionTargetType target_type, int target_id, int target_id_ex);			//完成任务

	inline int				main_progress() const { return main_progress_; }
	void					SetMission(int progress);
	bool					IsAcceptMission(int mission_id);
	bool					IsCompleteMission(int mission_id);
	std::map<int, int>*		GetAcceptMission(){ return &accept_mission_; }

	inline int				stage_normal_progress() const { return stage_normal_progress_; }
	inline int				stage_hard_progress() const { return stage_hard_progress_; }
	inline int 				stage_nightmare_progress() const { return stage_nightmare_progress_; }
	inline int 				stage_elite_progress() const { return stage_elite_progress_; }
	inline int 				stage_multi_progress() const { return stage_multi_progress_; }

	void					SetStageProcess(int level, StageType type);

private:
	CPlayer&			player_;

	int					main_progress_{ 0 };																		//主线进度

	std::map<int, int>	accept_mission_;																			//已接任务
	std::vector<int>	complete_branch_;																			//已完成支线

	int					stage_normal_progress_{ 0 };																//普通关卡进度
	int 				stage_hard_progress_{ 0 };																	//困难关卡进度
	int 				stage_nightmare_progress_{ 0 }; 															//噩梦关卡进度
	int 				stage_elite_progress_{ 0 };																	//精英关卡进度
	int 				stage_multi_progress_{ 0 };																	//多人关卡进度

	void		__LoadFromDatabase();

	void		__AcceptMission(const CMessage &msg);
	void		__CommitMission(const CMessage &msg);

	void		__GetReward(const CMissionTP* mission_templete);
	void		__GetOffer(const CMissionTP* mission_templete);
	void		__SendCountUpdate();
	void		__JudgeMission();
	void		__JudgeEquipLvUpTimes(int target_num);

	void		__UpdateMissionToClient();
};

