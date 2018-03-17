CREATE TABLE IF NOT EXISTS `currency_@date@` (
  `tid` BIGINT(20) NOT NULL COMMENT '事务ID',
  `time` INT(11) NULL DEFAULT '0' COMMENT '发生时间',
  `server` INT(10) UNSIGNED NULL DEFAULT '0' COMMENT '服务器ID',
  `player_id` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '玩家ID',
  `system` INT(10) NULL DEFAULT '0' COMMENT '系统模块',
  `msgid` INT(10) NULL DEFAULT '0' COMMENT '消息ID',
  `level` INT(10) NULL DEFAULT '0' COMMENT '玩家等级',
  `vip_level` INT(10) NULL DEFAULT '0' COMMENT '玩家VIP等级',
  `money_1` BIGINT(20) NULL DEFAULT '0' COMMENT '金币(改变后的值)',
  `delta_1` BIGINT(20) NULL DEFAULT '0' COMMENT '金币改变值',
  `money_2` BIGINT(20) NULL DEFAULT '0' COMMENT '钻石(改编后的值)',
  `delta_2` BIGINT(20) NULL DEFAULT '0' COMMENT '钻石改变值',
  `money_5` BIGINT(20) NULL DEFAULT '0' COMMENT '经验',
  `delta_5` BIGINT(20) NULL DEFAULT '0' COMMENT '经验改变值',
  `money_6` INT(10) NULL DEFAULT '0' COMMENT 'vip经验',
  `delta_6` INT(10) NULL DEFAULT '0' COMMENT 'vip经验改变值',
  `money_7` INT(10) NULL DEFAULT '0' COMMENT '功勋',
  `delta_7` INT(10) NULL DEFAULT '0' COMMENT '功勋改变值',
  `money_8` INT(10) NULL DEFAULT '0' COMMENT '功绩',
  `delta_8` INT(10) NULL DEFAULT '0' COMMENT '功绩改变值',
  `money_9` INT(10) NULL DEFAULT '0' COMMENT '荣誉',
  `delta_9` INT(10) NULL DEFAULT '0' COMMENT '荣誉改变值',
  `money_10` INT(10) NULL DEFAULT '0' COMMENT '名望',
  `delta_10` INT(10) NULL DEFAULT '0' COMMENT '名望改变值',
  `money_11` INT(10) NULL DEFAULT '0' COMMENT '贡献',
  `delta_11` INT(10) NULL DEFAULT '0' COMMENT '贡献改变值',
  `money_12` INT(10) NULL DEFAULT '0' COMMENT '军团战功',
  `delta_12` INT(10) NULL DEFAULT '0' COMMENT '军团战功改变值',
  `delta_vip` INT(10) NULL DEFAULT '0' COMMENT 'VIP等级改变值',
  `delta_level` INT(10) NULL DEFAULT '0' COMMENT '等级改变值',

  INDEX `tid` (`tid`),
  INDEX `player_id` (`player_id`)
) COLLATE='utf8_bin' ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `oil_@date@` (
  `tid` BIGINT(20) NOT NULL COMMENT '事务ID',
  `time` INT(11) NULL DEFAULT '0' COMMENT '发生时间',
  `server` INT(10) UNSIGNED NULL DEFAULT '0' COMMENT '服务器ID',
  `player_id` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '玩家ID',
  `system` INT(10) NULL DEFAULT '0' COMMENT '系统模块',
  `msgid` INT(10) NULL DEFAULT '0' COMMENT '消息ID',
  `oil` INT(10) NULL DEFAULT '0' COMMENT '燃油',
  `energy` INT(10) NULL DEFAULT '0' COMMENT '能源',
  `delta_oil` INT(10) NULL DEFAULT '0' COMMENT '燃油变化值',
  `delta_energy` INT(10) NULL DEFAULT '0' COMMENT '能源变化值',

  INDEX `tid` (`tid`),
  INDEX `player_id` (`player_id`)
) COLLATE='utf8_bin' ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `login_@date@` (
  `tid` BIGINT(20) NOT NULL COMMENT '事务ID',
  `time` INT(11) NULL DEFAULT '0' COMMENT '发生时间',
  `server` INT(10) UNSIGNED NULL DEFAULT '0' COMMENT '服务器ID',
  `player_id` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '玩家ID',
  `login` INT(10) NULL DEFAULT '0' COMMENT '1是登录,2是登出',
  `online_time` INT(10) NULL DEFAULT '0' COMMENT '在线时长',
  `ipaddr` VARCHAR(16) NULL DEFAULT '0' COMMENT 'IP地址' COLLATE 'utf8_bin',

  INDEX `tid` (`tid`),
  INDEX `player_id` (`player_id`)
) COLLATE='utf8_bin' ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `newplayer_@date@` (
  `server` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '服务器ID',
  `openid` VARCHAR(64) NULL DEFAULT '0' COMMENT '玩家账号' COLLATE 'utf8_bin',
  `player_id` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '玩家ID',
  `time` INT(11) NULL DEFAULT '0' COMMENT '发生时间',

  PRIMARY KEY (`server`, `player_id`),
  INDEX `openid` (`openid`)
) COLLATE='utf8_bin' ENGINE=InnoDB;

CREATE TABLE `hero_@date@` (
  `tid` BIGINT(20) NOT NULL COMMENT '事务ID',
  `time` INT(11) NOT NULL COMMENT '发生时间',
  `server` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `system` INT(10) NULL DEFAULT '0' COMMENT '系统模块',
  `msgid` INT(10) NULL DEFAULT '0' COMMENT '消息ID',
  `hero_uid` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '船只唯一ID',
  `hero_id` INT(11) NOT NULL DEFAULT '0' COMMENT '船只ID',
  `level` INT(11) NOT NULL DEFAULT '0' COMMENT '船只等级',
  `exp` INT(11) NOT NULL DEFAULT '0' COMMENT '船只经验',
  `grade` INT(11) NOT NULL DEFAULT '0' COMMENT '突破等级',
  `rand_attr` VARCHAR(128) NOT NULL DEFAULT '\'\'' COMMENT '随机属性' COLLATE 'utf8_bin',
  `fate_level` INT(11) NOT NULL DEFAULT '0' COMMENT '天命等级',
  `fate_exp` INT(11) NOT NULL DEFAULT '0' COMMENT '天命经验',
  `fate_seed` INT(11) NOT NULL DEFAULT '0' COMMENT '天命种子',
  `train_cost` INT(11) NOT NULL DEFAULT '0' COMMENT '训练消耗',
  `fate_cost` INT(11) NOT NULL DEFAULT '0' COMMENT '天命消耗',
  `old_level` INT(11) NOT NULL DEFAULT '0' COMMENT '原始等级',
  `old_exp` INT(11) NOT NULL DEFAULT '0' COMMENT '原始经验',
  `old_grade` INT(11) NOT NULL DEFAULT '0' COMMENT '原始突破等级',
  `old_rand_attr` VARCHAR(128) NOT NULL DEFAULT '\'\'' COMMENT '原始随机属性' COLLATE 'utf8_bin',
  `old_fate_level` INT(11) NOT NULL DEFAULT '0' COMMENT '原始天命等级',
  `old_fate_exp` INT(11) NOT NULL DEFAULT '0' COMMENT '原始天命经验',
  `old_fate_seed` INT(11) NOT NULL DEFAULT '0' COMMENT '原始天命种子',
  `old_train_cost` INT(11) NOT NULL DEFAULT '0' COMMENT '原始培养消耗',
  `old_fate_cost` INT(11) NOT NULL DEFAULT '0' COMMENT '原始天命消耗',
  `wake_level` INT(11) NOT NULL DEFAULT '0' COMMENT '觉醒等级',
  `wake_item` VARCHAR(64) NULL DEFAULT NULL COMMENT '觉醒道具',
  `old_wake_level` INT(11) NOT NULL DEFAULT '0' COMMENT '原始觉醒等级',
  `old_wake_item` VARCHAR(64) NULL DEFAULT NULL COMMENT '原始觉醒道具',

  INDEX `tid` (`tid`),
  INDEX `player_id` (`player_id`)
) COLLATE='utf8_bin' ENGINE=InnoDB;

CREATE TABLE `hero_delete_@date@` (
  `tid` BIGINT(20) NOT NULL COMMENT '事务ID',
  `time` INT(11) NOT NULL COMMENT '发生时间',
  `server` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `system` INT(10) NULL DEFAULT '0' COMMENT '系统模块',
  `msgid` INT(10) NULL DEFAULT '0' COMMENT '消息ID',
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `hero_uid` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '船只唯一ID',
  `hero_id` INT(11) NOT NULL DEFAULT '0' COMMENT '船只ID',
  `level` INT(11) NOT NULL DEFAULT '0' COMMENT '船只等级',
  `exp` INT(11) NOT NULL DEFAULT '0' COMMENT '船只经验',
  `grade` INT(11) NOT NULL DEFAULT '0' COMMENT '突破等级',
  `rand_attr` VARCHAR(128) NOT NULL DEFAULT '\'\'' COMMENT '随机属性' COLLATE 'utf8_bin',
  `fate_level` INT(11) NOT NULL DEFAULT '0' COMMENT '天命等级',
  `fate_exp` INT(11) NOT NULL DEFAULT '0' COMMENT '天命经验',
  `fate_seed` INT(11) NOT NULL DEFAULT '0' COMMENT '天命种子',
  `train_cost` INT(11) NOT NULL DEFAULT '0' COMMENT '训练消耗',
  `fate_cost` INT(11) NOT NULL DEFAULT '0' COMMENT '天命消耗',
  `wake_level` INT(11) NOT NULL DEFAULT '0' COMMENT '觉醒等级',
  `wake_item` VARCHAR(64) NULL DEFAULT NULL COMMENT '觉醒道具',

  INDEX `tid` (`tid`),
  INDEX `player_id` (`player_id`)
) COLLATE='utf8_bin' ENGINE=InnoDB;

CREATE TABLE `item_@date@` (
  `tid` BIGINT(20) NOT NULL COMMENT '事务ID',
  `time` INT(11) NOT NULL COMMENT '发生时间',
  `server` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `system` INT(10) UNSIGNED NOT NULL COMMENT '系统ID',
  `msgid` INT(10) UNSIGNED NOT NULL COMMENT '消息ID',
  `item_uid` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '道具唯一ID',
  `item_id` INT(11) NOT NULL DEFAULT '0' COMMENT '道具ID',
  `item_count` INT(11) NOT NULL DEFAULT '0' COMMENT '道具数量',
  `item_attr` VARCHAR(512) NOT NULL DEFAULT '\'\'' COMMENT '道具属性' COLLATE 'utf8_bin',
  `item_old_count` INT(11) NOT NULL DEFAULT '0' COMMENT '道具数量',
  `item_old_attr` VARCHAR(512) NOT NULL DEFAULT '\'\'' COMMENT '道具老的属性' COLLATE 'utf8_bin',

  INDEX `tid` (`tid`),
  INDEX `player_id` (`player_id`)
) COLLATE='utf8_bin' ENGINE=InnoDB;

CREATE TABLE `item_delete_@date@` (
  `tid` BIGINT(20) NOT NULL COMMENT '事务ID',
  `time` INT(11) NOT NULL COMMENT '发生时间',
  `server` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `system` INT(10) UNSIGNED NOT NULL COMMENT '系统ID',
  `msgid` INT(10) UNSIGNED NOT NULL COMMENT '消息ID',
  `item_uid` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '道具唯一ID',
  `item_id` INT(11) NOT NULL DEFAULT '0' COMMENT '道具ID',
  `item_count` INT(11) NOT NULL DEFAULT '0' COMMENT '道具数量',
  `item_attr` VARCHAR(512) NOT NULL DEFAULT '\'\'' COMMENT '道具属性' COLLATE 'utf8_bin',

  INDEX `tid` (`tid`),
  INDEX `player_id` (`player_id`)
) COLLATE='utf8_bin' ENGINE=InnoDB;

CREATE TABLE `carrier_@date@` (
  `tid` BIGINT(20) NOT NULL COMMENT '事务ID',
  `time` INT(11) NOT NULL COMMENT '发生时间',
  `server` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `carrier` INT(11) NOT NULL DEFAULT '0' COMMENT '航母ID',
  `level` INT(11) NOT NULL DEFAULT '0' COMMENT '航母等级',
  `exp` INT(11) NOT NULL DEFAULT '0' COMMENT '航母经验',
  `reform_level` INT(11) NOT NULL DEFAULT '0' COMMENT '航母改造等级',
  `old_level` INT(11) NOT NULL DEFAULT '0' COMMENT '旧航母改造等级',
  `old_exp` INT(11) NOT NULL DEFAULT '0' COMMENT '旧航母经验',
  `old_reform_level` INT(11) NOT NULL DEFAULT '0' COMMENT '旧航母改造等级',
  INDEX `tid` (`tid`),
  INDEX `player_id` (`player_id`)
) COLLATE='utf8_bin' ENGINE=InnoDB;

CREATE TABLE `carrier_delete_@date@` (
  `tid` BIGINT(20) NOT NULL COMMENT '事务ID',
  `time` INT(11) NOT NULL COMMENT '发生时间',
  `server` INT(10) UNSIGNED NOT NULL COMMENT '服务器ID',
  `player_id` BIGINT(20) NOT NULL COMMENT '玩家ID',
  `carrier` INT(11) NOT NULL DEFAULT '0' COMMENT '航母ID',
  `level` INT(11) NOT NULL DEFAULT '0' COMMENT '航母等级',
  `exp` INT(11) NOT NULL DEFAULT '0' COMMENT '航母经验',
  `reform_level` INT(11) NOT NULL DEFAULT '0' COMMENT '航母改造等级',
  INDEX `tid` (`tid`),
  INDEX `player_id` (`player_id`)
)COLLATE = 'utf8_bin' ENGINE = InnoDB;

CREATE TABLE `copy_@date@` (
  `tid` BIGINT(20) NULL DEFAULT NULL COMMENT '事务ID',
  `time` INT(11) NULL DEFAULT NULL COMMENT '发生时间',
  `server` INT(10) UNSIGNED NULL DEFAULT NULL COMMENT '服务器ID',
  `player_id` BIGINT(20) NULL DEFAULT NULL COMMENT '玩家ID',
  `copy_type` INT(11) NULL DEFAULT NULL COMMENT '副本类型',
  `copy_id` INT(11) NULL DEFAULT NULL COMMENT '副本Id',
  `copy_order` INT(11) NULL DEFAULT NULL COMMENT '副本顺序位',
  `star` TINYINT(4) NULL DEFAULT NULL COMMENT '副本星数',
  INDEX `tid` (`tid`),
  INDEX `player_id` (`player_id`),
  INDEX `copy_id` (`copy_id`)
) COLLATE='utf8_bin' ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `shop_@date@` (
  `tid` BIGINT(20) NOT NULL COMMENT '事务ID',
  `time` INT(11) NULL DEFAULT '0' COMMENT '发生时间',
  `server` INT(10) UNSIGNED NULL DEFAULT '0' COMMENT '服务器ID',
  `player_id` BIGINT(20) NOT NULL DEFAULT '0' COMMENT '玩家ID',
  `commodity_id` INT(10) NULL DEFAULT '0' COMMENT '商品ID',
  `shop_type` INT(10) NULL DEFAULT '0' COMMENT '商店类型',
  `buy_count` INT(10) NULL DEFAULT '0' COMMENT '购买次数',

  INDEX `tid` (`tid`),
  INDEX `player_id` (`player_id`)
) COLLATE = 'utf8_bin' ENGINE = InnoDB;

create table `army_exp_@date@` (
 `tid` bigint(20) NOT NULL COMMENT '事务ID',
 `time` int(11) DEFAULT '0' COMMENT '发生时间',
 `server` int(10) unsigned DEFAULT '0' COMMENT '服务器ID',
 `player_id` bigint(20) NOT NULL DEFAULT '0' COMMENT '玩家ID',
 `msgid` int(10) DEFAULT '0' COMMENT '消息ID',
 `army_id` bigint(20) DEFAULT '0' COMMENT '军团ID',
 `army_level` int(10) DEFAULT '0' COMMENT '军团等级',
 `army_exp` int(10) DEFAULT '0' COMMENT '军团经验',
 `army_exp_delta` int(10) DEFAULT '0' COMMENT '军团经验变化',
 `army_skill` varchar(128) DEFAULT NULL COMMENT '军团技能',

  KEY `tid` (`tid`),
  KEY `player_id` (`army_id`)
) ENGINE = InnoDB DEFAULT CHARSET = utf8 COLLATE = utf8_bin;

create table `mail_@date@` (
 `tid` bigint(20) NOT NULL COMMENT '事务ID',
 `time` int(11) DEFAULT '0' COMMENT '发生时间',
 `server` int(10) unsigned DEFAULT '0' COMMENT '服务器ID',
 `player_id` bigint(20) NOT NULL DEFAULT '0' COMMENT '玩家ID',
 `mail_id` bigint(20) DEFAULT '0' COMMENT '邮件ID',
 `mail_time` int(11) DEFAULT '0' COMMENT '邮件时间',
 `mail_type` int(10) DEFAULT '0' COMMENT '邮件类型',
 `mail_content` varchar(256) DEFAULT NULL COMMENT '邮件内容',
 `mail_reward` varchar(256) DEFAULT NULL COMMENT '邮件奖励',

  KEY `tid` (`tid`),
  KEY `player_id` (`player_id`)
) ENGINE = InnoDB DEFAULT CHARSET = utf8 COLLATE = utf8_bin;
