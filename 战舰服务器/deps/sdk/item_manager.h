#pragma once
#include <boost/unordered_map.hpp>
#include <noncopyable.h>

//类型T需要实现
//GetUniqueID(const T*)
//GetItemID(const T*)
//CopyFrom(T& dest, const T& from)
template <typename T>
struct ItemTrait {
  static int64_t GetUniqueID(const T&);
  static int32_t GetItemID(const T&);
  static void CopyFrom(T& dest, const T& from);
};

template <typename T>
class ItemManager : NonCopyable {
 public:
  typedef T value_type;
  typedef value_type* pointer;
  typedef boost::unordered_map<int64_t, value_type> container_type;
  typedef typename container_type::const_iterator const_iterator;

  typedef boost::unordered_multimap<int32_t, int64_t> id_map_type;
  typedef typename id_map_type::iterator id_iterator;

  ItemManager() { this->items_.reserve(256); }

  pointer GetItemByUniqueID(int64_t uid) {
    typename container_type::iterator iter = this->items_.find(uid);
    return iter != this->items_.end() ? &iter->second : NULL;
  }

  pointer GetItemByItemID(int32_t item_id) {
    id_iterator iter = this->item_id_map_.find(item_id);
    if (iter != this->item_id_map_.end()) {
      return this->GetItemByUniqueID(iter->second);
    }
    return NULL;
  }

  void AddItem(const value_type& value) {
    value_type& v = this->items_[ItemTrait<T>::GetUniqueID(value)];
    ItemTrait<T>::CopyFrom(v, value);
    this->AddItemID(ItemTrait<T>::GetItemID(value), ItemTrait<T>::GetUniqueID(value));
  }

  void RemoveItem(int32_t uid) {
    pointer p = this->GetItemByUniqueID(uid);
    if (p)
      this->RemoveItemID(ItemTrait<T>::GetItemID(*p),
                         ItemTrait<T>::GetUniqueID(*p));
    this->items_.erase(uid);
  }

  //!!!遍历的时候不能做删除/插入动作!!!
  template <class Fn>
  void ForEach(Fn fn) const {
    for (const_iterator iter = this->items_.begin();
         iter != this->items_.end(); ++iter) {
      const value_type& value = iter->second;
      fn(value);
    }
  }

  const_iterator begin() const { return this->items_.begin(); }
  const_iterator end() const { return this->items_.end(); }

  size_t size() const { return this->items_.size(); }
  void clear() { this->items_.clear(); }

 private:
  void AddItemID(int32_t item_id, int64_t uid) {
    std::pair<id_iterator, id_iterator> pair =
        this->item_id_map_.equal_range(item_id);
    for (id_iterator iter = pair.first; iter != pair.second; ++iter) {
      if (iter->second == uid) return;
    }
    this->item_id_map_.insert(std::make_pair(item_id, uid));
  }

  void RemoveItemID(int32_t item_id, int64_t uid) {
    std::pair<id_iterator, id_iterator> pair =
        this->item_id_map_.equal_range(item_id);
    for (id_iterator iter = pair.first; iter != pair.second; ++iter) {
      if (iter->second == uid) {
        this->item_id_map_.erase(iter);
        break;
      }
    }
  }

 private:
  container_type items_;
  id_map_type item_id_map_;
};
