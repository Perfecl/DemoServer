#include "closure.h"
#include "system.h"

Closure::Closure()
    : id_(ClosureManager::Instance().NewID()), time_(GetSeconds()) {}

ClosureManager::ClosureManager() {
  this->id_seeds_ = 0;
}

int64_t ClosureManager::NewID() { return ++this->id_seeds_; }

ClosurePtr ClosureManager::GetClosure(int64_t id) {
  boost::unordered_map<int64_t, ClosurePtr>::iterator iter =
      this->closures_.find(id);
  ClosurePtr result;
  if (iter != this->closures_.end()) {
    std::swap(result, iter->second);
    this->closures_.erase(iter);
  }
  return result;
}

void ClosureManager::Update() {
  int64_t millisec = GetMilliSeconds();
  std::multimap<int64_t, int64_t>::iterator end =
      this->time_out_queue_.lower_bound(millisec);
  if (end == this->time_out_queue_.end()) return;
  for (std::multimap<int64_t, int64_t>::iterator iter =
           this->time_out_queue_.begin();
       iter != end; ++iter) {
    const ClosurePtr& ptr = this->GetClosure(iter->second);
    if (ptr) ptr->TimeOut();
  }
  this->time_out_queue_.erase(this->time_out_queue_.begin(), end);
}
