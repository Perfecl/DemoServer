drop schema if exists db_game;
create schema if not exists db_game character set utf8;
use db_game;

create table if not exists tb_gift
(
	`key` 					varchar(18) primary key,						#礼包码
    `type`					int not null,									#礼包类型
    `pid`					int not null default 0							#被哪个用户领取
);

#游戏世界表
create table if not exists tb_game_world
(
	`sid`  					int primary key,								#服务器ID
    `speed_stage_id`		int not null,									#本周竞速赛关卡ID
    `speed_stage_max_level` int not null,									#本周竞速关卡最高等级
	`last_close_time`		timestamp not null default current_timestamp,	#服务器最后关闭时间
	`create_time`			timestamp not null default current_timestamp,	#服务器开服时间
    `event_flag`			int not null default 0							#时间标志
);

#祈福表
create table if not exists tb_trade
(
	`sid`  					int primary key,								#服务器ID
    `trade_card1`			int not null,									#祈福卡牌1
	`trade_card2`			int not null,									#祈福卡牌2
	`trade_card3`			int not null,									#祈福卡牌3
	`trade_card4`			int not null,									#祈福卡牌4
	`trade_card5`			int not null,									#祈福卡牌5
	`trade_card6`			int not null,									#祈福卡牌6
	`trade_card7`			int not null,									#祈福卡牌7
	`trade_card8`			int not null,									#祈福卡牌8
    `trade_card9`			int not null,									#祈福卡牌9
    `trade_reward1`			bigint not null,								#祈福卡牌奖励1
    `trade_reward2`			bigint not null,								#祈福卡牌奖励2
	`trade_reward3`			bigint not null,								#祈福卡牌奖励3
     constraint foreign key(`sid`) references tb_game_world(`sid`)
);

#公会表
create table if not exists tb_guild
(
    `guild_id`  			int primary key auto_increment,					#公会ID
    `sid`  					int not null,									#服务器ID
    `name` 					varchar(16) not null,							#公会名	
    `level` 				int not null default 1,							#公会等级
    `exp`					int not null default 0,							#公会经验
    `fund` 					int not null default 0, 						#公会资金
    `notice` 				varchar(256),									#公会告示
    `create_time`			timestamp not null default current_timestamp,	#创建时间
    `apply_list`			varchar(2048),									#申请入会列表
    `is_dissolve`			bool not null default false,					#是否解散					
    constraint foreign key(`sid`) references tb_game_world(`sid`)
);

#玩家表
create table if not exists tb_player
(
	`pid` 					int primary key auto_increment,					#玩家ID
	`sid`					int not null,									#服务器ID
	`username`				varchar(36) not null,							#帐号ID	
	`name`					varchar(14) not null,							#名字
	`sex`					bool not null,									#性别 (女 = false, 男 = true)
    `level`					int not null default 1,							#等级
    `exp`					bigint not null default 0,						#经验
    `silver`				bigint not null default 0,						#银币
    `gold`					int	not null default 0,							#金币
    `honor`					bigint not null default 0,						#荣誉
	`reputation`			int not null default 0,							#声望
	`stamina`				int not null default 200,						#精力
    `vip_value`				int not null default 0,							#vip值(累计充值金币)
    `guild_id`				int not null default 0,							#公会id
	`using_title`			int not null default 0,							#玩家正在使用的称号
	`have_titles`			varchar(2048),									#玩家拥有的称号
    `recents_friend`		varchar(2048),									#最近联系人
    `town_id`				int not null default 1,							#所在的城镇ID
    `town_x`				int not null default 100,						#城镇x坐标
    `town_y`				int not null default 100,						#城镇y坐标
    `access`				int not null default 0,							#权限
    `offline_time`			timestamp not null default current_timestamp,	#离线时间
    `create_time`			timestamp not null default current_timestamp,	#帐号创建时间
    `frozen_time`			timestamp not null default current_timestamp,	#帐号封停截至时间
	unique(`sid`,`name`),
    unique(`sid`,`username`),
    constraint foreign key(`sid`) references tb_game_world(`sid`)
);

#练功台
create table if not exists tb_exercise_platform
(
	`id`					int not null,									#练功台ID
    `sid`					int not null,									#服务器ID
    `pid`					int not null default 0,							#玩家ID
    `start_time` 			bigint not null default 0,						#开始时间
    `level`					int not null,									#等级
    `exp_mul`       		float not null,									#经验倍率
    primary key(`id`,`sid`),
    constraint foreign key(`sid`) references tb_game_world(`sid`)
);

create table if not exists tb_player_exercise_exp
(
	`pid`					int primary key,								#玩家ID
    `exp` 					bigint not null default 0,						#玩家离线经验
    `exercise_time`			int not null default 0,							#玩家领取次数
     constraint foreign key(`pid`) references tb_player(`pid`)
);

#玩家奖励表
create table if not exists tb_player_award
(
	`pid` 					int primary key,								#玩家ID
    `meditation_time`		timestamp not null default current_timestamp,	#上次领取冥想时间
	`online_day_award`		int not null default 0,							#登陆天数奖励领取进度
	`vip_level_award`		int not null default 0,							#vip等级奖励领取进度
	`normal_box`			int not null default 0,							#普通宝箱领取进度
	`hard_box`				int not null default 0,							#困难宝箱领取进度
	`nightmare_box`			int not null default 0,							#噩梦宝箱领取进度
	`speed_stage_record`	int not null default 0,							#竞速赛最好成绩
    constraint foreign key(`pid`) references tb_player(`pid`)
);

#需要定时重置列表
create table if not exists tb_player_need_reset
(
	`pid` 					int primary key,								#玩家ID
    `snake_award_d`			int not null default 0,							#贪吃蛇奖励
    `puzzle_award_d`		int not null default 0,							#拼图奖励
	`buy_stamina_times_d`	int not null default 0,							#购买的体力次数
    `trade_draw_times_d`	int not null default 12,						#祈福剩余抽牌次数
	`trade_got_award_w`		int not null default 0,							#祈福本周获得奖励记录
	`trade_cards`			varchar(32),									#祈福未抽卡牌
    `trade_hands`			varchar(32),									#祈福手上的卡牌
	`rm_refresh`			timestamp not null default current_timestamp,	#悬赏任务最后刷新时间
	`rm_times`				int not null default 0,							#悬赏任务已完成次数
	`rm_buy_times`			int not null default 0,							#悬赏任务购买次数
    `elite_already_done`	varchar(2048),									#精英关卡每天战斗过的关卡
	`elite_buy_times`		int not null default 0,							#精英关卡购买次数
	`multi_already_times`	varchar(2048),									#多人关卡战斗过的关卡
	`multi_buy_times`		varchar(2048),									#多人关卡购买过的关卡
    `speed_times`			int not null default 5,							#竞速赛每天剩余次数
    `speed_today_best_time`	int not null default 0,							#竞速本日最好成绩
    `is_got_battle_reward`	bool not null default false,					#今日剩余获得奖励次数
    `week_war_times`		int not null default 0,							#周争霸战斗次数
	`week_1v1_times`		int not null default 0,							#周1v1战斗次数
	`week_3v3_times`		int not null default 0,							#周3v3战斗次数
	`surplus_times`			int not null default 10,						#剩余挑战次数
	`escort_times`			int not null default 3,							#剩余押镖次数
	`rob_times`				int not null default 5,							#剩余抢劫次数
	`protect_times` 		int not null default 0,							#已经保护次数
    `offline_buy_times`		int not null default 0,							#离线战斗购买次数
    `today_attendance`		bool not null default false,					#今日是否签到过
    `vip_every_day` 		bool not null default false,					#每日vip奖励
    `guild_contribute`		int not null default 0,							#今日工会 捐献次数
	`online_box_award`		int not null default 1,							#在线宝箱领取进度
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#悬赏任务表
create table if not exists tb_player_rewardMission
(
	`pid` 					int,											#玩家ID			
	`rmid`					int,											#悬赏任务ID
	`rank`					int not null default 0,							#星级								
	`type`					int not null default 0,							#类型
	`target_id`				int not null default 0,							#任务目标ID
	`target_num`			int not null default 0,							#任务目标数量
	`state`					int not null default 0,							#任务状态
	primary key(pid,rmid),
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#玩家物品
create table if not exists tb_player_item
(
	`pid`					int,									#玩家ID
	`id`					int not null,							#物品ID	
	`quantity`				int,									#数量
	`time`					bigint,									#拾取时间		
	`is_temp`				bool,									#是不是临时的
	primary key(pid,id,is_temp),
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#玩家装备
create table if not exists tb_player_equip
(
	`pid`					int,									#玩家ID
	`id`					int not null,							#装备ID
	`level`					int not null default 0,					#强化等级
	`quality`				int not null default 0,					#品质(颜色)
	`suit_id`				int not null default 0,					#套装ID
	`enchanting1`			int not null default 0,					#附魔1
	`enchanting1_value`		float not null default 0,				#附魔1数值
	`enchanting2`			int not null default 0,					#附魔2
	`enchanting2_value`		float not null default 0,				#附魔2数值
	`enchanting2_is_active` bool not null default false,			#附魔2是否激活
	`gem`					varchar(4096),							#宝石
	`time`					bigint not null default 0, 				#拾取时间
	`loaction`				int not null default 0,					#位置
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#玩家军队
create table if not exists tb_player_army
(
	`pid`					int primary key,						#玩家ID
	`tech_atk`				int not null default 0,					#攻击科技
	`tech_matk`				int not null default 0,					#魔攻科技
	`tech_def`				int not null default 0,					#防御科技
	`tech_mdef`				int not null default 0,					#魔防科技
	`tech_hp`				int not null default 0,					#血量科技
	`is_in_practice_cd`		bool not null default false,			#是否进入训练CD
	`parctice_seconds`		int not null default 0,					#英雄累计训练CD时间
	`last_parctice_time`	bigint not null default 0,				#英雄最后训练时间
	`rune_page`				int not null default 0,					#符文页开放进度
	`rune_energy`			int not null default 0,					#符文页能量
	`hero_intimacy`			varchar(4096),							#英雄亲密度
	`hero1`					int not null default 8,					#英雄1
	`hero2`					int not null default 0,					#英雄2
	`hero3`					int not null default 0,					#英雄3
	`soldier1`				int not null default 1,					#士兵1
	`soldier2`				int not null default 0,					#士兵2
	`soldier3`				int not null default 0,					#士兵3
	`soldier4`				int not null default 0,					#士兵4
	`soldier5`				int not null default 0,					#士兵5
	`soldier6`				int not null default 0,					#士兵6
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#玩家士兵
create table if not exists tb_player_soldier
(
	`pid`					int,									#玩家ID
	`sodlier_id`			int,									#士兵ID
	`soul_lv`				int not null default 0,					#士兵灵能等级
	`soul_exp`				int not null default 0,					#士兵灵能层数
	`train`					int not null default 0,					#士兵训练等级
	primary key(pid,sodlier_id),
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#玩家英雄
create table if not exists tb_player_hero
(
	`pid`					int,		
	`hid`					int,
	`level`					int not null default 1,
	`exp`					int not null default 0,
	`quality`				int not null default 1,
	`safety_times`			int not null default 0,
	`train_str`				int not null default 0,
	`train_cmd`				int not null default 0,
	`train_int`				int not null default 0,
	`today_affair` 			bool not null default false,
	`runes`					varchar(2048),
	primary key(`pid`,`hid`),
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#玩家时装
create table if not exists tb_player_fashion
(
	`pid`					int,
	`id`					int not null,
	`get_time`				bigint not null default 0,
	`due_time`				bigint not null default 0,
	`is_on_body`			bool not null default false,
	primary key(pid,id),
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#玩家邮件
create table if not exists tb_player_mail
(
	`pid`					int, 									#玩家ID
	`model_id`				int not null default 1,					#内容模版ID
	`param0`				int not null default 0,					#参数0
	`param1`				int not null default 0,					#参数1
	`name`					varchar(50) default '',					#名称
	`time`					bigint not null,						#接收时间
	`is_read`				bool not null default false,			#是否读取过
	`reward_hero`			int not null default 0,					#奖励英雄
	`reward_soldier`		int not null default 0,					#奖励士兵
	`reward_items`			varchar(4096),							#奖励物品
	`reward_equips`			varchar(2048),							#奖励装备
	`reward_title`			int not null default 0,					#奖励称号
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#玩家内政
create table if not exists tb_player_internal
(
	`pid`					int primary key,
	`levy_times`			int not null default 5,					#剩余征收次数
	`gold_point`			int not null default 0,					#点金使用次数
	`diggings_times`		int not null default 5,					#矿山剩余使用次数
	`diggings_last_use`		bigint not null default 0,				#矿山最后使用时间
	`diggings_hero_id`		int not null default 0,					#矿山使用英雄ID
	`diggings_level`		int not null default 1,					#矿山等级
	`train_times`			int not null default 5,					#训练场剩余使用次数
	`train_last_use`		bigint not null default 0,				#训练场后使用时间
	`train_hero_id`			int not null default 0,					#训练场用英雄ID
	`train_level`			int not null default 1,					#训练场等级
	`guild_times`			int not null default 5,					#冒险行会剩余使用次数
	`guild_last_use`		bigint not null default 0,				#冒险行会后使用时间
	`guild_hero_id`			int not null default 0,					#冒险行会用英雄ID
	`guild_level`			int not null default 1,					#冒险行会等级
	`castle_atk`			int not null default 15,				#主堡物理攻击
	`castle_matk`			int not null default 15,				#主堡魔法攻击
	`castle_hp`				int not null default 15,				#主堡血量
	constraint foreign key(`pid`) references tb_player(`pid`)
);

create table if not exists tb_player_offline
(
	`pid`					int primary key,						#玩家ID
	`win_streak`			int not null default 0,					#连胜次数
	`last_time`				bigint not null default -1,				#最后挑战时间
	`dartcar_lv`			int not null default 0,					#镖车等级
	`protect_mode`			int not null default 0,					#保护模式
	`bodyguard`				int not null default 0,					#保镖ID
	`last_rob_time`			bigint not null default 0,				#上次打劫时间
	constraint foreign key(pid) references tb_player(pid)
);


#玩家战斗记录
create table if not exists tb_player_record
(
	`pid`					int not null,							#玩家ID
    `type`					int not null,							#类型(0.争霸 1.1v1 2.3v3)
    `point`					int not null default 1500,				#积分
	`S`						int not null default 0,					#S获取次数		
    `A`						int not null default 0,					#A获取次数	
    `B`						int not null default 0,					#B获取次数	
    `C`						int not null default 0,					#C获取次数	
    `win`					int not null default 0,					#胜利场次
    `lose`					int not null default 0,					#失败场次
    primary key(`pid`,`type`),
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#玩家练功台
create table if not exists tb_player_exercise
(
	`pid`					int primary key,								#玩家ID
    `contend_times`			int not null default 5,							#今日抢夺次数
    `buy_contend`			int not null default 0,							#购买抢夺次数
    `exp_pill_times`		int not null default 10,						#抢夺经验次数
    `today_exercise_time`	int not null default 0,							#今日累计时间
    `today_exercise_exp`	bigint not null default 0,						#今日累计经验
    `last_exercise_time`	int not null default 0,							#昨日累计时间
    `last_exercise_exp`		bigint not null default 0,						#昨日累计经验
    `exp_box`				bigint not null default 0,						#经验盒
    `exp_pill_cd`			bigint not null default 0,						#上次采集时间
    `contend_cd`			bigint not null default 0,						#上次抢夺时间
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#玩家任务表
create table if not exists tb_player_mission
(
	`pid`					int primary key,								#玩家ID
	`main_progress`			int not null default 0,							#主线进度
	`accept_mission` 		varchar(2048),									#主线任务
	`complete_branch`		varchar(2048),									#完成的支线任务
    `stage_normal`			int not null default 0,							#普通关卡进度
	`stage_hard`			int not null default 0,							#困难关卡进度
	`stage_nightmare` 		int not null default 0,							#噩梦关卡进度
	`stage_elite`			int not null default 0,							#精英关卡进度
	`stage_multi`			int not null default 0,							#多人关卡进度
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#玩家好友表
create table if not exists tb_player_friend
(
	`pid` 					int not null,									#玩家ID
    `target_pid` 			int not null,									#对方玩家ID
    `is_black`				bool not null,									#是否是黑名单
    primary key(`pid`,`target_pid`),
	constraint foreign key(`pid`) references tb_player(`pid`),
    constraint foreign key(`target_pid`) references tb_player(`pid`)
);

#公会成员表
create table if not exists tb_guild_member
(
	`guild_id`  			int not null,									#公会ID
    `pid`					int not null,									#玩家ID
    `position`				int not null default 0,							#职务(0.普通 1.官员 2.副会长 3.会长)
    `contribution`			int not null default 0,							#贡献度
    primary key(guild_id,pid),
    constraint foreign key(`guild_id`) references tb_guild(`guild_id`),
    constraint foreign key(`pid`) references tb_player(`pid`)
);

#首破记录表
create table if not exists tb_town_ranking
(
	`town_id`				int not null,									#城镇ID
    `sid`					int not null,									#服务器ID
    `pid`					int not null,									#玩家ID
    `pass_time`				timestamp not null default current_timestamp,	#过关时间
    primary key(`town_id`,`sid`,`pid`),
    constraint foreign key(`sid`) references tb_game_world(`sid`),
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#竞速赛排行榜
create table if not exists tb_speed_stage_ranking
(
	`sid`					int not null,									#服务器ID
    `pid`					int not null,									#玩家ID
    `time`			 		int not null,									#过关时间
    `hero1`					int not null default 0,							#英雄1
	`hero2`					int not null default 0,							#英雄2
	`hero3`					int not null default 0,							#英雄3
	`soldier1`				int not null default 0,							#士兵1
	`soldier2`				int not null default 0,							#士兵2
	`soldier3`				int not null default 0,							#士兵3
	`soldier4`				int not null default 0,							#士兵4
	`soldier5`				int not null default 0,							#士兵5
	`soldier6`				int not null default 0,							#士兵6
	primary key(`sid`,`pid`),
    constraint foreign key(`sid`) references tb_game_world(`sid`),
	constraint foreign key(`pid`) references tb_player(`pid`)
);

#离线战斗排行榜
create table if not exists tb_offline_ranking
(
	`sid`					int,									#服务器ID
	`rank`					int,									#排名
	`pid`					int,									#玩家ID
	primary key(sid,rank),
	constraint foreign key(sid) references tb_game_world(sid)
);


#玩家充值订单表
create table if not exists tb_order_form
(
	`order_id`				varchar(30) primary key,
	`pid`					int	not null,
	`point`					int not null,
	`rmb`					int not null,
	`pay_time`				timestamp not null default current_timestamp,
	`is_get`				bool not null default false,
    `src` 					int not null default 0,
	constraint foreign key(pid) references tb_player(pid)
);

#活动补偿等领取表
create table if not exists tb_operate_form
(
	`pid`					int	not null primary key,
	`oid`					int not null,
	`times`					int not null,
	`get_time`				timestamp not null default current_timestamp,
	constraint foreign key(pid) references tb_player(pid)
);

#创建玩家存储过程
drop procedure if exists sp_create_player;
delimiter &&
create procedure sp_create_player(IN `sid_` int ,IN `username_` varchar(36) ,IN `name_` blob ,IN `sex_` bool ,OUT `pid_` int) reads sql data modifies sql data
begin
if(select count(*) = 0 from tb_player where (tb_player.sid = `sid_` and tb_player.`name` = `name_`) or (tb_player.sid = `sid_` and tb_player.`username` = `username_`)) then
insert into tb_player values(default,`sid_`,`username_`,`name_`,`sex_`, default,default,default,default,default,default,default,default,default,default,'','',default,default,default,default,default,default,default);
set `pid_` = last_insert_id();
insert into tb_player_award values(`pid_`,default,default,default,default,default,default,default);
insert into tb_player_need_reset values(`pid_`,default,default,default,default,default,'','',default,default,default,'',default,'','',default,default,default,default,default,default,default,default,default,default,default,default,default,default,default);
insert into tb_player_mission values(`pid_`,default,'','',default,default,default,default,default);
#军队
insert into tb_player_army values(`pid_`,default,default,default,default,default,default,default,default,default,default,'',default,default,default,default,default,default,default,default,default);
insert into tb_player_hero values(`pid_`,8,default,default,default,default,default,default,default,default,'');
insert into tb_player_soldier values(`pid_`,1,default,default,default);
#内政
insert into tb_player_internal values(`pid_`,default,default,default,default,default,default,default,default,default,default,default,default,default,default,default,default,default);
#战斗记录
insert into tb_player_record values(`pid_`,0,default,default,default,default,default,default,default);
insert into tb_player_record values(`pid_`,1,default,default,default,default,default,default,default);
insert into tb_player_record values(`pid_`,2,default,default,default,default,default,default,default);
#练功台
insert into tb_player_exercise values(`pid_`,default,default,default,default,default,default,default,default,default,default);
#离线
insert into tb_player_offline values(`pid_`,default,default,default,default,default,default);
else
set `pid_` = 0;
end if;
end &&
delimiter ;

#创建公会存储过程
drop procedure if exists sp_create_guild;
delimiter &&
create procedure sp_create_guild(IN `sid_` int ,IN `name_` varchar(16) ,IN `pid_` int ,OUT `guild_id_` int) reads sql data modifies sql data
begin
if(select tb_guild.guild_id from tb_guild where tb_guild.`name` = `name_` and tb_guild.`sid` = `sid_`) then
set `guild_id_` = -4;
else
insert into tb_guild values(default,`sid_`,`name_`,default,default,default,'',default,'',default);
set `guild_id_` = last_insert_id();
insert into tb_guild_member values(`guild_id_`,`pid_`,3,default);
end if;
end &&
delimiter ;

