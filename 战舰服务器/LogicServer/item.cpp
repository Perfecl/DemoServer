#include "item.h"

LogicItem::LogicItem() {
  this->impl_.item_type_ = sy::ITEM_TYPE_NONE;
}

void LogicItem::CopyFrom(const LogicItem& other) {
  this->impl_ = other.impl_;
}

int32_t LogicItem::GetAttr(int32_t key) const {
  for (int32_t i = 0; i < this->impl_.data_.attr_size(); ++i) {
    const sy::ItemAttribute& attr = this->impl_.data_.attr(i);
    if (attr.key() == key) return attr.value();
  }
  return 0;
}

void LogicItem::SetAttr(int32_t key, int32_t value) {
  for (int32_t i = 0; i < this->impl_.data_.attr_size(); ++i) {
    sy::ItemAttribute* attr = this->impl_.data_.mutable_attr(i);
    if (attr->key() == key) {
      attr->set_value(value);
      return;
    }
  }

  sy::ItemAttribute* attr = this->impl_.data_.add_attr();
  attr->set_key(key);
  attr->set_value(value);
}

void LogicItem::CopyAttr(const sy::Item& item) {
  this->impl_.data_.clear_attr();
  this->impl_.data_.set_item_id(item.item_id());
  this->impl_.data_.set_count(item.count());
  for (int32_t i = 0; i < item.attr_size(); ++i) {
    this->impl_.data_.add_attr()->CopyFrom(item.attr(i));
  }
}

int32_t LogicItem::max_count() const {
  if (this->item_type() == sy::ITEM_TYPE_ITEM) {
    return this->item_base() ? this->item_base()->max_stack : 1;
  }
  if (this->item_type() == sy::ITEM_TYPE_PLANE) {
    return GetSettingValue(plane_stack_max);
  }
  return 1;
}

bool LogicItem::is_full() const {
  int32_t max_count = this->max_count();
  return max_count && this->impl_.data_.count() < max_count;
}

int32_t LogicItem::equip_type() const {
  //装备
  if (this->equip_base()) return this->equip_base()->type;
  //宝物
  if (this->army_base()) return this->army_base()->type;

  return -1;
}

int32_t LogicItem::GetMaxCount(int32_t item_id) {
  const std::pair<int32_t, boost::shared_ptr<ConfigEntry> >& pair =
      GetItemParam(item_id);
  if (pair.first == sy::ITEM_TYPE_ITEM) {
    const ItemBasePtr& base = boost::static_pointer_cast<ItemBase>(pair.second);
    if (!base) return 1;
    return base->max_stack;
  }
  if (pair.first == sy::ITEM_TYPE_PLANE) {
    return GetSettingValue(plane_stack_max);
  }
  return 1;
}

std::pair<int32_t, boost::shared_ptr<ConfigEntry> > LogicItem::GetItemParam(
    int32_t item_id) {
  {
    const ItemBasePtr& base = ITEM_BASE.GetEntryByID(item_id);
    if (base) return std::make_pair(sy::ITEM_TYPE_ITEM, base);
  }
  {
    const EquipBasePtr& base = EQUIP_BASE.GetEntryByID(item_id);
    if (base) return std::make_pair(sy::ITEM_TYPE_EQUIP, base);
  }
  {
    const ArmyBasePtr& base = ARMY_BASE.GetEntryByID(item_id);
    if (base) return std::make_pair(sy::ITEM_TYPE_ARMY, base);
  }
  {
    const CarrierPlaneBasePtr& base = CARRIER_PLANE_BASE.GetEntryByID(item_id);
    if (base) return std::make_pair(sy::ITEM_TYPE_PLANE, base);
  }
  return std::make_pair(sy::ITEM_TYPE_NONE, boost::shared_ptr<ConfigEntry>());
}

bool LogicItem::ResetItemID(int32_t item_id) {
  const std::pair<int32_t, boost::shared_ptr<ConfigEntry> >& param = LogicItem::GetItemParam(item_id);
  if (param.first && param.second) {
    this->impl_.base_ = param.second;
    this->impl_.item_type_ = param.first;
    this->impl_.data_.set_item_id(item_id);
    return true;
  }
  return false;
}

bool LogicItem::CheckItem(int32_t item_id) {
  const std::pair<int32_t, boost::shared_ptr<ConfigEntry> >& param = LogicItem::GetItemParam(item_id);
  if (param.first && param.second) {
    return true;
  }
  return false;
}

bool LogicItem::CreateItem(const ItemParam& param, __OUT__ LogicItem& item) {
  item.clear();
  item.impl_.data_.set_uid(0);
  item.impl_.data_.set_item_id(param.item_id);
  item.impl_.data_.set_count(param.item_count);

  //派发
  const std::pair<int32_t, boost::shared_ptr<ConfigEntry> >& item_config = LogicItem::GetItemParam(param.item_id);
  if (item_config.first && item_config.second) {
    item.impl_.base_ = item_config.second;
    item.impl_.item_type_ = item_config.first;
    return true;
  }
  ERROR_LOG(logger)("CreateItem, ItemID:%d, ItemType:%d not found", param.item_id, item_config.first);
  return false;
}

bool LogicItem::CreateItem(sy::Item& data,
                                  __OUT__ LogicItem& item) {
  ItemParam param;
  param.item_id = data.item_id();
  param.item_count = data.count();
  if (LogicItem::CreateItem(param, item)) {
    item.impl_.data_.Clear();
    item.impl_.data_.CopyFrom(data);
    return true;
  }
  return false;
}

ValuePair2<int32_t, int32_t> LogicItem::sell_price() const {
  ValuePair2<int32_t, int32_t> no_price;

  if (this->item_base()) {
    const ItemBase* base = this->item_base();
    return base->sell;
  } else if (this->equip_base()) {
    const EquipBase* base = this->equip_base();
    return base->sell;
  } else if (this->plane_base()) {
    return no_price;
  } else if (this->army_base()) {
    const ArmyBase* base = this->army_base();
    return base->sell;
  }

  return no_price;
}

