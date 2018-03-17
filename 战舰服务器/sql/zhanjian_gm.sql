-- 导出  表 zhanjian_gm.gm_day_address 结构
CREATE TABLE IF NOT EXISTS `gm_day_address` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `date_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '日期',
  `address_type` char(24) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL COMMENT '地域',
  `num` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '使用数量',
  PRIMARY KEY (`id`),
  KEY `key2` (`platform_id`,`server_id`,`date_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='手游订制数据之地域分布';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_add_item 结构
CREATE TABLE IF NOT EXISTS `gm_day_add_item` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `date_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '日期',
  `player_id` char(24) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL COMMENT '角色ID',
  `item_id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '物品id',
  `action_type` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '功能点id',
  `module_id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '所属模块',
  `change_num` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '物品增加数量',
  PRIMARY KEY (`id`),
  KEY `key1` (`platform_id`,`server_id`,`item_id`,`player_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='记录每种物品每天每个功能点的产出量';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_add_resource 结构
CREATE TABLE IF NOT EXISTS `gm_day_add_resource` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `date_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '日期',
  `player_id` char(24) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL COMMENT '角色ID',
  `resource_type` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '资源类型',
  `action_type` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '功能点id',
  `module_id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '所属模块',
  `change_num` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '资源增加数量',
  PRIMARY KEY (`id`),
  KEY `key1` (`platform_id`,`server_id`,`resource_type`,`player_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='记录每种资源每天每个功能点的产出量';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_channel 结构
CREATE TABLE IF NOT EXISTS `gm_day_channel` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `date_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '日期',
  `channel_type` char(24) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL COMMENT '地域',
  `num` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '使用数量',
  PRIMARY KEY (`id`),
  KEY `key2` (`platform_id`,`server_id`,`date_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='手游订制数据之渠道分布';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_charge 结构
CREATE TABLE IF NOT EXISTS `gm_day_charge` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `date_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '日期',
  `new_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '注册账号数',
  `day_charge_value_1` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '玩家在注册起前n天的充值金额',
  `day_charge_player_1` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '玩家在注册起前n天的充值人数',
  `day_charge_arppu_1` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '注册前n天ARPPU(注册前n天充值金额 / 注册前n天充值人数)',
  `day_charge_rate_1` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '注册前n天付费率(注册前n天充值人数 / 注册帐号数)',
  `day_charge_ltv_1` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT 'n日LTV(注册前n天充值金额 / 注册帐号数)',
  `day_charge_value_2` int(10) unsigned NOT NULL DEFAULT '0',
  `day_charge_player_2` int(10) unsigned NOT NULL DEFAULT '0',
  `day_charge_arppu_2` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_rate_2` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_ltv_2` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_value_3` int(10) unsigned NOT NULL DEFAULT '0',
  `day_charge_player_3` int(10) unsigned NOT NULL DEFAULT '0',
  `day_charge_arppu_3` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_rate_3` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_ltv_3` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_value_4` int(10) unsigned NOT NULL DEFAULT '0',
  `day_charge_player_4` int(10) unsigned NOT NULL DEFAULT '0',
  `day_charge_arppu_4` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_rate_4` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_ltv_4` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_value_5` int(10) unsigned NOT NULL DEFAULT '0',
  `day_charge_player_5` int(10) unsigned NOT NULL DEFAULT '0',
  `day_charge_arppu_5` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_rate_5` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_ltv_5` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_value_6` int(10) unsigned NOT NULL DEFAULT '0',
  `day_charge_player_6` int(10) unsigned NOT NULL DEFAULT '0',
  `day_charge_arppu_6` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_rate_6` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_ltv_6` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_value_7` int(10) unsigned NOT NULL DEFAULT '0',
  `day_charge_player_7` int(10) unsigned NOT NULL DEFAULT '0',
  `day_charge_arppu_7` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_rate_7` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_ltv_7` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_ltv_15` double(10,4) NOT NULL DEFAULT '0.0000',
  `day_charge_ltv_30` double(10,4) NOT NULL DEFAULT '0.0000',
  PRIMARY KEY (`id`),
  KEY `key1` (`platform_id`,`server_id`,`date_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='新进付费';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_day_info 结构
CREATE TABLE IF NOT EXISTS `gm_day_day_info` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `date_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '日期',
  `month_expect_pay` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '月充值预算值',
  `month_has_pay` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '月累计付费',
  `expect_rate` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '当日充值金额',
  `pay_money` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '当日充值金额',
  `login_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '当日登陆账号数',
  `pay_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '当日充值账号数',
  `arpu` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '总充值金额 / 登陆账号数',
  `arppu` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '总充值金额 / 总充值人数',
  `pay_rate` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '付费率(总充值人数 / 登陆账号数)',
  `old_login_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '老账号登录数',
  `new_account_pay` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '新账号充值金额',
  `new_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '注册帐号数',
  `new_login_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '新登录账号数',
  `activation_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '激活数(当日激活用户数量（多数以角色取名为埋点）)',
  `activation_rate` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '激活率',
  `new_pay_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '新增充值帐号数(当日充值且为首次充值的账号数)',
  `two_day_rate` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '充值次数',
  `three_day_rate` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '当日总充值金额',
  `seven_day_rate` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '付费率(充值帐号数 / 登录帐号数)',
  `dt_online` int(10) NOT NULL DEFAULT '0' COMMENT '平均在线',
  `top_online` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '最高在线',
  `login_pay_num` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '日充值保有人数(当日登录游戏的付费用户数)',
  `total_pay` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '保有付费玩家人数(周活跃付费玩家人数)',
  PRIMARY KEY (`id`),
  KEY `key1` (`platform_id`,`server_id`,`date_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='每个服务器日报数据';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_network 结构
CREATE TABLE IF NOT EXISTS `gm_day_network` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `date_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '日期',
  `network_type` char(24) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL COMMENT '网络类型',
  `num` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '使用数量',
  PRIMARY KEY (`id`),
  KEY `key2` (`platform_id`,`server_id`,`date_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='手游订制数据之网络分布';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_pay_info 结构
CREATE TABLE IF NOT EXISTS `gm_day_pay_info` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `date_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '日期',
  `new_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '注册账号数',
  `total_login_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '总登录用户数',
  `top_online_num` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '最高在线人数',
  `total_pay_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '总付费人数',
  `total_pay_times` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '总充值次数',
  `total_pay_money` int(10) NOT NULL DEFAULT '0' COMMENT '总充值金额',
  PRIMARY KEY (`id`),
  KEY `key1` (`platform_id`,`server_id`,`date_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='付费总览';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_phone 结构
CREATE TABLE IF NOT EXISTS `gm_day_phone` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `date_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '日期',
  `phone_type` char(24) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL COMMENT '手机',
  `num` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '使用数量',
  PRIMARY KEY (`id`),
  KEY `key2` (`platform_id`,`server_id`,`date_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='手游订制数据之机型分布';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_player_login 结构
CREATE TABLE IF NOT EXISTS `gm_day_player_login` (
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `player_id` bigint(20) NOT NULL COMMENT '角色ID',
  `date_time` date NOT NULL COMMENT '日期',
  `login_time` datetime NOT NULL COMMENT '登陆时间',
  PRIMARY KEY (`player_id`,`date_time`),
  KEY `server_id` (`server_id`),
  KEY `login_time` (`login_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='记录玩家登陆行为';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_retention_rates 结构
CREATE TABLE IF NOT EXISTS `gm_day_retention_rates` (
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) NOT NULL COMMENT '服务器id',
  `date_time` date NOT NULL COMMENT '日期',
  `new_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '注册账号数',
  `day_stay_2` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '注册后第n天留存人数',
  `day_stay_rate_2` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT 'n留率(n留人数 / 注册帐号数)',
  `day_stay_3` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '注册后第n天留存人数',
  `day_stay_rate_3` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT 'n留率',
  `day_stay_4` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '注册后第n天留存人数',
  `day_stay_rate_4` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT 'n留率',
  `day_stay_5` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '注册后第n天留存人数',
  `day_stay_rate_5` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT 'n留率',
  `day_stay_6` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '注册后第n天留存人数',
  `day_stay_rate_6` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT 'n留率',
  `day_stay_7` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '注册后第n天留存人数',
  `day_stay_rate_7` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT 'n留率',
  `day_3_2` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '3+/2+(用户注册日起7天以内登录天数>=3天的用户数 / 登录天数>=2天的用户数)',
  `day_4_3` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '4+/3+(用户注册日起7天以内登录天数>=4天的用户数 / 登录天数>=3天的用户数)',
  `account_source` smallint(6) unsigned NOT NULL DEFAULT '0' COMMENT '广告用户总数(1，广告用户，2，非广告用户，3所有用户)',
  PRIMARY KEY (`server_id`,`date_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='新进留存率';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_server_info 结构
CREATE TABLE IF NOT EXISTS `gm_day_server_info` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `date_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '日期',
  `new_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '注册账号数',
  `equally_hour_online` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '平均每小时在线人数',
  `top_hour_online` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '最高每小时在线人数',
  `player_dt_online` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'DT时间(玩家平均在线时间（分钟）)',
  `connect_ip` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '连接IP数(登录游戏的IP个数)',
  `login_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '登录帐号数',
  `old_login_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '非当日注册登录游戏的账号数',
  `activation_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '激活数(当日激活用户数量（多数以角色取名为埋点）)',
  `charge_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '充值帐号数(当日充值过的帐号数)',
  `total_charge_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '累计充值帐号数(截止到当日的去重累计充值帐号数)',
  `new_charge_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '新增充值帐号数(当日充值且为首次充值的账号数)',
  `charge_times` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '充值次数',
  `charge_value` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '当日总充值金额',
  `pay_rate` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '付费率(充值帐号数 / 登录帐号数)',
  `arppu` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT 'ARPPU(充值金额 / 充值帐号数)',
  `charge_player_rest_gold` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '保有付费玩家剩余黄金量(周活跃付费玩家总剩余黄金量)',
  `charge_player_consume_gold` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '保有付费玩家当日消耗黄金量(周活跃付费玩家当日总消耗黄金量)',
  `charge_player_num` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '保有付费玩家人数(周活跃付费玩家人数)',
  `equally_charge_player_rest_gold` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '保有付费玩家人均剩余黄金量(保有付费玩家剩余黄金量 / 保有付费玩家人数)',
  `equally_charge_player_consume_gold` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '保有付费玩家人均消耗黄金量(保有付费玩家当日消耗黄金量 / 保有付费玩家人数)',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='每个服务器运营概况表(运营总览)';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_sex 结构
CREATE TABLE IF NOT EXISTS `gm_day_sex` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `date_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '日期',
  `sex_type` char(24) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL COMMENT '性别',
  `num` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '使用数量',
  PRIMARY KEY (`id`),
  KEY `key2` (`platform_id`,`server_id`,`date_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='手游订制数据之性别分布';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_sub_item 结构
CREATE TABLE IF NOT EXISTS `gm_day_sub_item` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `date_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '日期',
  `player_id` char(24) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL COMMENT '角色ID',
  `item_id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '物品id',
  `action_type` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '功能点id',
  `module_id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '所属模块',
  `change_num` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '物品增加数量',
  PRIMARY KEY (`id`),
  KEY `key1` (`platform_id`,`server_id`,`item_id`,`player_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='记录每种物品每天每个功能点的消耗量';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_sub_resource 结构
CREATE TABLE IF NOT EXISTS `gm_day_sub_resource` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `date_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '日期',
  `player_id` char(24) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL COMMENT '角色ID',
  `resource_type` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '资源类型',
  `action_type` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '功能点id',
  `module_id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '所属模块',
  `change_num` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT '资源增加数量',
  PRIMARY KEY (`id`),
  KEY `key1` (`platform_id`,`server_id`,`resource_type`,`player_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='记录每种资源每天每个功能点的消耗量';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_day_take_rates 结构
CREATE TABLE IF NOT EXISTS `gm_day_take_rates` (
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) NOT NULL COMMENT '服务器id',
  `date_time` date NOT NULL COMMENT '日期',
  `new_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '注册账号数',
  `new_login_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '当日注册且记录登录行为的帐号数量',
  `connectivity_rate` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '连通率,新登录账号数 / 注册账号数',
  `valid_account` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '根据游戏初期引导节奏定义是否有效（如角色取名、指定等级、完成引导等）',
  `valid_rate` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '有效转化率,有效用户数 / 注册账号数',
  `minute_valid_10` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '10分钟转化人数,最后登出时间-注册时间>=n分钟人数',
  `minute_valid_30` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '30分钟转化人数',
  `minute_valid_60` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '60分钟转化人数',
  `minute_valid_rate_10` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '10分钟转化率,n分钟转化人数 / 注册账号数',
  `minute_valid_rate_30` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '30分钟转化率',
  `minute_valid_rate_60` double(10,4) NOT NULL DEFAULT '0.0000' COMMENT '60分钟转化率',
  `account_source` smallint(6) unsigned NOT NULL DEFAULT '0' COMMENT '广告用户总数(1，广告用户，2，非广告用户，3所有用户)',
  PRIMARY KEY (`server_id`,`date_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='新进转化率';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_online_info 结构
CREATE TABLE IF NOT EXISTS `gm_online_info` (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT NULL COMMENT '服务器id',
  `cur_time` bigint(20) NOT NULL DEFAULT '0' COMMENT '当前时间（每隔五分钟的时间戳）',
  `online_num` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '在线人数',
  KEY `cur_time` (`cur_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='每隔五分钟记录玩家在线人数表';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.gm_player_info 结构
CREATE TABLE IF NOT EXISTS `gm_player_info` (
  `platform_id` varchar(32) DEFAULT NULL COMMENT '平台id',
  `op_id` varchar(32) DEFAULT '0' COMMENT '平台运营商id',
  `server_id` varchar(32) DEFAULT '0' COMMENT '服务器ID',
  `player_id` bigint(20) NOT NULL COMMENT '角色ID',
  `create_time` date NOT NULL DEFAULT '0000-00-00' COMMENT '创角日期',
  `create_time_stamp` bigint(20) NOT NULL DEFAULT '0' COMMENT '创角时间戳',
  `curr_level` smallint(8) unsigned NOT NULL DEFAULT '1' COMMENT '等级',
  `day_2_login` smallint(8) unsigned NOT NULL DEFAULT '0' COMMENT '第二天是否登陆',
  `day_3_login` smallint(8) unsigned NOT NULL DEFAULT '0' COMMENT '第三天是否登陆',
  `day_4_login` smallint(8) unsigned NOT NULL DEFAULT '0' COMMENT '第四天是否登陆',
  `day_5_login` smallint(8) unsigned NOT NULL DEFAULT '0' COMMENT '第五天是否登陆',
  `day_6_login` smallint(8) unsigned NOT NULL DEFAULT '0' COMMENT '第六天是否登陆',
  `day_7_login` smallint(8) unsigned NOT NULL DEFAULT '0' COMMENT '第七天是否登陆',
  `guide_id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '指引步骤',
  `guide_branch_id` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '指引分步的id',
  `vip_level` smallint(8) unsigned NOT NULL DEFAULT '0' COMMENT 'vip等级',
  PRIMARY KEY (`player_id`),
  KEY `server_id` (`server_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='记录玩家信息的表';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.http_action 结构
CREATE TABLE IF NOT EXISTS `http_action` (
  `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT COMMENT '用户id',
  `account` varchar(32) DEFAULT NULL COMMENT '用户账号',
  `http_time` int(10) unsigned NOT NULL DEFAULT '0' COMMENT '活动开始时间',
  `action` varchar(32) DEFAULT NULL COMMENT '用户操作',
  `data` text,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='平台操作记录';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.server 结构
CREATE TABLE IF NOT EXISTS `server` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `sid` varchar(32) NOT NULL,
  `opgame_id` varchar(32) NOT NULL,
  `opgame_name` varchar(64) NOT NULL COMMENT '平台运营商名称',
  `timezone` varchar(50) NOT NULL COMMENT '时区',
  `language` int(10) unsigned NOT NULL COMMENT '语种',
  `op_sid` varchar(50) DEFAULT NULL,
  `our_key` varchar(50) DEFAULT NULL,
  `type` smallint(6) unsigned NOT NULL,
  `server_id` varchar(50) NOT NULL COMMENT '服务器id',
  `server_name` varchar(50) NOT NULL COMMENT '大区名字',
  `number` int(10) unsigned NOT NULL,
  `server_url` text NOT NULL COMMENT 'server地址',
  `recharge_url` text NOT NULL COMMENT '充值地址',
  `first_opentime` bigint(20) unsigned NOT NULL COMMENT '开服时间',
  `notice_opentime` bigint(20) unsigned NOT NULL,
  `recharge_opentime` bigint(20) unsigned NOT NULL,
  `merge_open_time` bigint(20) unsigned NOT NULL,
  `merge_end_time` bigint(20) unsigned NOT NULL,
  `ip_white` text NOT NULL COMMENT '白名单',
  `ip_black` text NOT NULL COMMENT '黑名单',
  `max_online` int(10) unsigned NOT NULL,
  `fcm_time` int(10) unsigned NOT NULL COMMENT '防沉迷状态',
  `is_active` smallint(6) unsigned NOT NULL,
  `is_recommend` smallint(6) unsigned NOT NULL,
  `is_hide` smallint(6) unsigned NOT NULL,
  `custom_suffix` text NOT NULL,
  `force_style` varchar(50) DEFAULT NULL,
  `is_first` smallint(6) unsigned NOT NULL,
  `protocol` smallint(6) unsigned NOT NULL,
  `server_status` smallint(6) unsigned NOT NULL COMMENT '服务器运行状态，0开启，1维护，2关闭',
  `is_microend` smallint(6) unsigned NOT NULL COMMENT '是否开启微端',
  `merge_ok_time` bigint(20) unsigned NOT NULL,
  `parent_merge_sid` varchar(50) NOT NULL,
  `child_merge_sid` text NOT NULL,
  `merge_count` smallint(6) unsigned NOT NULL,
  `merge_status` smallint(6) unsigned NOT NULL,
  `remark` text NOT NULL,
  `is_maintain` smallint(6) unsigned NOT NULL,
  `maintain_content` text NOT NULL,
  `game_code` text NOT NULL,
  `db_game_name` text NOT NULL,
  `db_log_name` text NOT NULL,
  `server_key` text NOT NULL COMMENT '服务器key',
  `op_id` text NOT NULL COMMENT '运营商id',
  `game_id` varchar(32) NOT NULL COMMENT '游戏id',
  `socket_url` text NOT NULL,
  `socket_port` int(11) NOT NULL,
  `chat_url` text NOT NULL COMMENT '聊天地址',
  `chat_port` int(11) NOT NULL COMMENT '聊天端口',
  `log_ip` varchar(32) NOT NULL,
  `group_url` text NOT NULL,
  `server_ip` varchar(32) NOT NULL COMMENT '服务器ip',
  `recharge_agent` text NOT NULL,
  `server_gip` varchar(32) NOT NULL,
  `server_nip` varchar(32) NOT NULL,
  `server_domain` text NOT NULL COMMENT '服务区域名',
  `dbwip` char(15) DEFAULT NULL COMMENT '数据库ip',
  `dbwuser` varchar(50) DEFAULT NULL COMMENT '数据库用户',
  `dbwpassword` varchar(50) DEFAULT NULL COMMENT '数据库密码',
  `dbwport` int(11) DEFAULT NULL COMMENT '数据库端口',
  `link_status` smallint(6) unsigned NOT NULL COMMENT '服务器连接状态0连接，1关闭',
  `server_version` varchar(32) NOT NULL COMMENT '服务器版本',
  `http_get_status` smallint(6) unsigned NOT NULL DEFAULT '0' COMMENT '0没用通过http获取过，1已经获取过',
  PRIMARY KEY (`id`),
  UNIQUE KEY `id` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='连接cs的服务器列表';

-- 数据导出被取消选择。


-- 导出  表 zhanjian_gm.server_update_config_log 结构
CREATE TABLE IF NOT EXISTS `server_update_config_log` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT COMMENT '自增唯一id',
  `server_id` varchar(50) NOT NULL,
  `update_time` datetime NOT NULL COMMENT '更新加载时间时间',
  `status` smallint(6) DEFAULT '0' COMMENT '0失败，1成功',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='更新配置的时间日志';

-- 30天留存 --
CREATE TABLE IF NOT EXISTS `gm_day_retention_30` (
  `platform_id` VARCHAR(32) NULL DEFAULT NULL COMMENT '平台id',
  `op_id` VARCHAR(32) NULL DEFAULT '0' COMMENT '平台运营商id',
  `server_id` VARCHAR(32) NOT NULL COMMENT '服务器id',
  `date_time` DATE NOT NULL COMMENT '日期',
  `new_account` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册账号数',
  `day_stay_2` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第2天留存人数',
  `day_stay_3` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第3天留存人数',
  `day_stay_4` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第4天留存人数',
  `day_stay_5` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第5天留存人数',
  `day_stay_6` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第6天留存人数',
  `day_stay_7` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第7天留存人数',
  `day_stay_8` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第8天留存人数',
  `day_stay_9` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第9天留存人数',
  `day_stay_10` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第10天留存人数',
  `day_stay_11` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第11天留存人数',
  `day_stay_12` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第12天留存人数',
  `day_stay_13` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第13天留存人数',
  `day_stay_14` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第14天留存人数',
  `day_stay_15` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第15天留存人数',
  `day_stay_16` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第16天留存人数',
  `day_stay_17` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第17天留存人数',
  `day_stay_18` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第18天留存人数',
  `day_stay_19` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第19天留存人数',
  `day_stay_20` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第20天留存人数',
  `day_stay_21` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第21天留存人数',
  `day_stay_22` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第22天留存人数',
  `day_stay_23` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第23天留存人数',
  `day_stay_24` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第24天留存人数',
  `day_stay_25` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第25天留存人数',
  `day_stay_26` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第26天留存人数',
  `day_stay_27` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第27天留存人数',
  `day_stay_28` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第28天留存人数',
  `day_stay_29` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第29天留存人数',
  `day_stay_30` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '注册后第30天留存人数',
  PRIMARY KEY (`server_id`, `date_time`)
) COMMENT='30天留存人数' COLLATE='utf8_general_ci' ENGINE=InnoDB;

CREATE TABLE `opgame_list` (
	`opgame_id` INT(11) NOT NULL DEFAULT '0' COMMENT '平台编号',
	`opgame_name` VARCHAR(50) NOT NULL COMMENT '平台名称',
	PRIMARY KEY (`opgame_id`)
)
COMMENT='平台列表'
COLLATE='utf8_general_ci'
ENGINE=InnoDB
;
CREATE TABLE `op_list` (
	`op_id` INT(11) NOT NULL DEFAULT '0' COMMENT '运营商编号',
	`op_name` VARCHAR(50) NOT NULL COMMENT '运营商名称',
	PRIMARY KEY (`op_id`)
)
COMMENT='运营商列表'
COLLATE='utf8_general_ci'
ENGINE=InnoDB
;
