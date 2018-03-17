#pragma once
#include <cpp/message.pb.h>
#include <common_define.h>
#include <array.h>

struct ItemParam {
  ItemParam() : item_id(0), item_count(0) {}
  ItemParam(int32_t id, int32_t count) : item_id(id), item_count(count) {}
  int32_t item_id;
  int32_t item_count; //大于0表示增加; 小于0表示减少
};

#define ARENA_LIST_COUNT 50

#define ITEM_COUNT_ONE_TIME 256
typedef Array<ItemParam, ITEM_COUNT_ONE_TIME> AddSubItemSet;
typedef Array<int64_t, ITEM_COUNT_ONE_TIME> DeleteItemSet;
typedef Array<int64_t, ITEM_COUNT_ONE_TIME> NotifyItemSet;
typedef Array<sy::Item, ITEM_COUNT_ONE_TIME> UpdateItemSet;

struct ModifyCurrency {
  typedef void (ModifyCurrency::*bool_type)() const;
  void safe_bool_idionms() const {}

  ModifyCurrency(int32_t msg_id, int32_t sys_id) {
    memset(this, 0, sizeof(*this));
    msgid = msg_id;
    system = sys_id;
  }
  void clear() { memset(this, 0, sizeof(*this)); }

  operator bool_type() const {
    if (bool(coin) + bool(money) + bool(exp) + bool(vip_exp) + bool(oil) +
        bool(energy) + bool(empty) + bool(plane) + bool(hero) + bool(prestige) +
        bool(muscle) + bool(exploit) + bool(_union) + force_update) {
      return &ModifyCurrency::safe_bool_idionms;
    }
    return NULL;
  }

  ModifyCurrency& operator*=(double times) {
    coin = static_cast<int32_t>(coin * times);
    money = static_cast<int32_t>(money * times);
    exp = static_cast<int32_t>(exp * times);
    vip_exp = static_cast<int32_t>(vip_exp * times);
    oil = static_cast<int32_t>(oil * times);
    energy = static_cast<int32_t>(energy * times);
    hero = static_cast<int32_t>(hero * times);
    plane = static_cast<int32_t>(plane * times);
    prestige = static_cast<int32_t>(prestige * times);
    muscle = static_cast<int32_t>(muscle * times);
    exploit = static_cast<int32_t>(exploit * times);
    _union = static_cast<int32_t>(_union * times);

    return *this;
  }

  int32_t& operator[](int32_t index) {
     switch (index) {
       case sy::MONEY_KIND_COIN:  return this->coin;     break;
       case sy::MONEY_KIND_MONEY: return this->money;    break;
       case sy::MONEY_KIND_OIL:   return this->oil;      break;
       case sy::MONEY_KIND_ENERGY:return this->energy;   break;
       case sy::MONEY_KIND_EXP:   return this->exp;      break;
       case sy::MONEY_KIND_VIPEXP:return this->vip_exp;  break;
       case sy::MONEY_KIND_HERO:  return this->hero;     break;
       case sy::MONEY_KIND_PLANE: return this->plane;    break;
       case sy::MONEY_KIND_PRESTIGE:return this->prestige;break;
       case sy::MONEY_KIND_MUSCLE:  return this->muscle; break;
       case sy::MONEY_KIND_EXPLOIT: return this->exploit;break;
       case sy::MONEY_KIND_UNION:   return this->_union; break;
     }
     return empty;
  }

  const int32_t& operator[](int32_t index) const {
     switch (index) {
       case sy::MONEY_KIND_COIN:  return this->coin;     break;
       case sy::MONEY_KIND_MONEY: return this->money;    break;
       case sy::MONEY_KIND_OIL:   return this->oil;      break;
       case sy::MONEY_KIND_ENERGY:return this->energy;   break;
       case sy::MONEY_KIND_EXP:   return this->exp;      break;
       case sy::MONEY_KIND_VIPEXP:return this->vip_exp;  break;
       case sy::MONEY_KIND_HERO:  return this->hero;     break;
       case sy::MONEY_KIND_PLANE: return this->plane;    break;
       case sy::MONEY_KIND_PRESTIGE:return this->prestige;break;
       case sy::MONEY_KIND_MUSCLE:  return this->muscle; break;
       case sy::MONEY_KIND_EXPLOIT: return this->exploit;break;
       case sy::MONEY_KIND_UNION:   return this->_union; break;
     }
     return empty;
  }

  int32_t system; //系统ID
  int32_t msgid;  //消息ID
  int32_t force_update;
  int32_t empty;

  int32_t coin;
  int32_t money;
  int32_t exp;
  int32_t vip_exp;
  int32_t oil;
  int32_t energy;
  int32_t hero;
  int32_t plane;
  int32_t prestige;
  int32_t muscle;
  int32_t exploit;
  int32_t _union;

  int32_t delta_level;
  int32_t delta_vip_level;
};

typedef Array<int64_t, ITEM_COUNT_ONE_TIME> DeletePlaneSet;
typedef Array<int64_t, ITEM_COUNT_ONE_TIME> NotifyPlaneSet;

typedef Array<int64_t, ITEM_COUNT_ONE_TIME> NotifyHeroSet;
typedef Array<int64_t, ITEM_COUNT_ONE_TIME> DeleteHeroSet;
