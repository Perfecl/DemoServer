CREATE TABLE `unique_id_gen` (
  `id` BIGINT(20) UNSIGNED NOT NULL AUTO_INCREMENT COMMENT '唯一ID',
  `server` INT(11) UNSIGNED NOT NULL DEFAULT '0' COMMENT '服务器ID',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=10001;
REPLACE INTO unique_id_gen(server) VALUES(0);

DROP TABLE IF EXISTS `account_0`;
CREATE TABLE IF NOT EXISTS `account_0` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin' COMMENT '第三方平台ID',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin' COMMENT '平台ID,暂时未使用',
  `server` INT(11) UNSIGNED NOT NULL COMMENT '服务器ID',
  `uid` bigint(20) NOT NULL COMMENT '玩家唯一ID',
  `create_time` int(11) NOT NULL COMMENT '创建UID时间',
  `last_login_time` int(11) NOT NULL COMMENT '最后登录时间',
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_1`;
CREATE TABLE IF NOT EXISTS `account_1` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_2`;
CREATE TABLE IF NOT EXISTS `account_2` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_3`;
CREATE TABLE IF NOT EXISTS `account_3` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_4`;
CREATE TABLE IF NOT EXISTS `account_4` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_5`;
CREATE TABLE IF NOT EXISTS `account_5` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_6`;
CREATE TABLE IF NOT EXISTS `account_6` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_7`;
CREATE TABLE IF NOT EXISTS `account_7` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_8`;
CREATE TABLE IF NOT EXISTS `account_8` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_9`;
CREATE TABLE IF NOT EXISTS `account_9` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_10`;
CREATE TABLE IF NOT EXISTS `account_10` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_11`;
CREATE TABLE IF NOT EXISTS `account_11` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_12`;
CREATE TABLE IF NOT EXISTS `account_12` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_13`;
CREATE TABLE IF NOT EXISTS `account_13` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_14`;
CREATE TABLE IF NOT EXISTS `account_14` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `account_15`;
CREATE TABLE IF NOT EXISTS `account_15` (
  `openid` varchar(64) NOT NULL COLLATE 'utf8_bin',
  `platform` char(16) NOT NULL COLLATE 'utf8_bin',
  `server` INT(11) UNSIGNED NOT NULL,
  `uid` bigint(20) NOT NULL,
  `create_time` int(11) NOT NULL,
  `last_login_time` int(11) NOT NULL,
  PRIMARY KEY (`openid`,`server`),
  UNIQUE KEY `uid` (`uid`)
) ENGINE=InnoDB COLLATE='utf8_bin';

DROP TABLE IF EXISTS `password`;
CREATE TABLE `password` (
  `account` VARCHAR(64) NOT NULL,
  `password` VARCHAR(64) NOT NULL,
  `date` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`account`)
) ENGINE=InnoDB COLLATE='utf8_bin';

