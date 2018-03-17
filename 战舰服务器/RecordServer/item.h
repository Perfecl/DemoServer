#pragma once
#include <item_manager.h>
#include <cpp/message.pb.h>

typedef sy::Item RecordItem;

template <>
struct ItemTrait<RecordItem> {
  static int64_t GetUniqueID(const RecordItem& item) { return item.uid(); }
  static int32_t GetItemID(const RecordItem& item) { return item.item_id(); }
  static void CopyFrom(RecordItem& dest, const RecordItem& from) {
    dest.CopyFrom(from);
  }
};

typedef ItemManager<RecordItem> RecordItemManager;
