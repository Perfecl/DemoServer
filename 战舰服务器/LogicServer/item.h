#pragma once
#include <item_manager.h>
#include <cpp/message.pb.h>
#include <noncopyable.h>
#include <array.h>
#include "define.h"
#include "config.h"

class LogicItem : NonCopyable {
 public:
  LogicItem();
  LogicItem(sy::Item& item);

  int32_t item_type() const { return this->impl_.item_type_; }
  int64_t uid() const { return this->impl_.data_.uid(); }
  int32_t item_id() const { return this->impl_.data_.item_id(); }
  void CopyFrom(const LogicItem& other);

  bool ResetItemID(int32_t new_item_id);
  int32_t GetAttr(int32_t key) const;
  void SetAttr(int32_t key, int32_t value);
  void CopyAttr(const sy::Item& item);

  const sy::Item& data() const { return this->impl_.data_; }
  //普通道具(可以堆叠的)
  const ItemBase* item_base() const {
    return this->item_type() == sy::ITEM_TYPE_ITEM
               ? static_cast<const ItemBase*>(this->impl_.base_.get())
               : NULL;
  }
  //装备
  const EquipBase* equip_base() const  {
    return this->item_type() == sy::ITEM_TYPE_EQUIP
               ? static_cast<const EquipBase*>(this->impl_.base_.get())
               : NULL;
  }
  //飞机, 宝石
  const CarrierPlaneBase* plane_base() const {
    return this->item_type() == sy::ITEM_TYPE_PLANE
               ? static_cast<const CarrierPlaneBase*>(this->impl_.base_.get())
               : NULL;
  }
  //宝物, 海军
  const ArmyBase* army_base() const {
    return this->item_type() == sy::ITEM_TYPE_ARMY
               ? static_cast<const ArmyBase*>(this->impl_.base_.get())
               : NULL;
  }

  sy::Item& item() { return this->impl_.data_; }
  void clear() {
    this->impl_.clear();
  }

  ValuePair2<int32_t,int32_t> sell_price() const;

  int32_t count() const { return this->impl_.data_.count(); }
  void count(int32_t count) { this->impl_.data_.set_count(count); }
  int32_t max_count() const;
  bool is_full() const;

  int32_t equip_type() const;

  static int32_t GetMaxCount(int32_t item_id);

 public:
  static bool CheckItem(int32_t item_id);
  static std::pair<int32_t, boost::shared_ptr<ConfigEntry> > GetItemParam(int32_t item_id);
  //工厂函数
  static bool CreateItem(const ItemParam& param, __OUT__ LogicItem& item);
  static bool CreateItem(sy::Item& item, __OUT__ LogicItem& logic_item);

 private:
  void set_type(int32_t type) { this->impl_.item_type_ = type; }

 private:
  struct ItemData {
    boost::shared_ptr<ConfigEntry> base_;
    int32_t item_type_;
    sy::Item data_;

    void clear() {
      this->base_.reset();
      this->item_type_ = sy::ITEM_TYPE_NONE;
      this->data_.Clear();
    }
  };

  ItemData impl_;
};

template <>
struct ItemTrait<LogicItem> {
  static int64_t GetUniqueID(const LogicItem& item) { return item.uid(); }
  static int32_t GetItemID(const LogicItem& item) { return item.item_id(); }
  static void CopyFrom(LogicItem& dest, const LogicItem& from) {
    dest.CopyFrom(from);
  }
};

typedef ItemManager<LogicItem> LogicItemManager;
