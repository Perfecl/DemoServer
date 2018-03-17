#include "stdafx.h"
#include "Incident.h"

void CIncident::SetIncidentByProto(const Incident& pto)
{
	m_strIncidentName = pto.name();

	m_bIsConditionTime = pto.is_condition_time();
	m_nConditionTime = pto.condition_time();

	m_bIsConditionPos = pto.is_condition_pos();
	m_nConditionPosCamp = pto.condition_pos_camp();
	m_fConditionPos = (float)pto.condition_pos();

	m_bIsConditionResource = pto.is_condition_resource();
	m_nConditionResourceCamp = pto.condition_resource_camp();
	m_nConditionResource = pto.condition_resource();

	m_bIsConditionHP = pto.is_condition_hp();
	m_nConditionHPGroup = pto.condition_hp_group();
	m_nConditionHP = pto.condition_hp();

	is_call_hero_ = pto.call_hero();

	m_bIsResultVictor = pto.is_result_victor();
	m_bIsResultLose = pto.is_result_lose();

	m_bIsResultGetBuff = pto.is_result_getbuff();
	m_nResultGetBuffGroup = pto.result_getbuff_group();
	m_nResultGetBuffID = pto.result_getbuff_id();

	m_bIsResultGetResource = pto.is_result_get_resource();
	m_nResultGetResourceCamp = pto.result_get_resource_camp();
	m_nResultGetResource = pto.result_get_resource();

	m_bIsResultBackupArrive = pto.is_result_backup_arrive();
	m_nResultBackupArriveGroup = pto.result_backup_arrive_group();

	m_bIsResultShowNotice = pto.is_result_show_notice();
	m_strShowNotice = ANSI_to_UTF8(pto.show_notice());

	m_bIsResultMasterTalk = pto.is_result_master_talk();
	m_nMasterNum = pto.master_num();
	m_strMasterTalk = ANSI_to_UTF8(pto.master_talk());

	m_bIsResultHeroTalk = pto.is_result_hero_talk();
	m_nHeroGroup = pto.hero_group();
	m_strHeroTalk = ANSI_to_UTF8(pto.hero_talk());

	m_bIsResultUnitRetreat = pto.is_result_unit_retreat();
	m_nUnitGroup = pto.unit_group();

	m_bIsResultRemoveBuff = pto.is_result_removebuff();
	m_nResultRemoveBuffGroup = pto.result_removebuff_group();
	m_nResultRemoveBuffID = pto.result_removebuff_id();

	m_bIsConditionUnitPos = pto.is_c_unit_pos();
	m_fUnitPos = (float)pto.unit_pos();
	m_nUnitPosGroup = pto.unit_pos_group();

	m_bIsChangResourceOverride = pto.is_chang_resource_override();
	m_nMasterId = pto.master_id();
	m_fResourceOverride = pto.resource_override();

	m_bIsBaseLv = pto.is_base_lv();
	m_nBaseLv = pto.base_lv();
	m_nBaseLvMaster = pto.base_lv_master();

	m_bIsStopRetreat = pto.is_stop_retreat();
	m_nStopRetratGroup = pto.stop_retrat_group();

	m_bIsPlayeStory = pto.is_play_story();
	m_nStoryId = pto.story_id();

	m_bIsCancelMark = pto.is_cancel_mark();

	m_bHeroDead = pto.is_hero_dead();
	m_bHeroDisappear = pto.is_hero_disappear();

	m_bResetHero = pto.is_reset_hero();
	m_bLockHero = pto.is_lock_hero();
	m_bUnlockHero = pto.is_unlock_hero();
	m_bLockSoldier = pto.is_lock_soldier();
	m_bUnlockSoldier = pto.is_unlock_soldier();
	m_bRemoveUnit = pto.is_remove_unit();
	m_nRemoveUnitGroup = pto.remove_unit();

	m_bChangeAI = pto.is_change_ai();
	m_nChangeAIGroup = pto.change_ai_group();
	m_nChangeAIType = pto.change_ai_type();
	m_nChangeAIRange = pto.change_ai_range();

	for (int k = 0; k < pto.condition_true_variable_id_size(); k++)
		m_vctConditionTrueVariableID.push_back(pto.condition_true_variable_id(k));

	for (int k = 0; k < pto.condition_false_variable_id_size(); k++)
		m_vctConditionFalseVariableID.push_back(pto.condition_false_variable_id(k));

	for (int k = 0; k < pto.change_true_variable_id_size(); k++)
		m_vctChangeTrueVariableID.push_back(pto.change_true_variable_id(k));

	for (int k = 0; k < pto.change_false_variable_id_size(); k++)
		m_vctChangeFalseVariableID.push_back(pto.change_false_variable_id(k));

	m_bOpenGate = pto.open_gate();
	m_nOpenGateID = pto.open_gate_id();

	m_bHeroUnavailable = pto.hero_unavailable();

	m_bMasterDisappear = pto.master_disappear();
	m_nMasterDisappearID = pto.master_disappear_id();

	m_bBeginTimer = pto.begin_timer();
	m_nBeginTimerID = pto.begin_timer_id();

	m_bNeedTimer = pto.need_timer();
	m_nNeedTimerID = pto.need_timer_id();
	m_nNeedTimerTime = pto.need_timer_time();

	m_bUnitDropBullet = pto.unit_drop_bullet();
	m_nDropBulletGroup = pto.drop_bullet_group();
	m_nDropBulletId = pto.drop_bullet_id();
	m_nBulletValue = pto.bullet_value();

	m_bDeleteBullet = pto.delete_bullet();
	m_nDeleteBulletCustomID = pto.delete_bullet_custom_id();

	m_bBossEnter = pto.boss_enter();

	open_timer_ = pto.open_timer();
	timer_time_ = pto.timer_time();
	timer_describe_ = ANSI_to_UTF8(pto.timer_describe().c_str()); 
	close_timer_ = pto.close_timer();

	set_backup_mark_ = pto.set_backup_mark();
	backup_mark_pos_ = pto.backup_mark_pos();
	cancel_backup_mark_ = pto.cancel_backup_mark();

	change_base_level_ = pto.change_base_level();
	change_base_level_maseter_ = pto.change_base_level_master();
	change_base_level_lv_ = pto.change_base_level_lv();
}
