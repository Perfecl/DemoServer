#pragma once

__interface IPlayerData
{
	virtual int				pid();
	virtual int				sid();
	virtual const char*		name();
	virtual bool			sex();
	virtual int				town_id();
	virtual int				level();
	virtual int				guild_id();
	virtual time_t			offline_time();

	virtual int				clothes();

	virtual int				reputation();
	virtual int				using_title();

	virtual int				bodygurand();

	virtual const CHeroCard*				hero(int index);
	virtual std::vector<const CEquip*>		hero_equip(int hero_id);
	virtual int								technology(int index);
	virtual int								speed_best_rank();
	virtual const BattleRecord&				battle_record(BattleRecordType type);
	virtual int								offline_battle_win_streak();
	virtual int								exercise_time();
	virtual int								battle_week_times(BattleRecordType type);
	virtual time_t							offline_last_time();	//¿Îœﬂ’Ω∂∑CD

	virtual std::string						guild_name();
	virtual int								guild_postion();
	
	virtual void							guild_id(int guild_id);
};

