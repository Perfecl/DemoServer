#pragma once
#include <cpp/message.pb.h>
#include <vector>
#include <common_define.h>
#include <config_file.h>
#include <config_entry.h>
#include <logger.h>
#include <pugixml.hpp>
#include "define.h"
#include <vector_set.h>
#include <vector_map.h>

#define DEF_ENTRY(NAME1)                                \
struct NAME1;                                           \
typedef boost::shared_ptr<NAME1> NAME1##Ptr;            \
struct NAME1 : public ConfigEntry

#define DEF_CONFIG_FILE(NAME)                           \
class NAME : public XmlConfigFile {                     \
  public:                                               \
    NAME(const std::string& file_name) : XmlConfigFile(file_name) {}  \
  protected:                                            \
    virtual bool Parse();                               \
}


typedef std::vector<ValuePair2<int32_t, int32_t> > ValuePair2Vec;
typedef std::vector<ValuePair2<int64_t, int64_t> > ValuePair2VecInt64;
typedef std::vector<ValuePair3<int32_t, int32_t, int32_t> > ValuePair3Vec;

void AddConfigFile(ConfigFile *file);
void AfterLoadConfig();

class LogicPlayer;

class ServerConfig : public XmlConfigFile {
 public:
  ServerConfig(const std::string& file_name) : XmlConfigFile(file_name) {
    this->recharge_platform_ = 1;
  }

  int32_t GetAttrAsInt(const char* xpath, const char* attr);
  std::string GetAttrAsString(const char* xpath, const char* attr);

  //first 是port
  //second是监听端口的类型(Player, GM之类的)
  typedef std::pair<uint16_t, uint16_t> ListenInfo;
  //监听的端口
  const std::vector<ListenInfo>& ports() const { return listen_ports_;}
  //日志等级
  LoggerLevel log_level() const { return log_level_; }
  //是否开启战报log
  bool report() const { return this->enable_report_; }
  //是否开启战报内部属性log
  bool report_attr() const { return this->enable_report_attr_; }
  //是否开启脏词过滤
  bool dirtywords() const { return this->enable_dirtywords_; }
  //是否开启客户端GM指令
  bool test_gm() const { return this->test_gm_; }
  //原始服务器ID
  uint32_t server_id() const { return original_server_id_; }
  //服务器的编号
  const std::vector<uint32_t>& server_ids() const { return server_ids_; }
  //存档服务器地址
  const std::pair<std::string, std::string>& record_server_addr() const {
    return record_server_addr_;
  }
  //认证服务器地址
  const std::pair<std::string, std::string>& auth_server_addr() const {
    return auth_server_addr_;
  }
  //中心服务器地址
  const std::pair<std::string, std::string>& center_server_addr() const {
    return center_server_addr_;
  }

  int32_t recharge_platform() const { return this->recharge_platform_; }
  void version(std::string ver) { this->version_ = ver; }
  const std::string& version() const { return this->version_; }
 protected:
  virtual bool Parse();
 private:
  std::vector<ListenInfo> listen_ports_;
  std::string platform_;
  LoggerLevel log_level_;
  bool enable_report_;
  bool enable_report_attr_;
  bool enable_dirtywords_;
  bool test_gm_;
  uint32_t original_server_id_;
  std::vector<uint32_t> server_ids_;
  std::pair<std::string, std::string> record_server_addr_;
  std::pair<std::string, std::string> auth_server_addr_;
  std::pair<std::string, std::string> center_server_addr_;
  std::string version_;
  int32_t recharge_platform_;
};

#define SETTING_KEY(k) const std::string k = #k
namespace Setting {
  SETTING_KEY(born_ship_id);
  SETTING_KEY(carrier_born_id);
  SETTING_KEY(born_oil);
  SETTING_KEY(born_engry);

  SETTING_KEY(oil_recover_time);
  SETTING_KEY(energy_recover_time);
  SETTING_KEY(oil_recover_limit);
  SETTING_KEY(oil_max_limit);
  SETTING_KEY(energy_recover_limit);
  SETTING_KEY(energy_max_limit);

  SETTING_KEY(name_min_byte);
  SETTING_KEY(name_max_byte);

  SETTING_KEY(ship_start_id);
  SETTING_KEY(ship_end_id);
  //抽船的四种材料
  SETTING_KEY(cre_boat_mat_a);
  SETTING_KEY(cre_boat_mat_b);
  SETTING_KEY(cre_boat_mat_c);
  SETTING_KEY(cre_boat_mat_d);
  //四种材料最少的个数
  SETTING_KEY(cre_a_need);
  SETTING_KEY(cre_b_need);
  SETTING_KEY(cre_c_need);
  SETTING_KEY(cre_d_need);
  //抽船冷却时间
  SETTING_KEY(cre_cool_time);
  //每N次元宝抽船(奖励)
  SETTING_KEY(exh_adv_times);
  SETTING_KEY(exh_adv10_times);
  //每次元宝抽船消耗元宝数
  SETTING_KEY(exh_gem_need);
  SETTING_KEY(exh_gem10_need);
  SETTING_KEY(creboat_free_time);
  //采购1次船舰消耗道具id
  SETTING_KEY(exh_item_id);
  //研发1次船舰消耗道具id
  SETTING_KEY(cre_item_id);
  //每日免费研发次数
  SETTING_KEY(cre_free_num);
  //每日免费研发cd（秒）
  SETTING_KEY(cre_free_cd);
  //采购1次船舰消耗道具个数
  SETTING_KEY(exh_item_num);
  //研发1次船舰消耗道具个数
  SETTING_KEY(cre_item_num);

  //船只上阵等级限制
  SETTING_KEY(seat2_open_lv);
  SETTING_KEY(seat3_open_lv);
  SETTING_KEY(seat4_open_lv);
  SETTING_KEY(seat5_open_lv);
  SETTING_KEY(seat6_open_lv);
  //爬塔的奖励
  SETTING_KEY(server_tower_award_box_id1);
  SETTING_KEY(server_tower_award_box_id2);

  //军团限时商店，刷新时刻。
  SETTING_KEY(shop9_refresh_time);
  //钻石商店-每日优惠，刷新时刻
  SETTING_KEY(shop10_refresh_time);
  //建设有几率发动暴击
  SETTING_KEY(league_sign_reward);

  //体力丹
  SETTING_KEY(oilr_id);
  //精力丹
  SETTING_KEY(energy_id);

  //装备改造
  SETTING_KEY(equipreform_exp);
  //船升级
  SETTING_KEY(shiplvup_exp);
  //飞机升级
  SETTING_KEY(planelvup_exp);
  //清除CD花费
  SETTING_KEY(cre_clearcd_need);
  //船舰突破的道具ID
  SETTING_KEY(shipreform_id);
  //最大抽卡次数
  SETTING_KEY(card_special_limit);
  SETTING_KEY(card_special_cost);
  //装备的位置
  SETTING_KEY(equip_position);
  //重置系统的时间
  SETTING_KEY(daily_refresh_time);
  //合成飞机需要的个数
  SETTING_KEY(cre_plane_num);
  //船洗属性的道具ID
  SETTING_KEY(train_id);
  //战舰装备重组消耗钻石数量
  SETTING_KEY(recombine_cost);
  //海军预备役经验值
  SETTING_KEY(army_exp);
  //海军争霸免战令时间
  SETTING_KEY(army_armistice_time);
  //签到循环起始id
  SETTING_KEY(sign_loop_id);
  //天命参数1
  SETTING_KEY(fate_prm1);
  //装备回收返还银币
  SETTING_KEY(equip_goldreturn_rate);

  //刷新商店初始免费次数
  SETTING_KEY(shopX_free_num);
  //刷新商店免费次数恢复1次时间（秒
  SETTING_KEY(shopX_recover_time);
  //刷新商店刷新道具id
  SETTING_KEY(shopX_refresh_id);
  //刷新商店刷新消耗资源类型
  SETTING_KEY(shopX_refresh_resource1);
  //刷新商店刷新消耗资源
  SETTING_KEY(shopX_refresh_resource2);

  //航母兑换
  SETTING_KEY(carrier_recover);
  //竞技场次数
  SETTING_KEY(arena_battle_num);
  //竞技场CD
  SETTING_KEY(arena_battle_time);
  //战舰研发消耗道具id
  SETTING_KEY(ship_fate_id);
  //领地攻讨单位时间经精力消耗（每小时)
  SETTING_KEY(patrol_cost);
  //领地攻讨4小时战舰碎片奖励概率（碎片数|权重概率）
  SETTING_KEY(patrol_1_award);
  //领地攻讨8小时战舰碎片奖励概率（碎片数|权重概率）
  SETTING_KEY(patrol_2_award);
  //领地攻讨12小时战舰碎片奖励概率（碎片数|权重概率）
  SETTING_KEY(patrol_3_award);
  //领地攻讨收益间隔时间（vip开启|收益间隔时间，分钟）
  SETTING_KEY(patrol_awardtime);
  //领地攻讨挂机时间（小时)
  SETTING_KEY(patrol_time);
  //竞技场刷新对手
  SETTING_KEY(arena_front_num);
  SETTING_KEY(arena_behind_num);
  //名将副本每日次数
  SETTING_KEY(general_day_times);
  //军衔起始结束ID
  SETTING_KEY(rank_start_id);
  SETTING_KEY(rank_end_id);
  //首次采购10次必定掉落橙将包
  SETTING_KEY(exh_get_id);
  //各个援军位开启等级
  SETTING_KEY(rescuearmy_open_lv);

  //海军碎片起始id
  SETTING_KEY(armyparts_start_id);
  //海军碎片结束id
  SETTING_KEY(armyparts_end_id);
  //部队抢夺单次消耗精力值
  SETTING_KEY(army_grab_expend);
  //围剿boss,征讨令，回复时间
  SETTING_KEY(dstrike_token_time);
  //围剿BOSS,征讨令上限
  SETTING_KEY(dstrike_token_num);
  //围剿boss,失效时间
  SETTING_KEY(dstrike_boss_time);
  //围剿BOSS令牌ID
  SETTING_KEY(dstrike_token_id);
  //围剿BOSS最后一击奖励
  SETTING_KEY(dstrike_kill_award);
  //围剿BOSS发现奖励
  SETTING_KEY(dstrike_find_award);
  //围剿BOSS令牌兑换功勋
  SETTING_KEY(dstrike_token_reward);
  //围剿BOSS活动时间
  SETTING_KEY(dstrike_activity_time);
  //围剿BOSS 2.5倍地图
  SETTING_KEY(dstrike_special_id);
  //爬塔一键三星
  SETTING_KEY(oil_3stars);
  //领地攻讨灭火奖励
  SETTING_KEY(patrol_help_reward);
  //飞机最大堆叠数量
  SETTING_KEY(plane_stack_max);
  //N次副本之后必掉
  SETTING_KEY(award_hero_drop_count);
  //暴击系数
  SETTING_KEY(crit_o);
  //怒气上限
  SETTING_KEY(fury_max);
  //航母攻击加成系数
  SETTING_KEY(carrier_x);
  //每天领取好友精力上限制
  SETTING_KEY(friend_jingli_max);
  //夺宝怪物组
  SETTING_KEY(duobao_monstergroup);
  //聊天CD
  SETTING_KEY(chat_cd_time);
  //七日活动可领奖时间
  SETTING_KEY(sevendays_award_day);
  //十四日活动可领时间
  SETTING_KEY(sevendays14_award_day);
  //竞技场发奖时间
  SETTING_KEY(arena_award_time);
  //航母升星材料
  SETTING_KEY(carrier_advanceditem_id);
  //航母-经验道具
  SETTING_KEY(carrierlvup_exp);
  //航母品质|护佑加成%
  SETTING_KEY(carrier_mount);
  //围剿海盗征讨
  SETTING_KEY(dstrike_item_mapped);
  //刷新商店列表
  SETTING_KEY(refresh_shop_type);
  //初始道具
  SETTING_KEY(initial_carry_id);

  //天命升级相关
  SETTING_KEY(fate_mean);
  SETTING_KEY(fate_stddev);
  SETTING_KEY(fate_min);
  SETTING_KEY(fate_max);
  //改名消耗
  SETTING_KEY(change_name_cost);

  //黑名单上限
  SETTING_KEY(friend_black_max);
  //好友上限
  SETTING_KEY(friend_max);

  //募集军资
  SETTING_KEY(God_of_Wealth_money_drop);
  SETTING_KEY(God_of_Wealth_oil_drop);
  SETTING_KEY(God_of_Wealth_cd);
  SETTING_KEY(God_of_Wealth_click_everyday);
  SETTING_KEY(God_of_Wealth_total_click);

  //首充礼包
  SETTING_KEY(first_charge_reward);
  //航母支援开启等级
  SETTING_KEY(carrier_RemoteSupport);
  //联盟创建消耗黄金
  SETTING_KEY(league_create);
  //创建公会需要的vip等级
  SETTING_KEY(league_create_VIP_level);
  //同时可申请的联盟
  SETTING_KEY(league_applicant_restriction1);
  //主动退出联盟的惩罚
  SETTING_KEY(league_leave_time);
  //军团战役初始次数
  SETTING_KEY(league_copy_free_time);
  //军团战役恢复速度
  SETTING_KEY(league_copy_recover_time);
  //军团战役开始恢复时间
  SETTING_KEY(league_copy_time1);
  SETTING_KEY(league_copy_time2);
  //公会副会长人数限制
  SETTING_KEY(league_controller_limit);

  //世界boss
  SETTING_KEY(pirate_boss_activity_time1);
  SETTING_KEY(pirate_boss_activity_time2);
  SETTING_KEY(pirate_boss_challenge_time);
  SETTING_KEY(pirate_boss_buff);
  SETTING_KEY(pirate_boss_monster_group);
  SETTING_KEY(pirate_boss_recover_time);
  SETTING_KEY(pirate_boss_lucky_hit_reward);
  SETTING_KEY(pirate_boss_last_hit_reward);
  SETTING_KEY(pirate_boss_every_hit_reward);

  //订制
  SETTING_KEY(hero_recruit_everyday_times);
  SETTING_KEY(hero_recruit_reset);
  SETTING_KEY(hero_recruit_cost1);
  SETTING_KEY(hero_recruit_cost10);
  SETTING_KEY(hero_recruit_switchcountry);
  SETTING_KEY(hero_recruit_cost_item);
  SETTING_KEY(hero_recruit_drop1);
  SETTING_KEY(hero_recruit_drop2);
  SETTING_KEY(hero_recruit_itemid);
  SETTING_KEY(hero_recruit_drop3);
  SETTING_KEY(hero_recruit_drop4);

  SETTING_KEY(limited_recruit_startday);  //限时招募，开放时间开服第X天
  SETTING_KEY(limited_recruit_endday);  //限时招募, 结束时间开服第X天
  SETTING_KEY(limited_recruit_1gold);   //招募1次X元宝, 10积分。
  SETTING_KEY(limited_recruit_10gold);   //招募10次X元宝，100积分。
  SETTING_KEY(limited_recruit_1score);   //招募1次300元宝, X积分。
  SETTING_KEY(limited_recruit_10score);  //招募10次2680元宝，X积分。
  SETTING_KEY(limited_recruit_1drop);    //招募1次，掉落包X。

  //跨服积分战
  SETTING_KEY(crossserverpk1_rankpk_time1);
  SETTING_KEY(crossserverpk1_rankpk_time2);
  SETTING_KEY(crossserverpk1_winnerpk_time1);
  SETTING_KEY(crossserverpk1_winnerpk_time2);
  SETTING_KEY(crossserverpk1_winnerpk_time3);
  SETTING_KEY(crossserverpk1_winnerpk_time4);
  SETTING_KEY(crossserverpk1_challenge_count);
  SETTING_KEY(crossserverpk1_refresh_count);
  SETTING_KEY(crossserverpk1_buff);
  SETTING_KEY(crossserverpk1_fight_award1);
  SETTING_KEY(crossserverpk1_fight_award2);

  //二次充值奖励
  SETTING_KEY(second_charge_reward);
  SETTING_KEY(second_charge_condition);
  //一元购
  SETTING_KEY(oneRMB_charge_reward);
  //制霸全球怪物组
  SETTING_KEY(legion_war_monstergroup);
  //制霸全球开放日期
  SETTING_KEY(legion_war_region_time1);
  //制霸全球开放时间
  SETTING_KEY(legion_war_region_time2);

  SETTING_KEY(legion_foreplay_challenge_count);  //制霸太平洋前戏，战斗次数，每隔X小时恢复1次
  SETTING_KEY(legion_foreplay_challenge_recovertime); //制霸太平洋前戏，战斗次数，每隔X小时恢复1次
  SETTING_KEY(legion_foreplay_endday);           //制霸太平洋前戏，活动时间为开服第1 - X天。
  SETTING_KEY(legion_foreplay_forecast);         //制霸太平洋前戏，主界面出现“拯救太平洋”系统预告的等级
  SETTING_KEY(legion_foreplay_time1);            //制霸太平洋前戏
  SETTING_KEY(weekly_card_sign);

  SETTING_KEY(limited_recruit_0score);  //限时采购最低上榜积分

  SETTING_KEY(activity_equip_1gold);    //深海探宝1次花费
  SETTING_KEY(activity_equip_10gold);   //深海探宝10次花费
  SETTING_KEY(activity_equip_1score);   //深海探宝1次花费获得积分
  SETTING_KEY(activity_equip_freetime); //深海探宝每日免费次数
  SETTING_KEY(activity_equip_1drop);    //深海探宝每次掉落包
  SETTING_KEY(activity_equip_ntimes);   //深海探宝X次必得掉落包
  SETTING_KEY(activity_equip_10more);   //深海探宝必得掉落包

  SETTING_KEY(activity_carrier_1gold);
  SETTING_KEY(activity_carrier_10gold);
  SETTING_KEY(activity_carrier_1score);
  SETTING_KEY(activity_carrier_freetime);
  SETTING_KEY(activity_carrier_1drop);
  SETTING_KEY(activity_carrier_score1); //普通榜
  SETTING_KEY(activity_carrier_score2); //精英榜
  SETTING_KEY(activity_carrier_ntimes);
  SETTING_KEY(activity_carrier_10more);

  SETTING_KEY(legion_war_region_score1);
  SETTING_KEY(legion_war_region_score2);

  SETTING_KEY(recruit1_reward1);
  SETTING_KEY(recruit2_reward1);

  SETTING_KEY(festival_time0);
  SETTING_KEY(festival_time1);
  SETTING_KEY(festival_time2);
  SETTING_KEY(festival_drop);

  SETTING_KEY(legion_war_region_award1);
  SETTING_KEY(legion_war_region_award2);

  SETTING_KEY(plane_transfer_cost);

  SETTING_KEY(login_sign_cost);  //签到补签价格(元宝)
  SETTING_KEY(activity_open_day);

  SETTING_KEY(version_reward_id);

  SETTING_KEY(medal_copy_id);                //获得将灵副本的id，掉落包X。（从掉落包中取，得到的id就是medal_copy.id）
  SETTING_KEY(medal_recruit_1gold);  //  勋章搜索1次消耗纪念币（id = 23900160）
  SETTING_KEY(medal_recruit_5gold);  //  勋章搜索5次消耗纪念币（id = 23900160）
  SETTING_KEY(medal_recruit_reward1);  //  勋章搜索每次获得10000美元（id = 1）
  SETTING_KEY(medal_recruit_1drop);          //  勋章搜索1次，掉落包X。(青铜数量根据获得的勋章品质而定)
  SETTING_KEY(medal_recruit_5drop);  //  勋章搜索, 保底掉落包X。
  SETTING_KEY(medal_recruit_number5);  //  勋章搜索, 每X次获得1次保底。
  SETTING_KEY(medal_recruit_show);     //  勋章搜索预览，掉落包X。
  SETTING_KEY(medal_recruit_freetime);  //  勋章搜索, 每24小时X次免费次数
  SETTING_KEY(medal_number2);  //  勋章历练, 每天免费刷新挑战对象X次。
  SETTING_KEY(medal_number3);  //  勋章历练, 每天付费刷新挑战对象，X黄金1次。
  SETTING_KEY(medal_recruit_reward2); //勋章搜索,勋章品质

  SETTING_KEY(league_replace_day);

  SETTING_KEY(chat_min_lv);

  SETTING_KEY(pearlharbor_battle_num);  //捍卫珍珠港每天挑战次数
  SETTING_KEY(pearlharbor_buff_num);    //每个轮玩法周期buff使用次数
  SETTING_KEY(pearlharbor_time);        //每次buff开启持续时间（秒）
  SETTING_KEY(pearlharbor_missing_enemy);  //每天遗漏敌人数量
  SETTING_KEY(pearlharbor_time1);  // 捍卫珍珠港每周开启时间（每周三10 : 00至周日22 : 00）
  SETTING_KEY(pearlharbor_time2);  //捍卫珍珠港结束时间
  SETTING_KEY(pearlharbor_time3);  //捍卫珍珠港第1波怪（几时开始 | 几时结束）
  SETTING_KEY(pearlharbor_time4);  // 捍卫珍珠港第2波怪（几时开始 | 几时结束）
  SETTING_KEY(pearlharbor_time5);  // 捍卫珍珠港第3波怪（几时开始 | 几时结束）
  SETTING_KEY(pearlharbor_buff_effect);  // 捍卫珍珠港buff效果
  SETTING_KEY(pearlharbor_start_time);   //捍卫珍珠港开始时间

  SETTING_KEY(limited_recruit_starttime);   //限时招募，开放时间。（含X） 20171030
  SETTING_KEY(limited_recruit_endtime);     //限时招募，结束时间。（含X） 20171101

  SETTING_KEY(hero_back_start_time);  //英雄回归最低触发天数
  SETTING_KEY(hero_back_max_time);    //英雄回归最长延续天数


  enum {
    kDistribution = 10000,
    kResearchItemKindCount = 4,  //四种材料
  };

  extern time_t kFestivalStartTime;
  extern time_t kFestivalEndTime;
  extern int32_t kArenaAwardTime;

  extern int32_t kDstrikeTokenID;
  extern int32_t kDstrikeTokenNum;
  extern int32_t kDstrikeTokenTime;

  extern int32_t kRecoverOilTime;
  extern int32_t kOilMax;
  extern int32_t kOilRecoverMax;

  extern int32_t kRecoverEnergyTime;
  extern int32_t kEnergyMax;
  extern int32_t kEnergyRecoverMax;

  extern int32_t kMaxLevel;

  extern int32_t kNameMinLen;
  extern int32_t kNameMaxLen;
  //船只ID
  extern int32_t kShipStartID;
  extern int32_t kShipEndID;
  //船只上阵等级
  extern int32_t kSeatCount[7];
  //装备改造经验
  extern std::pair<int32_t, int32_t> kEquipRefineExp[5];
  //清除造船CD花费
  extern ValuePair2<int32_t, int32_t> kClearCDResearchHero;
  //重置系统的时间点
  extern int32_t kRefreshSeconds;
  //暴击系数
  extern int32_t kCrit;

  extern time_t kLimitedRecruitStarttime;
  extern time_t kLimitedRecruitEndtime;

  //是否在活动中
  bool IsInFestival();

  int64_t GetValue(const std::string& key);
  const std::vector<int32_t>& GetValue1(const std::string& key);
  const ValuePair2Vec& GetValue2(const std::string& key);

  int32_t GetValueInVec2(const std::string& key, int32_t first);
  int32_t GetMaxValueInVec2(const std::string& key, int32_t first);

  void AddValue(const std::string& key, int64_t value);
  void AddValue(const std::string& key, std::vector<int32_t>& value);
  void AddValue(const std::string& key, ValuePair2Vec& value);

  int32_t GetGeneralCopyCount(LogicPlayer* player);
};

#define GetSettingValue(NAME) Setting::GetValue(Setting::NAME)

DEF_ENTRY(PrefixBase) {
  bool Fill(pugi::xml_node node);
  static std::string MakeName(uint32_t server_id, const std::string& name);

  std::string prefix;
};
#define PREFIX_BASE ConfigEntryManager<PrefixBase>::Instance()

DEF_CONFIG_FILE(SettingConfigFile);

class DirtyWordsConfigFile : public ConfigFile {
 public:
  DirtyWordsConfigFile(const std::string& file_name) : ConfigFile(file_name) {}

 protected:
  virtual bool Parse();
};

struct FirstServerRewardAccount {
  std::string openid;
  int32_t money;  //RMB
  int32_t month_card;
  int32_t life_card;
  int32_t weekly_card;
  int32_t month_card_1;
  int32_t month_card_2;
};

const FirstServerRewardAccount* GetFirstServerReward(const std::string& openid);

class Server70001ConfigFile : public ConfigFile {
 public:
  Server70001ConfigFile(const std::string& file_name) : ConfigFile(file_name) {}

 protected:
  virtual bool Parse();
};

class VersionConfigFile : public XmlConfigFile {
 public:
  VersionConfigFile(const std::string& file_name) : XmlConfigFile(file_name) {}

 protected:
  virtual bool Parse();
};

DEF_ENTRY(HeroBase) {
  bool Fill(pugi::xml_node node);

  int32_t GetLevelUpExp(int32_t current_level) const;

  int8_t quality;
  int8_t job;
  int8_t country;
  ValuePair2<int32_t, int32_t> skill;
  std::vector<int32_t> combo; //合体技能
  int32_t super_combo;        //超合体技
  int32_t make_cd;          //建造CD
  int32_t breakadvancedid;  //突破组ID
  ValuePair2Vec talent1;    //突破天赋
  std::vector<int32_t> karma1;  //缘分

  int16_t fury;             //怒气
  int32_t train_id;         //培养组ID
  ValuePair2<int32_t, int32_t> ship_piece; //船只合成
  ValuePair2<int32_t, int32_t> sell;

  int32_t wake_itemtree;
};

#define HERO_BASE ConfigEntryManager<HeroBase>::Instance()

DEF_ENTRY(RelationBase) {
  bool Fill(pugi::xml_node node);

  std::vector<int32_t> hero1_id;
  int32_t equip;
  int32_t target;
  ValuePair2Vec pro;
};
#define RELATION_BASE ConfigEntryManager<RelationBase>::Instance()

DEF_CONFIG_FILE(HeroConfigFile);

DEF_ENTRY(BreakTalentBase) {
  bool Fill(pugi::xml_node node);

  int32_t target;
  ValuePair2Vec buff_type;
};
#define BREAK_TALENT ConfigEntryManager<BreakTalentBase>::Instance()

struct BreakAdvAttr {
  BreakAdvAttr() { memset(this, 0, sizeof(*this)); }
  int32_t attr[sy::ATTACK_ATTR_FF + 1];

  int32_t operator[](int32_t index) const {
    if (index >= sy::ATTACK_ATTR_HP && index <= sy::ATTACK_ATTR_FF) {
      return attr[index];
    }
    return 0;
  }
};

DEF_ENTRY(BreakAdvancedBase) {
  bool Fill(pugi::xml_node node);

  static BreakAdvAttr GetBreakAdvAttr(int32_t group_id, int32_t grade,
                                      int32_t level);

  int32_t need_hero_lv;
  int32_t coin_cost;
  int32_t item_cost;
  int32_t hero_cost;

  std::vector<int32_t> base_attribute;
  std::vector<int32_t> grow_attribute;
};
#define BREAK_ADVANCED ConfigEntryManager<BreakAdvancedBase>::Instance()
inline int32_t BreakAdvancedID(int32_t group_id, int32_t grade) {
  return group_id * 100 + grade;
}

DEF_CONFIG_FILE(TalentConfigFile);

DEF_ENTRY(ItemBase) {
  bool Fill(pugi::xml_node node);

  static VectorSet<int32_t> navy_item;

  int8_t quality;
  int8_t can_lost;
  int32_t max_stack;
  ValuePair2<int32_t, int32_t> sell;
  int32_t effect;
  int32_t need_lv;
};
#define ITEM_BASE ConfigEntryManager<ItemBase>::Instance()

DEF_CONFIG_FILE(ItemConfigFile);

DEF_ENTRY(EquipBase) {
  bool Fill(pugi::xml_node node);

  int32_t GetGaiZaoTianFuLevel(int32_t level) const {
    for (ValuePair2Vec::const_reverse_iterator iter = gaizao_tainfu.rbegin();
         iter != gaizao_tainfu.rend(); ++iter) {
      if (level >= iter->v1) return iter->v1;
    }
    return 0;
  }

  int32_t GetShenJinTainFuLevel(int32_t level) const {
    for (ValuePair2Vec::const_reverse_iterator iter = shengjin_tainfu.rbegin();
         iter != shengjin_tainfu.rend(); ++iter) {
      if (level >= iter->v1) return iter->v1;
    }
    return 0;
  }

  int8_t quality;
  int8_t type;
  int32_t suit;
  ValuePair2Vec baseproperty;
  ValuePair2Vec upproperty;
  ValuePair2Vec refproperty;
  ValuePair2<int32_t, int32_t> equip_piece; //装备合成
  ValuePair2<int32_t, int32_t> sell;
  ValuePair2Vec gaizao_tainfu;
  ValuePair2Vec shengjin_tainfu;
};
#define EQUIP_BASE ConfigEntryManager<EquipBase>::Instance()

DEF_ENTRY(SuitBase) {
  bool Fill(pugi::xml_node node);

  int32_t suit;
  int32_t num;
  ValuePair2Vec property;
};
#define SUIT_BASE ConfigEntryManager<SuitBase>::Instance()

DEF_ENTRY(EquipLevelUpBase) {
  bool Fill(pugi::xml_node node);

  //first是money类型
  //second是money的值,值是0表示不能升级
  ValuePair2<int32_t, int32_t> GetMoneyByTyEquipQuality(int32_t quality) const;

  ValuePair2<int32_t, int32_t> green;
  ValuePair2<int32_t, int32_t> blue;
  ValuePair2<int32_t, int32_t> puple;
  ValuePair2<int32_t, int32_t> orange;
  ValuePair2<int32_t, int32_t> red;
  ValuePair2<int32_t, int32_t> gold;
};
#define EQUIP_LEVEL_UP_BASE ConfigEntryManager<EquipLevelUpBase>::Instance()

DEF_ENTRY(EquipRefineBase) {
  bool Fill(pugi::xml_node node);

  int32_t GetExpByEquipQuality(int32_t quality) const;

  int32_t green;
  int32_t blue;
  int32_t puple;
  int32_t orange;
  int32_t red;
  int32_t gold;
};
#define EQUIP_REFINE_BASE ConfigEntryManager<EquipRefineBase>::Instance()

DEF_ENTRY(EquipMasterBase) {
  bool Fill(pugi::xml_node node);

  static const EquipMasterBase* GetEquipMasterBase(int32_t group_id, int32_t level);

  int32_t need_lv;
  ValuePair2Vec property;
};
#define EQUIP_MASTER_BASE ConfigEntryManager<EquipMasterBase>::Instance()

DEF_CONFIG_FILE(EquipConfigFile);

DEF_ENTRY(ExpBase) {
  ExpBase() : ship_exp(&ship1_exp), carrier_exp(&carrier1_exp) {}

  bool Fill(pugi::xml_node node);

  int32_t GetShipExp(int32_t quality) const {
    if (!(quality >= sy::QUALITY_WHITE && quality <= sy::QUALITY_RED)) return 0;
    return ship_exp[quality - 1];
  }

  int32_t GetCarrierExp(int32_t quality) const {
    if (!(quality >= sy::QUALITY_WHITE && quality <= sy::QUALITY_RED)) return 0;
    return carrier_exp[quality - 1];
  }

  int32_t role_exp;
  int32_t *ship_exp;

  int32_t ship1_exp;
  int32_t ship2_exp;
  int32_t ship3_exp;
  int32_t ship4_exp;
  int32_t ship5_exp;
  int32_t ship6_exp;

  int32_t *carrier_exp;

  int32_t carrier1_exp;
  int32_t carrier2_exp;
  int32_t carrier3_exp;
  int32_t carrier4_exp;
  int32_t carrier5_exp;
  int32_t carrier6_exp;

  int32_t add_power;
  int32_t add_energy;
  //体力对应经验和金币
  int32_t power_exp;
  int32_t power_money;
  //精力对应的经验和金币
  int32_t energy_exp;
  int32_t energy_money;
};
#define EXP_BASE ConfigEntryManager<ExpBase>::Instance()

DEF_ENTRY(VipExpBase) {
  bool Fill(pugi::xml_node node);

  int32_t vip_exp;
};
#define VIP_EXP_BASE  ConfigEntryManager<VipExpBase>::Instance()

DEF_CONFIG_FILE(ExpConfigFile);

DEF_ENTRY(CarrierBase) {
  bool Fill(pugi::xml_node node);

  int32_t GetLevelUpExp(int32_t current_level) const;
  std::pair<int32_t, int32_t> GetSkillByGrade(int32_t grade) const {
    for (ValuePair3Vec::const_reverse_iterator iter = this->skill.rbegin();
         iter != this->skill.rend(); ++iter) {
      if (grade >= iter->v1) {
        return std::pair<int32_t, int32_t>(iter->v2, iter->v3);
      }
    }
    return std::pair<int32_t, int32_t>(0 , 0);
  }

  int8_t quality;
  int8_t country;

  int32_t hit;
  int32_t crit;

  int32_t breakadvancedid;

  ValuePair3Vec skill;
  std::vector<int32_t> slot;
  std::vector<int32_t> plane_type;

  ValuePair2Vec item;
};
#define CARRIER_BASE  ConfigEntryManager<CarrierBase>::Instance()

DEF_ENTRY(CarrierPlaneBase) {
  bool Fill(pugi::xml_node node);

  int8_t type;
  int32_t plane_lv;

  int32_t next_id;
  ValuePair2Vec cost;

  ValuePair2Vec extra_carrier;
  ValuePair2Vec extra_ship;

  static const CarrierPlaneBase* GetPlaneBase(int32_t type,
                                              int32_t plane_level);
};
#define CARRIER_PLANE_BASE ConfigEntryManager<CarrierPlaneBase>::Instance()

DEF_ENTRY(CarrierSlotBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec item;
  ValuePair2Vec money;
};
#define CARRIER_SLOT_BASE ConfigEntryManager<CarrierSlotBase>::Instance()

DEF_CONFIG_FILE(CarrierConfigFile);

struct ModifyCurrency;

DEF_ENTRY(LootBase) {
  bool Fill(pugi::xml_node node);

  void Loot(ModifyCurrency & modify, AddSubItemSet & item,
               __OUT__ std::vector<int32_t> * out) const;

  int32_t box_id;
  int32_t level;
  int32_t type;
  int32_t value;
  ValuePair3Vec info;
};
#define LOOT_BASE ConfigEntryManager<LootBase>::Instance()

class LootConfigFile : public XmlConfigFile {
 public:
  LootConfigFile(const std::string& file_name) : XmlConfigFile(file_name) {}

  static LootBasePtr Get(int32_t box_id, int32_t level);
 protected:
  virtual bool Parse();
};

//副本
DEF_ENTRY(CopyBase) {
  bool Fill(pugi::xml_node node);

  std::vector<int32_t> award;
  std::vector<int32_t> lose_award;
  std::vector<int32_t> first_award; //首胜奖励
  int32_t award_hero; //N通关必掉
  int32_t copy_box; //手动领取的宝箱

  int32_t normal_card;
  int32_t special_card;

  std::vector<LootBasePtr> AwardPtr(int32_t level) const {
    std::vector<LootBasePtr> vec;
    for (std::vector<int32_t>::const_iterator iter = award.begin();
         iter != award.end(); ++iter) {
      const LootBasePtr& ptr = LootConfigFile::Get(*iter, level);
      if (ptr) vec.push_back(ptr);
    }
    return vec;
  }

  std::vector<LootBasePtr> LoseAward(int32_t level) const {
    std::vector<LootBasePtr> vec;
    for (std::vector<int32_t>::const_iterator iter = lose_award.begin();
         iter != lose_award.end(); ++iter) {
      const LootBasePtr& ptr = LootConfigFile::Get(*iter, level);
      if (ptr) vec.push_back(ptr);
    }
    return vec;
  }

  std::vector<LootBasePtr> FirstAward(int32_t level) const {
    std::vector<LootBasePtr> vec;
    for (std::vector<int32_t>::const_iterator iter = first_award.begin();
         iter != first_award.end(); ++iter) {
      const LootBasePtr& ptr = LootConfigFile::Get(*iter, level);
      if (ptr) vec.push_back(ptr);
    }
    return vec;
  }

  LootBasePtr NormalCardPtr(int32_t level) const {
    return LootConfigFile::Get(normal_card, level);
  }

  LootBasePtr SpecialCardPtr(int32_t level) const {
    return LootConfigFile::Get(special_card, level);
  }

  int8_t type;        //类型
  std::vector<int32_t> monster; //怪物ID
  std::vector<int32_t> reinforcement; //我方援军
  int32_t power;      //消耗体力
  int32_t energy;     //消耗精力
  int32_t change_exp; //是否增加经验
  int32_t change_money;//是否增加金币

  int32_t lv;         //进入等级
  int32_t front_id;   //依赖
  int32_t enter_limit;//逻辑顺序依赖
  int32_t times_day;  //每天通关副本的次数
  int32_t times_one;  //是否只能通关一次
  int32_t star;       //是否需要评星
  int32_t forbid_day; //禁入日期
  int32_t mapped_copy_id; //副本归类的ID
  ValuePair2<int32_t, int32_t> condition; //胜利判断

  int32_t GetOrder() const;
  mutable int32_t chapter;  //计算列(章节)
  mutable int32_t order;  //计算列(逻辑顺序)
  void SetOrder(int32_t order) const { this->order = order; }
  void SetChapter(int32_t chapter) const { this->chapter = chapter; }
  bool IsTowerShowBox() const;
};
#define COPY_BASE ConfigEntryManager<CopyBase>::Instance()

//关卡
DEF_ENTRY(CopyGateBase) {
  bool Fill(pugi::xml_node node);

  std::vector<CopyBasePtr> copys; //包含的副本
  CopyBasePtr hang_ptr;           //挂机副本
};
#define COPY_GATE_BASE ConfigEntryManager<CopyGateBase>::Instance()

DEF_ENTRY(CopyChapterBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2<int32_t, int32_t> open_limit;
  int32_t last_chapter;
  int8_t type;
  int8_t box;

  std::vector<CopyGateBasePtr> levels;
  ValuePair2Vec star1;
  const CopyBasePtr& GetFirstCopy() const;
  static CopyChapterBasePtr GetFollowChapter(int32_t copy_type, int32_t chapter);
};
#define COPY_CHAPTER_BASE ConfigEntryManager<CopyChapterBase>::Instance()

DEF_CONFIG_FILE(CopyConfigFile);

DEF_ENTRY(ShipPackBase) {
  bool Fill(pugi::xml_node node);

  static int32_t Random(int32_t random, const std::vector<ValuePair2<int32_t, int32_t> >& vec);

  static int32_t GetInitialShip(int64_t uid, int32_t count);

  std::vector<ValuePair2<int32_t, int32_t> > heroid;
  int32_t sum;
};
#define SHIP_PACK_BASE ConfigEntryManager<ShipPackBase>::Instance()

//item_count材料的个数
//item_index,材料的索引[0,4)
std::pair<int32_t, int32_t> GetShipPackIdByItemCount(int32_t item_count, int32_t item_index);

DEF_ENTRY(ShipRaffleBase) {
  bool Fill(pugi::xml_node node);

  std::vector<int32_t> packs;

  std::vector<ShipPackBasePtr> packs_ptr;
};

const ShipRaffleBasePtr& GetShipRaffleByCount(int32_t count);
const ShipRaffleBasePtr& GetShipRaffleByCount(int32_t count, int32_t money);


DEF_CONFIG_FILE(BuildShipAndPlaneConfigFile);

DEF_ENTRY(TrainBase) {
  bool Fill(pugi::xml_node node);

  int32_t cost_item;
  ValuePair2<int32_t, int32_t> cost_money;
  int32_t train;
  int32_t grow;
};
#define TRAIN_BASE ConfigEntryManager<TrainBase>::Instance()

DEF_ENTRY(TrainLimitBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec hp_max;
  ValuePair2Vec attack_max;
  ValuePair2Vec wf_max;
  ValuePair2Vec ff_max;

  //固定写死四个属性
  void GetLimit(int32_t level, __OUT__ int32_t* attr_max) const;
};
#define TRAIN_LIMIT_BASE ConfigEntryManager<TrainLimitBase>::Instance()

DEF_CONFIG_FILE(TrainConfigFile);

DEF_ENTRY(ArmyBase) {
  bool Fill(pugi::xml_node node);

  static ArmyBase* GetArmyBaseByArmyItem(int32_t item_id);
  int32_t GetLevelUpExp(int32_t current_level) const;

  int8_t quality;
  int8_t type;
  int32_t exp;
  int8_t can_up;
  int8_t can_equip;

  ValuePair2Vec soldier;        //合成消耗
  ValuePair3Vec baseproperty;
  ValuePair2Vec refproperty;
  ValuePair2<int32_t, int32_t> sell;
  ValuePair2Vec gaizao_tainfu;
  ValuePair2Vec shengjin_tainfu;

  int32_t probability_player;
  int32_t probability_computer;
};
#define ARMY_BASE ConfigEntryManager<ArmyBase>::Instance()

DEF_ENTRY(ArmyExpBase) {
  bool Fill(pugi::xml_node node);

  int32_t GetExp(int32_t quality) const {
    if (quality >= sy::QUALITY_GREEN && quality <= sy::QUALITY_UNIQUE) {
      return (&green)[quality - sy::QUALITY_GREEN];
    }
    return 0;
  }

  int32_t green;
  int32_t blue;
  int32_t puple;
  int32_t orange;
  int32_t red;
  int32_t gold;
};
#define ARMY_EXP_BASE ConfigEntryManager<ArmyExpBase>::Instance()

DEF_ENTRY(ArmyRefineBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec cost;
  int32_t self_cost;
};
#define ARMY_REFINE_BASE ConfigEntryManager<ArmyRefineBase>::Instance()

DEF_CONFIG_FILE(ArmyConfigFile);

DEF_ENTRY(ShopBase) {
  bool Fill(pugi::xml_node node);

  static void RandomFeatsCommodity(
      int32_t shop_id, int32_t level,
      ::google::protobuf::RepeatedPtrField< ::sy::ShopCommodityInfo> * out,
      int32_t server_start_day);
  static bool IsRefreshShop(int32_t shop_id);

  std::pair<int32_t, int32_t> GetBuyTypeAndCount(int32_t vip) const;
  int32_t GetPrice(int32_t bought_count, int32_t buy_count) const;

  int32_t shoptype;                         //商城类型
  ValuePair2<int32_t,int32_t> item;         //商品内道具（id|数量）
  int32_t moneytype;                        //货币类型
  ValuePair2Vec price;                      //价格
  ValuePair3Vec vip;                        //购买限制
  ValuePair2<int32_t, int32_t> star;        //其他购买限制（爬塔星数，等级）
  int32_t lv_min;                           //出现等级
  int32_t lv_max;                           //出现最高等级
  int32_t weight;                           //出现概率
  int32_t lattice_position;                 //出现格子
  int32_t onsale;                           //是否上架
  ValuePair2Vec exchange;                   //兑换
  int32_t startday;                         //开服时间
};
#define SHOP_BASE ConfigEntryManager<ShopBase>::Instance()

DEF_CONFIG_FILE(ShopConfigFile);

DEF_ENTRY(FateBase) {
  bool Fill(pugi::xml_node node);

  static int RandomShipFateScale();

  int32_t cost;                                  //消耗道具
  int32_t base_value;                            //天命成功下限
  int32_t max_value;                             //天命成功上限
  ValuePair2Vec attack_value;                    //攻击属性
  ValuePair2<int32_t, int32_t> hp_value;         //生命属性
  ValuePair2<int32_t, int32_t> huopaodef_value;  //火炮防御
  ValuePair2<int32_t, int32_t> daodandef_value;  //导弹防御

};
#define FATE_BASE ConfigEntryManager<FateBase>::Instance()

DEF_CONFIG_FILE(FateConfigFile);

DEF_ENTRY(PatrolBase) {
  bool Fill(pugi::xml_node node);

  static int32_t GeneratePatrolAward(
      int32_t award_count, int32_t player_level, int32_t patrol_level,
      PatrolBase * base, VectorMap<int32_t, int32_t> & awards_temp);

  int32_t GetSkillAward(int32_t key) {
    for (ValuePair2Vec::const_iterator it = skill_award.begin();
         it != skill_award.end(); ++it) {
      if (it->v1 == key) return it->v2;
    }
    return 0;
  }

  int32_t GetSkillTime(int32_t key) {
    for (ValuePair2Vec::const_iterator it = skill_time.begin();
         it != skill_time.end(); ++it) {
      if (it->v1 == key) return it->v2;
    }
    return 0;
  }

  int32_t GetSkillCost(int32_t key) {
    for (ValuePair2Vec::const_iterator it = skill_cost.begin();
         it != skill_cost.end(); ++it) {
      if (it->v1 == key) return it->v2;
    }
    return 0;
  }

  int32_t ship_award;                              //战舰碎片实际掉落
  int32_t item_award;                              //道具资源实际掉落
  int32_t mapped_id;                               //副本映射
  ValuePair2Vec skill_award;  //领地技能等级|道具翻倍概率
  ValuePair2Vec skill_time;  //领地技能等级|领地技能升级条件（小时）
  ValuePair2Vec skill_cost;  //领地技能等级|升级消耗（元宝）
};

#define PATROL_BASE ConfigEntryManager<PatrolBase>::Instance()

DEF_CONFIG_FILE(PatrolConfigFile);

struct ArenaRewardBase {
  bool Fill(pugi::xml_node node);

  int32_t id;
  ValuePair2Vec daily_reward;
};

struct ArenaGoldRewardBase {
  bool Fill(pugi::xml_node node);

  int32_t id;
  int32_t gold;
};

struct ArenaRankBase {
  bool Fill(pugi::xml_node node);

  int32_t id;
  ValuePair2<int32_t, int32_t> difference;
};

typedef VectorMap<int32_t, ArenaRewardBase>::const_iterator ArenaRewardIter;
typedef VectorMap<int32_t, ArenaGoldRewardBase>::const_iterator ArenaGoldRewardIter;

class ArenaConfigFile : public XmlConfigFile {
 public:
  ArenaConfigFile(const std::string& file_name) : XmlConfigFile(file_name) {}

  static ArenaRewardIter GetDailyAwardByRank(int32_t rank);
  static ArenaRewardIter GetDailyAwardEnd();
  static ArenaGoldRewardIter GetGoldRewardByRank(int32_t rank);
  static ArenaGoldRewardIter GetGoldRewardEnd();

  static int32_t GetArenaRankOffset(int32_t rank);

 protected:
  virtual bool Parse();
};

DEF_ENTRY(BuyCountCostBase) {
  bool Fill(pugi::xml_node node);

  int32_t GetBuyCount(int32_t vip_level) const {
    for (ValuePair2Vec::const_reverse_iterator iter = times.rbegin();
         iter != times.rend(); ++iter) {
      if (vip_level >= iter->v1) return iter->v2;
    }
    return 0;
  }

  int32_t GetBuyCost(int32_t count) const {
    for (ValuePair2Vec::const_reverse_iterator iter = cost.rbegin();
         iter != cost.rend(); ++iter) {
      if (count >= iter->v1) return iter->v2;
    }
    return 0;
  }

  int32_t type;
  ValuePair2Vec times;
  ValuePair2Vec cost;
};

#define BUY_COUNT_BASE  ConfigEntryManager<BuyCountCostBase>::Instance()

DEF_ENTRY(VipFunctionBase) {
  bool Fill(pugi::xml_node node);
  ValuePair2Vec viplv;
  ValuePair2<int32_t, int32_t> lv;
  int32_t GetValue(int32_t vip, int32_t level) const;
};

#define VIP_FUNCTION_BASE ConfigEntryManager<VipFunctionBase>::Instance()


DEF_CONFIG_FILE(BuyCountConfigFile);

void LootByRandomCommon(const ValuePair2Vec& container, int32_t random_count, std::vector<int32_t>& out);

DEF_ENTRY(TowerBuffBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2<int32_t, int32_t> effect_type;
  int32_t star_num;
};

#define TOWER_BUFF_BASE ConfigEntryManager<TowerBuffBase>::Instance()

class TowerConfigFile: public XmlConfigFile {
 public:
  TowerConfigFile(const std::string& file_name) : XmlConfigFile(file_name) {}

  static void GetTowerLootAward(AddSubItemSet& item_set, ModifyCurrency& modify,
                                ValuePair2Vec& award, int32_t money,
                                int32_t level);
  static void RandomBuff(int32_t& b3, int32_t& b6, int32_t& b9);
 protected:
  virtual bool Parse();
};

DEF_CONFIG_FILE(RecoverConfigFile);

DEF_ENTRY(RecoverBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec type;
};

#define RECOVER_BASE ConfigEntryManager<RecoverBase>::Instance()

struct DstrikeBossBase;
struct DstrikeRankAwardBase;
struct DstrikeMeritAwardBase;

class DstrikeConfigFile : public XmlConfigFile {
 public:
  DstrikeConfigFile(const std::string& file_name) : XmlConfigFile(file_name) {}
  static const DstrikeBossBase* RandomBoss(int32_t level, int32_t type);
  static const DstrikeBossBase* GetBossByID(int64_t boss_id);
  static const DstrikeRankAwardBase* GetRewardByRank(int32_t rank);
  static const DstrikeMeritAwardBase* GetDailyAward(int32_t level, int32_t index);

 protected:
  virtual bool Parse();
};

DEF_ENTRY(DstrikeTriggerBase) {
  bool Fill(pugi::xml_node node);
  int32_t lv;
  ValuePair2Vec main;
  ValuePair2Vec elite;
};

#define DSTRIKE_TRIGGER_BASE ConfigEntryManager<DstrikeTriggerBase>::Instance()

DEF_ENTRY(DstrikeMeritAwardBase) {
  bool Fill(pugi::xml_node node);
  int32_t lv;
  int32_t index;
  int32_t merit_num;
  ValuePair2<int32_t,int32_t> merit_award;
};
#define DSTRIKE_BOSS_DAILY_AWARD_BASE ConfigEntryManager<DstrikeMeritAwardBase>::Instance()

typedef google::protobuf::RepeatedField<google::protobuf::int64> AttrVec;

int64_t CalcFightScore(const AttrVec& vec, bool ignore_hit);

DEF_ENTRY(MonsterGroupBase) {
  bool Fill(pugi::xml_node node);
  int64_t GetAttr(const ValuePair2Vec& values, int32_t attr_index) const;

  std::vector<int32_t> monster_group;
  std::vector<int32_t> skill_lv;
  std::vector<int32_t> monster_lv;
  std::vector<int32_t> quality;

  const AttrVec& GetMonsterAttr(int32_t pos) const {
    switch (pos) {
      case 1: return monster1_attr;
      case 2: return monster2_attr;
      case 3: return monster3_attr;
      case 4: return monster4_attr;
      case 5: return monster5_attr;
      case 6: return monster6_attr;
    }
    static AttrVec empty;
    empty.Resize(sy::AttackAttr_ARRAYSIZE, 0);
    return empty;
  }

  sy::CurrentCarrierInfo& carrier_info() const {
    if (this->carrier.carrier_id()) {
      return this->carrier;
    }
    carrier.set_carrier_id(monster_group[0]);
    carrier.mutable_attr1()->CopyFrom(carrier_attr);
    carrier.set_quality(quality[0]);
    return carrier;
  }

  std::vector<sy::HeroInfo>& hero_info() const {
    if (this->heros.empty()) {
      for (int32_t pos = 1; pos <= 6; ++pos) {
        sy::HeroInfo info;
        info.set_uid(0);
        info.set_level(monster_lv[pos]);
        info.set_fate_level(skill_lv[pos]);
        info.set_exp(0);
        info.mutable_attr1()->CopyFrom(GetMonsterAttr(pos));
        info.set_hero_id(monster_group[pos]);
        info.set_quality(quality[pos]);
        this->heros.push_back(info);
      }
    }
    return this->heros;
  }

  AttrVec carrier_attr;
  AttrVec monster1_attr;
  AttrVec monster2_attr;
  AttrVec monster3_attr;
  AttrVec monster4_attr;
  AttrVec monster5_attr;
  AttrVec monster6_attr;

  private:
   mutable sy::CurrentCarrierInfo carrier;
   mutable std::vector<sy::HeroInfo> heros;
};

#define MONSTER_GROUP_BASE ConfigEntryManager<MonsterGroupBase>::Instance()

DEF_ENTRY(MonsterBase){
  bool Fill(pugi::xml_node node);

  int32_t job;
  int32_t country;
  ValuePair2<int32_t, int32_t> skill;
  std::vector<int32_t> combo;
  int32_t start_buff;
};

#define MONSTER_BASE ConfigEntryManager<MonsterBase>::Instance()

const MonsterGroupBasePtr& GetRobotConfigByID(int32_t robot_id);

DEF_CONFIG_FILE(MonsterConfigFile);

DEF_ENTRY(ArmyBossBase) {
  bool Fill(pugi::xml_node node);

  int32_t chapter;
  int32_t monster;
  int32_t country;
  int32_t reward_kill;        //最后一击奖励,增加军团经验
  int32_t reward_kill2;       //最后一击,给个人增加军团贡献
  ValuePair2<int32_t, int32_t> reward_challenge;//每次挑战奖励,军团贡献
  ValuePair3Vec reward_choose;//抽取的奖励
  ValuePair2Vec reward_once;  //副本通关奖励(章节ID*10+1才有,其他都是空的)
  MonsterGroupBasePtr monster_base;
};

#define ARMY_BOSS_BASE ConfigEntryManager<ArmyBossBase>::Instance()

DEF_ENTRY(DstrikeRankAwardBase) {
  bool Fill(pugi::xml_node node);
  int32_t range;
  ValuePair2<int32_t,int32_t> damage_rankaward;
  ValuePair2<int32_t,int32_t> merit_rankaward;
};

//id = lv * 100 + quality
DEF_ENTRY(DstrikeBossBase) {
  bool Fill(pugi::xml_node node);
  int32_t lv;
  int32_t quality;
  MonsterGroupBasePtr monster;
  ValuePair2Vec grow_monster_value;

  void FillMonsterInfos(sy::CurrentCarrierInfo & carrier,
                        std::vector<sy::HeroInfo> & heros, int32_t level) const;

  int32_t GetAttrGrow(int32_t attr_index) const;
  void FillBlood(
      google::protobuf::RepeatedField< ::google::protobuf::int64> & blood,
      int32_t level) const;
};

#define DSTRIKE_BOSS_BASE ConfigEntryManager<DstrikeBossBase>::Instance()

DEF_ENTRY(SignBase) {
  bool Fill(pugi::xml_node node);

  static int32_t max_id;

  ValuePair2Vec award;
  int32_t vipaward_lv;
};
#define SIGN_BASE ConfigEntryManager<SignBase>::Instance()

DEF_CONFIG_FILE(SignConfigFile);

DEF_ENTRY(BuffBase) {
  bool Fill(pugi::xml_node node);
  int32_t guorp_id;
  int32_t bufftimes;
  int32_t target_type;
  int32_t target;
  int32_t buff_value_type;
  int32_t buff_type;
  int32_t last;
  int32_t pro;  //单位是百万分之一
  ValuePair2<int32_t, int32_t> pro_type1;
  ValuePair2<int32_t, int32_t> pro_type2;
  ValuePair2<int32_t, int32_t> pro_type3;
};
#define BUFF_BASE ConfigEntryManager<BuffBase>::Instance()

DEF_ENTRY(SkillBase) {
  bool Fill(pugi::xml_node node);
  int32_t cd;
  int32_t times;
  int32_t fury;
  int32_t fight_type1;
  int32_t target_type;
  int32_t target;
  ValuePair2Vec carrier_attack_type;
  ValuePair2<int32_t, int32_t> value;
  std::vector<BuffBasePtr> buff_ptr;

  ValuePair2Vec skill_damage;

  std::pair<int32_t, int32_t> GetSkillDamage(int32_t fate_level) const;
};

#define SKILL_BASE ConfigEntryManager<SkillBase>::Instance()

DEF_CONFIG_FILE(SkillConfigFile);

DEF_ENTRY(ActivityBase) {
  bool Fill(pugi::xml_node node);

  int32_t time;
  ValuePair2Vec reward;
  int32_t points;
};
#define ACTIVITY_BASE ConfigEntryManager<ActivityBase>::Instance()

DEF_ENTRY(ActivityRewardBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec reward;
};
#define ACTIVITY_REWARD_BASE ConfigEntryManager<ActivityRewardBase>::Instance()

DEF_ENTRY(AchievementBase) {
  bool Fill(pugi::xml_node node);

  int32_t type;
  int32_t time;
  ValuePair2Vec reward;
};
#define ACHIEVEMENT_BASE ConfigEntryManager<AchievementBase>::Instance()

DEF_CONFIG_FILE(ActivityConfigFile);

DEF_ENTRY(RankBase) {
  bool Fill(pugi::xml_node node);

  int32_t rank;
  ValuePair2Vec add;
  ValuePair2Vec reward;
  int32_t cost;
  int32_t carrier_box;
};
#define RANK_BASE ConfigEntryManager<RankBase>::Instance()

DEF_CONFIG_FILE(RankConfigFile);

DEF_ENTRY(SevenDaysBase) {
  bool Fill(pugi::xml_node node);

  int32_t day;
  int32_t index;
  int32_t type;
  int32_t condition;
  std::vector<int32_t> reward;
};
#define SEVEN_DAYS_BASE ConfigEntryManager<SevenDaysBase>::Instance()

DEF_ENTRY(SevenDays14Base) {
  bool Fill(pugi::xml_node node);

  int32_t day;
  int32_t index;
  int32_t type;
  int32_t condition;
  std::vector<int32_t> reward;
};
#define SEVEN_DAYS_14_BASE ConfigEntryManager<SevenDays14Base>::Instance()

DEF_CONFIG_FILE(SevenDaysConfigFile);

DEF_ENTRY(SupportBase) {
  bool Fill(pugi::xml_node node);

  static const SupportBasePtr GetSupportBase(int32_t level);

  int32_t need_lv;
  ValuePair2Vec property;
};
#define SUPPORT_BASE ConfigEntryManager<SupportBase>::Instance()

DEF_CONFIG_FILE(SupportConfigFile);

//layer * 10 + level
DEF_ENTRY(CarrierCopyBase){
  bool Fill(pugi::xml_node node);

  //随机战斗力
  int32_t fight_attr(int32_t player_fight) const;
  //随机等级
  int32_t level(int32_t player_level) const;

  int32_t fc_mean;
  int32_t fc_stddev;
  int32_t fc_min;
  int32_t fc_max;
  int32_t lvl_base;
  int32_t lvl_change;

  int32_t reward_a;
  int32_t reward_b;
  int32_t reward_fixed;
};
#define CARRIER_COPY_BASE ConfigEntryManager<CarrierCopyBase>::Instance()

DEF_ENTRY(CarrierCopyAwardBase){
  bool Fill(pugi::xml_node node);

  std::vector<int32_t> reward;
};
#define CARRIER_COPY_AWARD_BASE ConfigEntryManager<CarrierCopyAwardBase>::Instance()

DEF_CONFIG_FILE(CarrierCopyFile);

DEF_ENTRY(ChartBase) {
  bool Fill(pugi::xml_node node);

  std::vector<int32_t> need;
  ValuePair2Vec property;

  static std::vector<ChartBase*> charts;
};
#define CHART_BASE ConfigEntryManager<ChartBase>::Instance()

DEF_CONFIG_FILE(ChartConfigFile);

DEF_ENTRY(FundBase) {
  bool Fill(pugi::xml_node node);

  int32_t level;
  int32_t return1;
};
#define FUND_BASE ConfigEntryManager<FundBase>::Instance()

DEF_ENTRY(WelfareBase) {
  bool Fill(pugi::xml_node node);

  int32_t count;
  ValuePair2<int32_t,int32_t> award;
};
#define WELFARE_BASE ConfigEntryManager<WelfareBase>::Instance()

DEF_ENTRY(DailyAwardBase) {
  bool Fill(pugi::xml_node node);

  int32_t onsale;
  ValuePair2Vec award;
  ValuePair2<int32_t, int32_t> condition;
  std::vector<int32_t> drop_box;
};
#define DAILY_AWARD_BASE ConfigEntryManager<DailyAwardBase>::Instance()

DEF_ENTRY(VIPDailyAwardBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec award;
};
#define VIP_DAILY_AWARD_BASE ConfigEntryManager<VIPDailyAwardBase>::Instance()

DEF_ENTRY(VIPWeeklyShopBase) {
  bool Fill(pugi::xml_node node);

  int32_t type_num;
  int32_t vip;
  int32_t lv;
  ValuePair2Vec award;
  int32_t moneytype;
  int32_t price;
  int32_t available_times;
};
#define VIP_WEEKLY_SHOP_BASE ConfigEntryManager<VIPWeeklyShopBase>::Instance()

DEF_ENTRY(LimitedRecruitRankBase) {
  bool Fill(pugi::xml_node node);

  static const ValuePair2Vec* GetAwardByRank(int32_t rank);

  int32_t range1;
  int32_t range2;
  ValuePair2Vec award;
};
#define LIMITED_RECRUIT_RANK_BASE ConfigEntryManager<LimitedRecruitRankBase>::Instance()

DEF_ENTRY(LimitedRecruitBoxBase) {
  bool Fill(pugi::xml_node node);

  int32_t score;
  ValuePair2Vec award;
};
#define LIMITED_RECRUIT_BOX_BASE ConfigEntryManager<LimitedRecruitBoxBase>::Instance()

DEF_ENTRY(DiamondFundBase) {
  bool Fill(pugi::xml_node node);

  int32_t buy_count;
  int32_t type;
  int32_t day;
  ValuePair2<int32_t, int32_t> award;
};
#define DIAMOND_FUND_BASE ConfigEntryManager<DiamondFundBase>::Instance()

DEF_ENTRY(DailyRechargeItemBase) {
  bool Fill(pugi::xml_node node);

  std::string goods_id;
  int32_t drop_box;
};

const DailyRechargeItemBase* GetDailyRechargeItemByGoodID(const std::string& goodid);

DEF_CONFIG_FILE(ServerOpenConfigFile);

DEF_ENTRY(LeagueSkillBase) {
  bool Fill(pugi::xml_node node);

  int32_t cost_type;
  int32_t skill_max;
  int32_t need_league;
  ValuePair2Vec attr;
};
#define LEAGUE_SKILL_BASE ConfigEntryManager<LeagueSkillBase>::Instance()

DEF_ENTRY(LeagueSkillCostBase) {
  bool Fill(pugi::xml_node node);

  int32_t cost_type1;
  int32_t cost_type2;
  int32_t cost_type3;
  int32_t cost_type4;
};
#define LEAGUE_SKILL_COST_BASE ConfigEntryManager<LeagueSkillCostBase>::Instance()

DEF_ENTRY(LeagueSignBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2<int32_t, int32_t> cost;
  int32_t sign_progress;
  int32_t league_exp;
  int32_t league_currency;
  int32_t vip;
};
#define LEAGUE_SIGN_BASE ConfigEntryManager<LeagueSignBase>::Instance()

DEF_ENTRY(LeagueBase) {
  bool Fill(pugi::xml_node node);

  int32_t exp;
  int32_t limit;
};
#define LEAGUE_BASE ConfigEntryManager<LeagueBase>::Instance()

DEF_ENTRY(LeagueLevelBase) {
  bool Fill(pugi::xml_node node);

  int32_t exp;
  int32_t step;
  int32_t need_lvl;
};
#define LEAGUE_LEVEL_BASE ConfigEntryManager<LeagueLevelBase>::Instance()

DEF_ENTRY(LeagueSignRewardBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec reward;
};
#define LEAGUE_SIGN_REWARD_BASE ConfigEntryManager<LeagueSignRewardBase>::Instance()

DEF_CONFIG_FILE(LeagueConfigFile);

DEF_ENTRY(SevenDayRaceBase){
  bool Fill(pugi::xml_node node);

  ValuePair2Vec reward;
};

SevenDayRaceBase* GetSevenDaysRace(int32_t rank);

DEF_ENTRY(RechargeBase) {
  bool Fill(pugi::xml_node node);
  int32_t price;
  int32_t extra_gold;
  int32_t index;
  std::string goods_id;
};
#define RECHARGE_BASE ConfigEntryManager<RechargeBase>::Instance()

RechargeBasePtr GetRechargeByGoodsID(const std::string& goods_id);

const std::string& GetMappedGoodID(const std::string& goodid);

std::pair<std::string, int32_t> GetGoodIDByItemID(int32_t item_id);

DEF_ENTRY(WorldBossAttrBase) {
  bool Fill(pugi::xml_node node);

  static void FillMonsterInfos(sy::CurrentCarrierInfo & carrier,
                               std::vector<sy::HeroInfo> & heros, int32_t level,
                               int32_t monster_group);

  static void Clear();

  ValuePair2VecInt64 monster_group;
  ValuePair2VecInt64 grow_monster_value;
};
#define WORLD_BOSS_ATTR_BASE ConfigEntryManager<WorldBossAttrBase>::Instance()

DEF_ENTRY(WorldBossMeritAwardBase) {
  bool Fill(pugi::xml_node node);

  int32_t lv;
  int32_t index;
  int32_t merit_num;
  ValuePair2<int32_t, int32_t> merit_award;
};
#define WORLD_BOSS_MERTIA_AWARD_BASE ConfigEntryManager<WorldBossMeritAwardBase>::Instance()

DEF_ENTRY(WorldBossKillAwardBase) {
  bool Fill(pugi::xml_node node);

  const WorldBossKillAwardBase* GetKillAward(int32_t level);

  ValuePair2<int32_t, int32_t> merit_award;
};
#define WORLD_BOSS_KILL_AWARD_BASE ConfigEntryManager<WorldBossKillAwardBase>::Instance()

DEF_ENTRY(WorldBossRankAwardBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec merit_rankaward;
  ValuePair2Vec damage_rankaward;
};
#define WORLD_BOSS_RANK_AWARD_BASE ConfigEntryManager<WorldBossRankAwardBase>::Instance()

DEF_ENTRY(WorldBossLeagueRankAwardBase) {
  bool Fill(pugi::xml_node node);

  int32_t merit_rankaward;
};
#define WORLD_BOSS_LEAGUE_RANK_AWARD_BASE ConfigEntryManager<WorldBossLeagueRankAwardBase>::Instance()

DEF_CONFIG_FILE(WorldBossConfigFile);

DEF_ENTRY(WakeItemBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec attr;
  ValuePair2Vec cost;
  int32_t cost_money;
};
#define WAKE_ITEM_BASE ConfigEntryManager<WakeItemBase>::Instance()

DEF_ENTRY(WakeTalentBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec attr;
};
#define WAKE_TALENT_BASE ConfigEntryManager<WakeTalentBase>::Instance()

DEF_ENTRY(WakeConditionBase) {
  bool Fill(pugi::xml_node node);

  int32_t need_lv;
  int32_t need_hero;
  int32_t need_item;
  int64_t need_coin;
  std::vector<int32_t> itemtree1;
  std::vector<int32_t> itemtree2;
  std::vector<int32_t> itemtree3;
  std::vector<int32_t> itemtree4;
  std::vector<int32_t> itemtree5;
  std::vector<int32_t> itemtree6;
  std::vector<int32_t> itemtree7;
  std::vector<int32_t> itemtree8;
  std::vector<int32_t> itemtree9;
  std::vector<int32_t> itemtree10;
  std::vector<int32_t> itemtree11;
};
#define WAKE_CONDITION_BASE ConfigEntryManager<WakeConditionBase>::Instance()

DEF_CONFIG_FILE(WakeConfigFile);

DEF_ENTRY(LegionCityBase){
  bool Fill(pugi::xml_node node);

  std::vector<int32_t> nearby;
  int32_t quality;
};
#define LEGION_CITY_BASE ConfigEntryManager<LegionCityBase>::Instance()

//问鼎积分
const std::vector<int32_t>& GetLegionPersonalAward(int32_t quality);
//占领分
const std::vector<int32_t>& GetLegionArmyScore(int32_t quality);

DEF_ENTRY(LegionScoreBase){
  bool Fill(pugi::xml_node node);
};

DEF_ENTRY(LegionAwardBase){
  bool Fill(pugi::xml_node node);

  int32_t money;      //威望
  int32_t rank;       //问鼎积分
};
#define LEGION_AWARD_BASE ConfigEntryManager<LegionAwardBase>::Instance()

//全球制霸悬赏
DEF_ENTRY(LegionPlayerRewardBase){
  bool Fill(pugi::xml_node node);

  std::vector<std::pair<int32_t, int32_t> > award;
};
#define LEGION_PLAYER_REWARD_BASE ConfigEntryManager<LegionPlayerRewardBase>::Instance()

DEF_ENTRY(LegionArmyRewardBase){
  bool Fill(pugi::xml_node node);

  std::vector<std::pair<int32_t, int32_t> > award;
};
#define LEGION_ARMY_REWARD_BASE ConfigEntryManager<LegionArmyRewardBase>::Instance()

//全球制霸每日任务
DEF_ENTRY(LegionDailyRewardBase){
  bool Fill(pugi::xml_node node);
  int32_t times;
  int32_t money;
};
#define LEGION_DAILY_REWARD_BASE ConfigEntryManager<LegionDailyRewardBase>::Instance()

DEF_ENTRY(LegionWarTargetBase){
  bool Fill(pugi::xml_node node);
  int32_t lvl_base;
  int32_t lvl_change;
  int32_t reward_a;
  int32_t reward_b;
  int32_t reward_c;
  int32_t reward_d;
};

LegionWarTargetBasePtr GetLegionWarTargetBaseByLevel(int32_t level);

DEF_ENTRY(LegionForeplayCopyBase) {
  bool Fill(pugi::xml_node node);

  int32_t monster;
  ValuePair2<int32_t, int32_t> merit_award;
  ValuePair2Vec server_award;
};
#define LEGION_FOREPLAY_COPY_BASE ConfigEntryManager<LegionForeplayCopyBase>::Instance()

DEF_ENTRY(LegionForeplayAwardBase) {
  bool Fill(pugi::xml_node node);

  int32_t merit_num;
  ValuePair2Vec merit_award;
};
#define LEGION_FOREPLAY_AWARD_BASE ConfigEntryManager<LegionForeplayAwardBase>::Instance()

DEF_ENTRY(LegionForeplayAward1Base) {
  bool Fill(pugi::xml_node node);

  static int32_t GetAward(int32_t day);

  int32_t money;
};
#define LEGION_FOREPLAY_AWARD1_BASE ConfigEntryManager<LegionForeplayAward1Base>::Instance()

DEF_ENTRY(LegionForeplayRankBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec damage_rankaward;
};
#define LEGION_FOREPLAY_RANK_BASE ConfigEntryManager<LegionForeplayRankBase>::Instance()

DEF_CONFIG_FILE(LegionConfigFile);

bool AddWakeAttr(int32_t level, int32_t type,
                 Array<int64_t, sy::AttackAttr_ARRAYSIZE>& attr);

const VectorMap<int32_t, int32_t>* GetUsedWakeItem(int32_t level, int32_t type);

const std::vector<int32_t>* GetWakeItemTree(int32_t type,
                                            WakeConditionBase* wake_condition);

DEF_ENTRY(EliteCopyRefreshBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2<int32_t, int32_t> GetHourLimit(int32_t hour) const {
    if (hour == 6) return hour6;
    if (hour == 12) return hour12;
    if (hour == 18) return hour18;
    return ValuePair2<int32_t, int32_t>();
  }

  //v1是本次随机的BOSS
  //v2是累计随机的BOSS
  ValuePair2<int32_t, int32_t> hour6;
  ValuePair2<int32_t, int32_t> hour12;
  ValuePair2<int32_t, int32_t> hour18;
};

const EliteCopyRefreshBase* GetEliteCopyRefreshByLevel(int32_t level);

DEF_ENTRY(EliteCopyBase) {
  bool Fill(pugi::xml_node node);

  int32_t power;
  int32_t item1;
  int32_t item2;
  int32_t monster;
};
#define ELITEC_COPY_BASE ConfigEntryManager<EliteCopyBase>::Instance()

DEF_CONFIG_FILE(EliteCopyConfigFile);

DEF_ENTRY(RedGoldSkillBase) {
  bool Fill(pugi::xml_node node);

  static void AddRedGoldAttr(int32_t equip_id, int32_t refine_level,
                             int32_t gold_level,
                             Array<int64_t, sy::AttackAttr_ARRAYSIZE> & attr);

  ValuePair2Vec attr;
  int32_t equip_id;
  int32_t grade_level;
  int32_t gold_level;
};
#define RED_GOLD_SKILL_BASE ConfigEntryManager<RedGoldSkillBase>::Instance()

DEF_CONFIG_FILE(RedGoldConfigFile);

DEF_ENTRY(CrossServerPK1WinAwardBase) {
  bool Fill(pugi::xml_node node);

  int32_t win;
  int32_t award;
};
#define CROSS_SERVER_PK1_WIN_AWARD_BASE ConfigEntryManager<CrossServerPK1WinAwardBase>::Instance()

DEF_ENTRY(CrossServerPK1RankAwardBase) {
  bool Fill(pugi::xml_node node);

  static int32_t GetAwardBaseByRank(int32_t rank);

  int32_t range1;
  int32_t range2;
  int32_t rankaward;
};
#define CROSS_SERVER_PK1_RANK_AWARD_BASE ConfigEntryManager<CrossServerPK1RankAwardBase>::Instance()

DEF_ENTRY(CrossServerPK1RollPlayerBase) {
  bool Fill(pugi::xml_node node);

  int32_t fc_mean;
  int32_t fc_stddev;
  int32_t fc_min;
  int32_t fc_max;
  int32_t lvl_base;
  int32_t lvl_change;
  int32_t reward_a;
  int32_t reward_b;
};
#define CROSS_SERVER_PK1_ROLL_PLAYER_BASE ConfigEntryManager<CrossServerPK1RollPlayerBase>::Instance()

DEF_CONFIG_FILE(CrossServerConfigFile);

DEF_ENTRY(ActivityInfoBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec time;
  ValuePair2<int32_t, int32_t> IsTodayInActivty(int32_t delta_days) const;
  ValuePair2<int32_t, int32_t> cost1;
  ValuePair2<int32_t, int32_t> cost10;
  ValuePair2<int32_t, int32_t> item;
};
#define ACTIVITY_INFO_BASE ConfigEntryManager<ActivityInfoBase>::Instance()

DEF_ENTRY(ActivityEquipBase) {
  bool Fill(pugi::xml_node node);

  int32_t score;
  ValuePair2<int32_t, int32_t> award;
};
#define ACTIVITY_EQUIP_BASE ConfigEntryManager<ActivityEquipBase>::Instance()

DEF_ENTRY(ActivityCountBase) {
  bool Fill(pugi::xml_node node);

  int32_t activity_type;
  int32_t count;
  ValuePair2<int32_t, int32_t> award;
};
#define ACTIVITY_COUNT_BASE ConfigEntryManager<ActivityCountBase>::Instance()

DEF_ENTRY(ActivityCarrierBase) {
  bool Fill(pugi::xml_node node);

  std::vector<std::pair<int32_t, int32_t> > reward1;
  std::vector<std::pair<int32_t, int32_t> > reward2;
};

const ActivityCarrierBase* GetSweepStakeCarrier(int32_t rank);

bool IsSweepStakeCountLucky(int64_t player_id, int32_t count);

int32_t GetActivityNextCount(int32_t type, int32_t current_count);

DEF_CONFIG_FILE(SweepStakeConfigFile);

DEF_CONFIG_FILE(UpdateVersionConfigFile);

bool CanGetUpdateVersionAward(int32_t package_id,
                              const std::string& cur_version,
                              const std::string& got_version);

DEF_ENTRY(MedalOpenBase) {
  bool Fill(pugi::xml_node node);

  int32_t level;
  int32_t open_limit;
};
#define MEDAL_OPEN_BASE ConfigEntryManager<MedalOpenBase>::Instance()

DEF_ENTRY(MedalChartBase) {
  bool Fill(pugi::xml_node node);

  int32_t chapter;
  std::vector<int32_t> cost;
};
#define MEDAL_CHART_BASE ConfigEntryManager<MedalChartBase>::Instance()

DEF_ENTRY(MedalChart2Base) {
  bool Fill(pugi::xml_node node);

  int32_t star;
  ValuePair2Vec attr;
};
#define MEDAL_CHART2_BASE ConfigEntryManager<MedalChart2Base>::Instance()

DEF_ENTRY(MedalFormationBase) {
  bool Fill(pugi::xml_node node);

  static const MedalFormationBase* GetAttrByStar(int32_t star);

  ValuePair2Vec attr;
};
#define MEDAL_FORMATION_BASE ConfigEntryManager<MedalFormationBase>::Instance()

DEF_ENTRY(MedalCopyBase) {
  bool Fill(pugi::xml_node node);

  int32_t quality;
  int32_t monster;
  ValuePair2Vec reward;
};
#define MEDAL_COPY_BASE ConfigEntryManager<MedalCopyBase>::Instance()

DEF_CONFIG_FILE(MedalConfigFile);

DEF_ENTRY(RedequipRisestarBase) {
  bool Fill(pugi::xml_node node);

  int32_t equip_id;
  int32_t star;
  int32_t attr_type;
  int32_t attr_max;
  int32_t attr1;
};
#define REDEQUIP_RISESTAR_BASE ConfigEntryManager<RedequipRisestarBase>::Instance()

DEF_ENTRY(RedequipCostBase) {
  bool Fill(pugi::xml_node node);

  int32_t exp;
  int32_t exp1;
  int32_t exp2;
  int32_t exp3;
  ValuePair2Vec cost1;
  ValuePair2Vec cost2;
  ValuePair2Vec cost3;
  int32_t rate1;
  int32_t rate2;
};
#define REDEQUIP_COST_BASE ConfigEntryManager<RedequipCostBase>::Instance()

DEF_CONFIG_FILE(RedEquipConfigFile);

bool IsInLegionWarOpenDays();

DEF_ENTRY(HeroreturnBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec award;
};
#define HERORETURN_BASE ConfigEntryManager<HeroreturnBase>::Instance()

DEF_ENTRY(HeroreturnRechargeBase) {
  bool Fill(pugi::xml_node node);

  int32_t type;
  int32_t condition;
  ValuePair2Vec award;
};
#define HERORETURN_RECHARGE_BASE ConfigEntryManager<HeroreturnRechargeBase>::Instance()

DEF_CONFIG_FILE(HeroComeBackConfigFile);

DEF_ENTRY(PearlharborCopyBase) {
  bool Fill(pugi::xml_node node);

  int32_t chapter;
  ValuePair2<int32_t, int32_t> reward_challenge;
  ValuePair2Vec reward_kill2;
  int32_t monster_num;
  std::vector<int32_t> monster_image;

  std::vector<int32_t> RandomGroupMonster();
};
#define PEARLHARBOR_COPY_BASE ConfigEntryManager<PearlharborCopyBase>::Instance()

DEF_ENTRY(PearlharborBreakRewardBase) {
  bool Fill(pugi::xml_node node);

  int32_t chapter;
  int32_t quality;
  ValuePair2Vec reward;
};
#define PEARLHARBOR_BREAK_REWARD_BASE ConfigEntryManager<PearlharborBreakRewardBase>::Instance()

DEF_ENTRY(PearlharborRankPersonalawardBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec award;
};
#define PEARLHARBOR_RANK_PERSONALAWARD_BASE ConfigEntryManager<PearlharborRankPersonalawardBase>::Instance()

DEF_ENTRY(PearlharborRankLeagueawardBase) {
  bool Fill(pugi::xml_node node);

  ValuePair2Vec award;
};
#define PEARLHARBOR_RANK_LEAGUEAWARD_BASE ConfigEntryManager<PearlharborRankLeagueawardBase>::Instance()

DEF_CONFIG_FILE(PearlHarborConfigFile);

struct AcitivityContentBase {
  AcitivityContentBase() : activity_id(0), activity_type(0), duration(0) {}

  int32_t activity_id;
  int32_t activity_type;
  int32_t duration;
  std::string field;
  std::string content;
  std::string desc;

  std::vector<std::string> raw_field;
  std::vector<std::vector<std::string> > raw_str;
};

const AcitivityContentBase* GetActivityByID(int32_t id);

DEF_ENTRY(WeeklyActivityBase) {
  bool Fill(pugi::xml_node node);

  int32_t count;
  int32_t weekday;
  int32_t duration;
  int32_t content_id;
};

#define WEEKLY_ACTIVITY_BASE ConfigEntryManager<WeeklyActivityBase>::Instance()

DEF_ENTRY(ServerStartActivityBase) {
  bool Fill(pugi::xml_node node);

  int32_t count;
  int32_t duration;
  int32_t content_id;
};

struct ActivityInfoConfig {
  ActivityInfoConfig()
      : activity_type(0), activity_id(0), begin_time(0), end_time(0) {}
  int32_t activity_type;
  int32_t activity_id;
  int32_t begin_time;
  int32_t end_time;
  std::string desc;

  std::vector<std::vector<std::string> > raw_content;
  std::vector<std::string> raw_field;
};

std::vector<ActivityInfoConfig> GetWeeklyActivityByWeeks(int32_t week_index);
std::vector<ActivityInfoConfig> GetServerStartActivityByDay(int32_t start_days);

DEF_CONFIG_FILE(FileActivityConfigFile);
