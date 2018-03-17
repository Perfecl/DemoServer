CREATE TABLE IF NOT EXISTS `role_name` (
  `server` int(11) UNSIGNED NOT NULL COMMENT '服务器ID',
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin' COMMENT '第三方平台ID',
  `name` varchar(64) NOT NULL COLLATE 'utf8_bin' COMMENT '玩家名',
  `uid` bigint(20) NOT NULL COMMENT '玩家UID',
  UNIQUE INDEX (`server`,`name`),
  PRIMARY KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

CREATE TABLE IF NOT EXISTS `army` (
  `army_id` BIGINT(20) NOT NULL COMMENT '军团ID',
  `army_name` VARCHAR(64) NOT NULL COMMENT '军团名',
  `server` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `avatar` INT(11) NULL DEFAULT '0' COMMENT '军团头像',
  `level` INT(11) NULL DEFAULT '0' COMMENT '军团等级',
  `exp` BIGINT(20) NULL DEFAULT '0' COMMENT '军团经验',
  `master_id` BIGINT(20) NULL DEFAULT '0' COMMENT '军团长',
  `master_name` VARCHAR(64) NULL DEFAULT NULL COMMENT '军团长名字',
  `announcement1` VARCHAR(256) NULL DEFAULT NULL COMMENT '军团公告',
  `announcement2` VARCHAR(256) NULL DEFAULT NULL COMMENT '军团公告',
  `army_log` VARCHAR(4096) NULL DEFAULT NULL COMMENT '军团日志(最近20条)',
  `army_skill` VARCHAR(128) NULL DEFAULT NULL COMMENT '军团技能上限',
  `donate_count` INT(11) NULL DEFAULT '0' COMMENT '军团捐献次数',
  `donate_time` INT(11) NULL DEFAULT '0' COMMENT '军团捐献时间戳',
  `donate_value` INT(11) NULL DEFAULT '0' COMMENT '军团捐献值(活跃度)',
  `army_shop` VARCHAR(256) NULL DEFAULT NULL COMMENT '军团刷新商店',
  `buy_record` VARCHAR(4096) NULL DEFAULT NULL COMMENT '军团刷新商店购买记录',
  `shop_refresh_time` BIGINT(20) NULL DEFAULT '0' COMMENT '公会商店上次刷新时间',

  PRIMARY KEY (`army_id`),
  UNIQUE INDEX `army_name` (`army_name`),
  INDEX `server` (`server`)
) COLLATE='utf8_general_ci' ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `army_apply` (
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `army_id` BIGINT(20) NOT NULL COMMENT '军团ID',
  `server_id` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `name` VARCHAR(64) NULL DEFAULT NULL COMMENT '玩家名称',
  `level` INT(11) NULL DEFAULT NULL COMMENT '玩家等级',
  `vip_level` INT(11) NULL DEFAULT NULL COMMENT 'VIP等级',
  `fight` BIGINT(20) NULL DEFAULT NULL COMMENT '战斗力',
  `avatar` INT(11) NULL DEFAULT NULL COMMENT '玩家头像',

  UNIQUE INDEX `player_id_army_id` (`army_id`, `player_id`),
  INDEX `server_id` (`server_id`)
) COLLATE='utf8_general_ci' ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `army_member` (
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `army_id` BIGINT(20) NOT NULL COMMENT '军团ID',
  `server` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `name` VARCHAR(64) NOT NULL COMMENT '玩家名字',
  `position` INT(11) NOT NULL COMMENT '职位',
  `level` INT(11) NOT NULL COMMENT '等级',
  `vip_level` INT(11) NOT NULL COMMENT 'VIP等级',
  `fight` BIGINT(20) NOT NULL COMMENT '战斗力',
  `army_exp` INT(11) NOT NULL COMMENT '累计军团贡献',
  `today_exp` INT(11) NOT NULL COMMENT '今日祭天贡献',
  `army_update_time` INT(11) NOT NULL COMMENT '更新时间',
  `avatar` INT(11) NULL DEFAULT '0' COMMENT '军团头像',
  
  PRIMARY KEY (`player_id`),
  INDEX `army_id` (`army_id`),
  INDEX `server` (`server`)
) COLLATE='utf8_general_ci' ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `player_0` (
  `uid` BIGINT(20) NOT NULL COMMENT '玩家唯一ID',
  `openid` VARCHAR(64) NOT NULL COMMENT '玩家帐号',
  `server` INT(11) UNSIGNED NOT NULL COMMENT '服务器编号',
  `channel` VARCHAR(64) NULL DEFAULT NULL COMMENT '渠道编号',
  `login_channel` VARCHAR(64) NULL DEFAULT NULL COMMENT '登录渠道',
  `device_id` VARCHAR(128) NULL DEFAULT NULL COMMENT '设备ID',
  `name` VARCHAR(64) NOT NULL COMMENT '角色名',
  `create_time` INT(11) NOT NULL COMMENT '创角时间',
  `last_login_time` INT(11) NOT NULL COMMENT '上次登录时间',
  `last_mail_id` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '上次阅读邮件ID',
  `last_server_mail_id` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '上次获取服务器邮件ID',
  `fresh_time` INT(11) NOT NULL DEFAULT '0' COMMENT '上次重置时间戳',
  `truce_time` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '免战时间',
  `login_days` SMALLINT NOT NULL DEFAULT '0' COMMENT '登录天数',
  `avatar` INT(11) NOT NULL DEFAULT '0' COMMENT '玩家头像',

  `level` SMALLINT(6) NOT NULL DEFAULT '0' COMMENT '玩家等级',
  `exp` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '玩家经验',
  `vip_level` SMALLINT(6) NOT NULL DEFAULT '0' COMMENT 'VIP等级',
  `vip_exp` INT(11) NOT NULL DEFAULT '0' COMMENT 'VIP经验',
  `coin` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '金币',
  `money` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '元宝/钻石',
  `oil` SMALLINT(6) NULL DEFAULT '0' COMMENT '汽油',
  `last_oil_time` INT(11) NULL DEFAULT '0' COMMENT '上次恢复汽油时间',
  `energy` SMALLINT(6) NULL DEFAULT '0' COMMENT '精力',
  `last_energy_time` INT(11) NULL DEFAULT '0' COMMENT '上次恢复精力时间',
  `prestige` INT(11) NULL DEFAULT '0' COMMENT '声望',
  `hero` INT(11) NULL DEFAULT '0' COMMENT '将魂',
  `plane` INT(11) NULL DEFAULT '0' COMMENT '神魂',
  `muscle` INT(11) NULL DEFAULT '0' COMMENT '威名',
  `exploit` INT(11) NULL DEFAULT '0' COMMENT '功勋',
  `union` INT(11) NULL DEFAULT '0' COMMENT '公会贡献',
  `rank_id` INT(11) NULL DEFAULT '0' COMMENT '军衔',

  `current_carrier_id` INT(11) NULL DEFAULT '0' COMMENT '当前装备的航母',
  `carrier_plane_attr` VARCHAR(512) NULL DEFAULT '' COMMENT '航母飞机增加的属性',
  `carrier_plane` VARCHAR(512) NULL DEFAULT '' COMMENT '航母上装备的飞机',
  `carrier_extra_damage` VARCHAR(512) NULL DEFAULT '' COMMENT '航母额外攻击',

  `research_hero_time` INT(11) NULL DEFAULT '0' COMMENT '研发船只时间戳',
  `free_hero_time` INT(11) NULL DEFAULT '0' COMMENT '免费研发船只时间戳',
  `research_hero_id` INT(11) NULL DEFAULT '0' COMMENT '研发出来的船只',
  `research_hero_item_count` INT(11) NULL DEFAULT '0' COMMENT '材料研发船只次数',
  `research_hero_money_count` INT(11) NULL DEFAULT '0' COMMENT '元宝研发船只次数',
  `research_hero_money_count2` INT(11) NULL DEFAULT '0' COMMENT '十连抽次数',
  `research_hero_rd_count` INT(11) NULL DEFAULT '0' COMMENT '研发次数',
  `research_hero_last_free_rd_time` INT(11) NULL DEFAULT '0' COMMENT '上次免费研发时间戳', 
  `research_hero_day_free_rd_count`  INT(11) NULL DEFAULT '0' COMMENT '今日免费研发抽取次数',

  `dstrike_level` INT(11) NULL DEFAULT '0' COMMENT '围剿BOSS等级',
  `dstrike_time` INT(11) NULL DEFAULT '0' COMMENT '征讨令最后恢复时间',
  `dstrike_merit` INT(11) NULL DEFAULT '0' COMMENT '围剿BOSS当日的功勋',
  `dstrike_damage` BIGINT(20) NULL DEFAULT '0' COMMENT '围剿BOSS当日的伤害',
  `dstrike_daily_award` BIGINT(20) UNSIGNED NULL DEFAULT '0' COMMENT '围剿BOSS每日功勋奖励',

  `sign_id` INT(11) NULL DEFAULT '0' COMMENT '签到ID',
  `sign_time` BIGINT(20) NULL DEFAULT '0' COMMENT '签到时间',

  `status` INT(11) NULL DEFAULT '0' COMMENT '当前账号状态',
  `status_time` INT(11) NULL DEFAULT '0' COMMENT '封号禁言时间戳',
  `flag` INT(11) NULL DEFAULT '0' COMMENT '账号类型(0普通玩家,1GM,2指导员,3福利号)',

  `dialog_id` VARCHAR(256) NULL DEFAULT '' COMMENT '新手引导ID',
  `client_flag` VARCHAR(1024) NULL DEFAULT '' COMMENT '客户端标识',
  `story_id` VARCHAR(256) NULL DEFAULT '' COMMENT '剧情ID',
  `total_recharge` INT(11) NULL DEFAULT '0' COMMENT '历史累计充值',

  PRIMARY KEY (`uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `item_0` (
  `uid` bigint(20) NOT NULL COMMENT '道具唯一ID',
  `player_id` bigint(20) NOT NULL COMMENT '玩家ID',

  `item_id` int(11) NOT NULL DEFAULT '0' COMMENT '道具ID',
  `item_count` int(11) NOT NULL DEFAULT '0' COMMENT '道具个数',
  `item_attr` varchar(512) NOT NULL DEFAULT '' COMMENT '道具属性',

  PRIMARY KEY (`player_id`, `uid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `mail_0` (
  `player_id` bigint(20) NOT NULL COMMENT '玩家ID',
  `mail_id` bigint(20) NOT NULL COMMENT '邮件唯一ID',
  `mail_time` int(11) NOT NULL COMMENT '邮件时间戳',
  `mail_type` tinyint(4) NOT NULL COMMENT '邮件类型',
  `mail_content` varchar(256) NOT NULL COMMENT '邮件内容',
  `mail_reward` varchar(256) NULL DEFAULT NULL COMMENT '邮件附件',

  PRIMARY KEY (`player_id`, `mail_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `hero_0` (
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `uid` BIGINT(20) NOT NULL COMMENT '船只唯一ID',
  `hero_id` INT(11) NOT NULL COMMENT '船只ID',
  `level` INT(11) NOT NULL COMMENT '船只等级',
  `exp` INT(11) NOT NULL COMMENT '船只经验',
  `grade` INT(11) NOT NULL DEFAULT '0' COMMENT '突破等级',
  `rand_attr` VARCHAR(128) NULL DEFAULT NULL COMMENT '随机属性',
  `rand_attr_1` VARCHAR(128) NULL DEFAULT NULL COMMENT '洗属性备份',
  `fate_level` INT(11) NULL DEFAULT '0' COMMENT '天命等级',
  `fate_exp` INT(11) NULL DEFAULT '0' COMMENT '天命经验',
  `fate_seed` INT(11) NULL DEFAULT '0' COMMENT '天命经验',
  `train_cost` INT(11) NULL DEFAULT '0' COMMENT '训练消耗',
  `fate_cost` INT(11) NULL DEFAULT '0' COMMENT '天命消耗',
  `relation` VARCHAR(128) NULL DEFAULT NULL COMMENT '已经激活的缘分',
  `wake_level` INT(11) NOT NULL DEFAULT '0' COMMENT '觉醒等级',
  `wake_item` VARCHAR(64) NULL DEFAULT NULL COMMENT '觉醒道具',
  `attr` varchar(10) NULL DEFAULT NULL COMMENT '',

  PRIMARY KEY(`player_id`, `uid`)
) ENGINE = InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `tactic_0` (
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `hero_pos` VARCHAR(64) NULL DEFAULT '0' COMMENT '英雄的位置',
  `battle_pos` VARCHAR(64) NULL DEFAULT NULL COMMENT '战斗时的位置',
  `support_pos` VARCHAR(64) NULL DEFAULT NULL COMMENT '援军的位置',
  `equip1` VARCHAR(128) NULL DEFAULT NULL COMMENT '位置1的装备',
  `equip2` VARCHAR(128) NULL DEFAULT NULL COMMENT '位置2的装备',
  `equip3` VARCHAR(128) NULL DEFAULT NULL COMMENT '位置3的装备',
  `equip4` VARCHAR(128) NULL DEFAULT NULL COMMENT '位置4的装备',
  `equip5` VARCHAR(128) NULL DEFAULT NULL COMMENT '位置5的装备',
  `equip6` VARCHAR(128) NULL DEFAULT NULL COMMENT '位置6的装备',
  `carrier` VARCHAR(1024) NULL DEFAULT NULL COMMENT '航母信息',
  `obtained_carriers` VARCHAR(1024) NULL DEFAULT NULL COMMENT '获得过的航母',
  `max_fight_attr` BIGINT(20) NULL DEFAULT '0' COMMENT '历史最高战斗力',
  `army_id` BIGINT(20) NULL DEFAULT '0' COMMENT '军团ID',
  `army_skill` VARCHAR(1024) NULL DEFAULT NULL COMMENT '军团技能',
  `leave_time` BIGINT(20) NULL DEFAULT '0' COMMENT '离开军团时间',

  PRIMARY KEY(`player_id`)
) ENGINE = InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `copy_0` (
  `player_id` BIGINT NOT NULL COMMENT '玩家唯一ID',
  `progress` VARCHAR(1024) NULL COMMENT '副本进度',
  `copy_count` VARCHAR(1024) NULL COMMENT '副本通关次数',
  `passed_copy` BLOB NOT NULL COMMENT '一次性通关副本',
  `chapter_award` BLOB NULL COMMENT '章节奖励',
  `gate_award` BLOB NULL COMMENT '关卡奖励',
  `buy_count` VARCHAR(1024) NULL DEFAULT NULL COMMENT '今日购买的次数',
  `tower_max_order` SMALLINT(6) NULL DEFAULT '0' COMMENT '爬塔最高层数',
  `tower_max_star` SMALLINT(6) NULL DEFAULT '0' COMMENT '爬塔最高星数',
  `tower_current_order` SMALLINT(6) NULL DEFAULT '0' COMMENT '爬塔当前层数',
  `tower_current_star` SMALLINT(6) NULL DEFAULT '0' COMMENT '爬塔当前星数',
  `tower_buff` VARCHAR(128) NULL DEFAULT NULL COMMENT '爬塔当前buff',
  `tower_buff_star` SMALLINT(6) NULL DEFAULT '0' COMMENT '可以购买buff的星星数',
  `tower_award` SMALLINT(6) NULL DEFAULT '0' COMMENT '已经领取奖励的层数',
  `tower_copy_star` INT(11) NULL DEFAULT '0' COMMENT '当前层副本星数',
  `tower_current_buff` VARCHAR(32) NULL DEFAULT NULL COMMENT '爬塔当前随机的buff',
  `tower_max_star_order` INT(11) NULL DEFAULT '0' COMMENT '最高满星爬塔层数',
  `achievement` BLOB NULL COMMENT '成就',
  `carrier_copy` VARCHAR(4096) NULL COMMENT '航母副本',
  `carrier_copy_info` VARCHAR(1024) NULL COMMENT '航母副本通关情况',
  `medal_copy_id` INT(11) NULL DEFAULT '0' COMMENT '勋章副本ID',
  `medal_star`  INT(11) NULL DEFAULT '0' COMMENT '勋章集册星数',
  `medal_state` VARCHAR(4096)  NULL COMMENT '勋章集册状态',
  `medal_achi` INT(11) NULL DEFAULT '0' COMMENT '勋章集册成就',

  PRIMARY KEY(`player_id`)
) ENGINE = InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `shop_0` (
  `player_id` BIGINT NOT NULL COMMENT '玩家唯一ID',
  `feats_last_time` BIGINT NOT NULL DEFAULT '0' COMMENT '免费抽取最后时间戳',
  `feats_used_count` INT NOT NULL DEFAULT '0' COMMENT '每日已刷新次数',
  `feats_shop_info` VARCHAR(256) NULL COMMENT '兑换商店信息',
  `normal_shop_info` VARCHAR(2048) NULL COMMENT '普通物商品信息',
  `life_shop_info` VARCHAR(2048) NULL COMMENT '终生物品商品信息',

  PRIMARY KEY(`player_id`)
) ENGINE = InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `new_shop_0` (
   `player_id` BIGINT NOT NULL COMMENT '玩家唯一ID',
   `shop_id` INT NOT NULL COMMENT '商店ID',
   `last_time` BIGINT NOT NULL DEFAULT '0' COMMENT '免费抽取最后时间戳',
   `used_count` INT NOT NULL DEFAULT '0' COMMENT '每日已刷新次数',
   `refresh_shop_info` VARCHAR(256) NULL COMMENT '刷新商店信息',

   PRIMARY KEY(`player_id`,`shop_id`)
) ENGINE = InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `pk_rank_list` (
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `server` INT(11) UNSIGNED NULL DEFAULT NULL COMMENT '服务器ID',
  `rank` INT(11) NULL DEFAULT NULL COMMENT '排行',

  PRIMARY KEY (`player_id`),
  INDEX `server` (`server`)
) ENGINE = InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `reward_0` (
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `pk_targets` VARCHAR(128) NULL DEFAULT NULL COMMENT '随机的玩家',
  `pk_rank_times` TINYINT(4) NULL DEFAULT '0' COMMENT '今天已经挑战次数',
  `last_pk_time` INT(11) NULL DEFAULT '0' COMMENT '上次竞技时间戳',
  `pk_rank_reward_rank` INT(11) NULL DEFAULT '0' COMMENT '上次排名',
  `pk_rank_reward_time` TINYINT(4) NULL DEFAULT '0' COMMENT '上次领取排名奖励时间',
  `pk_max_rank` INT(11) NULL DEFAULT '0' COMMENT '历史最高排名',
  `patrol_total_time` INT(11) NULL DEFAULT '0' COMMENT '巡逻总时间',
  `patrol_infos` VARCHAR(8192) NULL COMMENT '巡逻信息',
  `daily_sign` VARCHAR(128) NULL COMMENT '日常刷新信息',
  `month_card` BIGINT NOT NULL DEFAULT '0' COMMENT '月卡期限',
  `big_month_card` BIGINT NOT NULL DEFAULT '0' COMMENT '大月卡期限',
  `life_card` BIGINT NOT NULL DEFAULT '0' COMMENT '终生卡购买时间',
  `vip_weekly` VARCHAR(1024) NULL DEFAULT NULL COMMENT 'vip每周商店',
  `weekly_card` INT NULL DEFAULT NULL COMMENT '周基金购买时间',
  `weekly_card_login` TINYINT NULL DEFAULT '0' COMMENT '周基金登录天数',
  `weekly_card_status` TINYINT NULL DEFAULT '0' COMMENT '周基金领取状态',
  `month_card_1` INT NULL DEFAULT '0' COMMENT '小月基金购买时间',
  `month_card_1_login` SMALLINT NULL DEFAULT '0' COMMENT '小月基金登录天数',
  `month_card_1_status` INT NULL DEFAULT '0' COMMENT '小月基金领取状态',
  `month_card_2` INT NULL DEFAULT '0' COMMENT '大月基金购买时间',
  `month_card_2_login` SMALLINT NULL DEFAULT '0' COMMENT '大月基金登录天数',
  `month_card_2_status` INT NULL DEFAULT '0' COMMENT '大月基金领取状态',

  PRIMARY KEY (`player_id`)
) ENGINE = InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `friend_0` (
  `player_id` BIGINT NOT NULL COMMENT '玩家ID',
  `friend_id` BIGINT NOT NULL COMMENT '好友ID',
  `type` TINYINT NULL COMMENT '好友类型',
  `name` VARCHAR(64) NULL COMMENT '好友名字',
  `level` INT NULL DEFAULT '0' COMMENT '好友等级',
  `vip_level` INT NULL DEFAULT '0' COMMENT '好友VIP等级',
  `avatar` INT NULL DEFAULT '0' COMMENT '好友头像',
  `energy` INT NULL DEFAULT '0' COMMENT '好友赠送的精力',
  `energy_time` INT NULL DEFAULT '0' COMMENT '赠送还有精力时间',
  `score` BIGINT NULL DEFAULT '0' COMMENT '好友战力',
  `last_active_time` INT NULL DEFAULT '0' COMMENT '最后登出时间',
  `patrol_id` INT NULL DEFAULT '0' COMMENT '好友巡逻最高副本id',
  `army_name` VARCHAR(64) NULL DEFAULT NULL COMMENT '好友公会名',
  `rank_id` INT NULL DEFAULT '0' COMMENT '好友军衔id',

  PRIMARY KEY (`player_id`, `friend_id`)
) ENGINE = InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `report_0` (
  `player_id` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '玩家ID',
  `uid` BIGINT(11) NOT NULL,
  `report_type` TINYINT(4) NULL DEFAULT '0' COMMENT '战报摘要类型',
  `report_content` VARCHAR(256) NULL DEFAULT '0' COMMENT '战报摘要内容',
  PRIMARY KEY (`player_id`, `uid`)
) ENGINE = InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `copy_star_0` (
  `player_id` BIGINT NOT NULL COMMENT '玩家ID',
  `copy_id` INT NOT NULL COMMENT '副本ID',
  `star` TINYINT NULL COMMENT '副本星数',
  PRIMARY KEY (`player_id`, `copy_id`)
) COLLATE='utf8_general_ci' ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `carrier_0` (
  `player_id` BIGINT NOT NULL COMMENT '玩家ID',
  `carrier_id` INT(11) NOT NULL DEFAULT '0' COMMENT '航母ID',
  `level` INT(6) NOT NULL DEFAULT '0' COMMENT '等级',
  `exp` INT(11) NOT NULL DEFAULT '0' COMMENT '经验经验',
  `reform_level` INT(6) NOT NULL DEFAULT '0' COMMENT '改造等级',
   PRIMARY KEY(`player_id`, `carrier_id`)
) COLLATE = 'utf8_general_ci' ENGINE = InnoDB;

CREATE TABLE IF NOT EXISTS `dstrike_boss` (
  `server_id` INT(10) UNSIGNED NOT NULL COMMENT '服务器编号',
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `name` VARCHAR(64) NOT NULL COMMENT '玩家名称',
  `boss_id` INT(10) UNSIGNED NOT NULL COMMENT 'BOSS ID',
  `quality` INT(11) NOT NULL COMMENT 'BOSS品质',
  `level` INT(11) NOT NULL COMMENT 'BOSS等级',
  `blood` VARCHAR(256) NOT NULL COMMENT 'BOSS剩余血量',
  `status` INT(11) NOT NULL COMMENT 'BOSS状态',
  `time` INT(11) NOT NULL COMMENT 'BOSS发现时间',
  `expire_time` INT(11) NOT NULL COMMENT 'BOSS过期时间',
  `total_blood` BIGINT(20) NULL DEFAULT '0' COMMENT 'boss总的血量',

  PRIMARY KEY(`server_id`, `player_id`)
) ENGINE = InnoDB;

CREATE TABLE IF NOT EXISTS `server_shop` (
  `server_id` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `shop_data` VARCHAR(8192) NOT NULL COMMENT '商品信息',
  `astrology_country_id` INT(10) NOT NULL DEFAULT '0' COMMENT '占星奖池国家ID',
  `astrology_refresh_time` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '占星奖池刷新时间',

  PRIMARY KEY(`server_id`)
) ENGINE = InnoDB;

CREATE TABLE IF NOT EXISTS `server_mail` (
  `server_id` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `server_mail_id` BIGINT(20) NOT NULL COMMENT '邮件唯一ID',
  `mail_time` INT(10) NOT NULL COMMENT '邮件发送时间',
  `mail_content` VARCHAR(256) NOT NULL COMMENT '邮件内容',
  `mail_reward` VARCHAR(256) NOT NULL COMMENT '邮件附件',

  PRIMARY KEY(`server_id`, `server_mail_id`)
) ENGINE = InnoDB;

CREATE TABLE IF NOT EXISTS `ip_list` (
  `server_id` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `white_list` BLOB NOT NULL COMMENT '白名单',
  `black_list` BLOB NOT NULL COMMENT '黑名单',

  PRIMARY KEY (`server_id`)
) ENGINE=InnoDB ;

CREATE TABLE IF NOT EXISTS `server_notice` (
  `notice_id` BIGINT(20) NOT NULL COMMENT '公告唯一ID',
  `server_id` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `notice_type` INT(10) NOT NULL COMMENT '公告类型',
  `start_time`  BIGINT(20) NOT NULL COMMENT '公告开始时间',
  `end_time`  BIGINT(20) NOT NULL COMMENT '公告结束时间',
  `interval_time` INT(10) NOT NULL COMMENT  '公告间隔时间',
  `content` VARCHAR(256) NOT NULL COMMENT '公告内容',
  `order` INT(10) NOT NULL COMMENT  '公告排序',
  `link_url` VARCHAR(256) NOT NULL COMMENT '公告连接',

  PRIMARY KEY (`notice_id`,`server_id`),
  INDEX `serverid` (`server_id`)
) ENGINE=InnoDB ;

CREATE TABLE IF NOT EXISTS `rank_player_details` (
  `rank_type` TINYINT(4) NOT NULL COMMENT '排行榜类型',
  `player_id` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '玩家ID',
  `update_time` INT(11) NOT NULL DEFAULT '0' COMMENT '最后更新时间(用来清理垃圾数据)',
  `name` VARCHAR(64) NULL DEFAULT NULL COMMENT '玩家名字',
  `army_name` VARCHAR(64) NULL DEFAULT NULL COMMENT '军团名字',
  `level` INT(11) NULL DEFAULT NULL COMMENT '玩家等级',
  `fight_attr` BIGINT(20) NULL DEFAULT NULL COMMENT '玩家战斗力',
  `vip_level` INT(11) NULL DEFAULT NULL COMMENT 'VIP等级',
  `exploit` INT(11) NULL DEFAULT NULL COMMENT '围剿BOSS功勋',
  `damage` BIGINT(20) NULL DEFAULT NULL COMMENT '围剿BOSS伤害',
  `star` INT(11) NULL DEFAULT NULL COMMENT '副本星数',
  `avatar` TINYINT(4) NULL DEFAULT NULL COMMENT '头像',

  PRIMARY KEY (`rank_type`, `player_id`)
) COLLATE='utf8_general_ci' ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `rank_list_details` (
  `server_id` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `rank_type` TINYINT(4) NOT NULL COMMENT '排行榜类型',
  `players` BLOB NULL COMMENT '玩家ID',

  PRIMARY KEY (`server_id`, `rank_type`)
) COLLATE='utf8_general_ci' ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `recharge` (
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `time` INT(11) NOT NULL COMMENT '充值的时间',
  `money` INT(11) NOT NULL COMMENT '充值钱数目',
  `goodid` INT(11) NOT NULL COMMENT '商品ID',

  INDEX `player_id` (`player_id`)
) COLLATE='utf8_general_ci' ENGINE=InnoDB;

CREATE TABLE `recharge_details` (
  `order_id` VARCHAR(64) NULL DEFAULT NULL COMMENT '订单ID',
  `device_type` INT(11) NULL DEFAULT NULL COMMENT '设备类型(1:Android,2:iOS)',
  `user_id` VARCHAR(64) NULL DEFAULT NULL COMMENT '玩家的平台ID',
  `pay_amount` INT(11) NULL DEFAULT NULL COMMENT '支付金额(单位分)',
  `currency_code` VARCHAR(16) NULL DEFAULT NULL COMMENT '货币类型',
  `channel_id` varchar(64) NULL DEFAULT NULL COMMENT '1 Googlepay, 2 iso, 3其他渠道',
  `server_id` INT(10) UNSIGNED NULL DEFAULT NULL COMMENT '服ID',
  `role_id` BIGINT(20) NULL DEFAULT NULL COMMENT '玩家ID',
  `goods_type` INT(10) NULL DEFAULT NULL COMMENT '1为金币, 2为内购',
  `game_coin` INT(10) NULL DEFAULT NULL COMMENT '需要发放的金币数',
  `goods_id` varchar(32) NULL DEFAULT NULL COMMENT '商品ID',
  `timestamp` VARCHAR(32) NULL DEFAULT NULL COMMENT '时间戳',
  `stage` INT(11) NULL DEFAULT NULL COMMENT '内部用(1表示CenterServer,3表示RecordServer)',
  `recharge_time` INT(11) NULL DEFAULT NULL COMMENT '我们自己实际收到的',

  UNIQUE INDEX `order_id` (`order_id`, `stage`),
  INDEX `role_id` (`role_id`),
  INDEX `server_id` (`server_id`),
  INDEX `recharge_time` (`recharge_time`)
) COLLATE='utf8_general_ci'  ENGINE=InnoDB;

CREATE TABLE `recharge_openid` (
  `server` INT(10) UNSIGNED NULL DEFAULT NULL COMMENT '服务器ID',
  `openid` VARCHAR(64) NOT NULL COMMENT '玩家账号',
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `recharge_time` INT(11) NULL DEFAULT NULL COMMENT '充值时间',
  `money` INT(11) NULL DEFAULT NULL COMMENT '充值金额',

  INDEX `recharge_time` (`recharge_time`),
  PRIMARY KEY (`openid`)
) COLLATE='utf8_general_ci' ENGINE=InnoDB;

CREATE TABLE `recharge_uid` (
  `server` INT(10) UNSIGNED NULL DEFAULT NULL COMMENT '服务器时间',
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `recharge_time` INT(11) NULL DEFAULT NULL COMMENT '充值时间',
  `money` INT(11) NULL DEFAULT NULL COMMENT '充值金额',

  INDEX `recharge_time` (`recharge_time`),
  PRIMARY KEY (`player_id`)
) COLLATE='utf8_general_ci' ENGINE=InnoDB;

CREATE TABLE `activity_record_0` (
 `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
 `activity_type` INT(10) NOT NULL DEFAULT 0 COMMENT '活动类型',
 `refresh_time` BIGINT(20) NOT NULL DEFAULT 0 COMMENT '刷新时间',
 `bought_record` VARCHAR(256) NULL DEFAULT NULL COMMENT '购买记录',

  PRIMARY KEY(`player_id`,`activity_type`)
) COLLATE = 'utf8_general_ci' ENGINE = InnoDB;

CREATE TABLE `activity` (
 `server_id` int(10) unsigned NOT NULL COMMENT '服务器ID',
 `activity_type` int(10) NOT NULL DEFAULT '0' COMMENT '活动类型',
 `begin_time` bigint(20) NOT NULL DEFAULT '0' COMMENT '开始时间',
 `end_time` bigint(20) NOT NULL DEFAULT '0' COMMENT '结束时间',
 `field` varchar(512) DEFAULT NULL COMMENT '字段',
 `content` varchar(8096) DEFAULT NULL COMMENT '内容',
 `description` varchar(128) DEFAULT NULL COMMENT '活动描述',
  PRIMARY KEY (`server_id`,`activity_type`)

) COLLATE = 'utf8_general_ci' ENGINE = InnoDB;

CREATE TABLE `activity_record_new_0` (
 `player_id` bigint(20) NOT NULL COMMENT '玩家ID',
 `activity_type` int(10) NOT NULL DEFAULT '0' COMMENT '活动类型',
 `activity_id` bigint(20) NOT NULL DEFAULT '0' COMMENT '活动ID',
 `refresh_time` bigint(20) NOT NULL DEFAULT '0' COMMENT '刷新时间',
 `bought_record` varchar(256) DEFAULT NULL COMMENT '购买记录',
 `award` varchar(8096) DEFAULT NULL COMMENT '奖励获取',
  PRIMARY KEY (`player_id`,`activity_type`,`activity_id`)
) COLLATE='utf8_general_ci' ENGINE=InnoDB;

CREATE TABLE `activity_new` (
 `server_id` int(10) unsigned NOT NULL COMMENT '服务器ID',
 `activity_type` int(10) NOT NULL DEFAULT '0' COMMENT '活动类型',
 `activity_id` bigint(20) NOT NULL DEFAULT '0' COMMENT '活动ID',
 `begin_time` bigint(20) NOT NULL DEFAULT '0' COMMENT '开始时间',
 `end_time` bigint(20) NOT NULL DEFAULT '0' COMMENT '结束时间',
 `field` varchar(512) DEFAULT NULL COMMENT '字段',
 `content` varchar(8096) DEFAULT NULL COMMENT '内容',
 `description` varchar(128) DEFAULT NULL COMMENT '活动描述',
 PRIMARY KEY (`server_id`,`activity_type`,`activity_id`)
) COLLATE='utf8_general_ci' ENGINE=InnoDB;

CREATE TABLE `activity_mould` (
 `activity_type` INT(11) NOT NULL COMMENT '活动类型',
 `description` VARCHAR(128) NOT NULL COMMENT '备注',
 `field` VARCHAR(512) NULL DEFAULT NULL COMMENT '列描述',
 `content` VARCHAR(8096) NULL DEFAULT NULL COMMENT '内容',
 PRIMARY KEY (`activity_type`, `description`)
)
COLLATE='utf8_general_ci'
ENGINE=InnoDB
;
