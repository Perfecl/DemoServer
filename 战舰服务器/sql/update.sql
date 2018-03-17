--2017-10-10--
alter table copy_0 add column `medal_copy_id` INT(11) NULL DEFAULT '0' COMMENT '勋章副本ID',
add column `medal_star`  INT(11) NULL DEFAULT '0' COMMENT '勋章集册星数',
add column `medal_state` VARCHAR(4096)  NULL COMMENT '勋章集册状态',
add column `medal_achi`INT(11) NULL DEFAULT '0' COMMENT '勋章集册成就';
