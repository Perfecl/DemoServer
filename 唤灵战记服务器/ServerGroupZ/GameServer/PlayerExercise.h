#pragma once

class CPlayerExercise
{
public:
	CPlayerExercise(CPlayer& player);
	~CPlayerExercise();

	void RoutineReset();

	void ProcessMessage(const CMessage& msg);
	void SaveToDatabase();
	void SendInitialMessage();

	int contend_times() { return contend_times_; }
	int buy_contend() { return buy_contend_; }
	int exp_pill_times() { return exp_pill_times_; }
	int	exercise_time() { return today_exercise_time_; }
	__int64 exercise_exp() { return today_exercise_exp_; }
	int last_exercise_time() { return last_exercise_time_; }
	__int64 last_exercise_exp() { return last_exercise_exp_; }
	__int64 exp_box() { return exp_box_; }

	void AddContendTimes(){ contend_times_++; }
	void AddBuyContend(){ buy_contend_++; }
	void AddContendExp(){ exp_pill_times_++; }
	void AddExerciseTime(){ today_exercise_time_++; }
	void AddexerciseExp(__int64 exp){ today_exercise_exp_ += exp; }

	void set_last_exercise_time(){ last_exercise_time_ = today_exercise_time_; }
	void set_last_exercise_exp(){ last_exercise_exp_ = today_exercise_exp_; }

	void ChangeExpBox(__int64 exp){ exp_box_ += exp; }
	void ClearExpBox(){ exp_box_ = 0; }

	bool in_exercise_platform() { return in_exercise_platform_; }
	void SendDownFromPlatform(int type, int pid, time_t time);

private:
	CPlayer&			player_;
	int					contend_times_{ 0 };						//今日抢夺次数
	int					buy_contend_{ 0 };							//购买抢夺次数
	int					exp_pill_times_{ 0 };						//抢夺经验次数
	int					today_exercise_time_{ 0 };					//今日累计时间
	__int64				today_exercise_exp_{ 0 };					//今日累计经验

	int					last_exercise_time_{ 0 };					//昨日累计时间
	__int64				last_exercise_exp_{ 0 };					//昨日累计经验
	__int64				exp_box_{ 0 };								//经验盒

	time_t				exp_pill_cd_{ 0 };							//上次采集时间
	time_t				contend_cd_{ 0 };							//上次抢夺时间

	bool in_exercise_platform_{ false };							//是否在练功台界面

	void __LoadFromDatabase();

	void __ContendPlatform(const CMessage& msg);
	void __TakePlatformExp();
	void __TakeExpPill(const CMessage& msg);
	void __BuyContendTimes();
	void __EnterExercisePlatform();
	void __ClearExercisePlatformCD(const CMessage& msg);
};
