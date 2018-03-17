#ifndef __TIME_ACTIVITY_H__
#define __TIME_ACTIVITY_H__
#include <cpp/message.pb.h>
#include <vector>
#include <singleton.h>
#include <boost/shared_ptr.hpp>
#include <string>
#include "config.h"

typedef google::protobuf::RepeatedPtrField<std::string> PbStringArray;


struct ActivityRowNew {
  int32_t GetInt32(const std::string& key) const;
  int64_t GetInt64(const std::string& key) const;
  std::string GetString(const std::string& key) const;

  const ValuePair2Vec& GetAward() const { return award_; }

  VectorMap<std::string, std::string> field_map_;
  ValuePair2Vec award_;

  void FillHeader(PbStringArray* header) const;
  void FillRow(PbStringArray* row) const;
};

class TimeActivityNew {
 public:
  TimeActivityNew(int32_t type, int64_t id) : type_(type), id_(id) {}
  ~TimeActivityNew() {}

  void set_info(const ActivityInfoConfig& info);
  void set_info(const sy::TimeActivityInfo& info);

  int32_t type() const { return this->type_; }
  int64_t id() const { return this->id_; }
  int32_t begin_time() const { return this->begin_time_; }
  int32_t end_time() const { return this->end_time_; }
  void begin_time(int32_t time) { this->begin_time_ = time; }
  void end_time(int32_t time) { this->end_time_ = time; }
  const std::string& description() { return this->description_; }
  void description(const std::string& str) { this->description_ = str; }

  int32_t row_count() const { return rows_.size(); }
  const ActivityRowNew* row(int32_t index) const;

 private:
  int32_t type_;
  int64_t id_;
  int32_t begin_time_;
  int32_t end_time_;
  std::string description_;

  std::vector<ActivityRowNew> rows_;
};

class ActivityManagerNew : public Singleton<ActivityManagerNew> {
 public:
  typedef boost::shared_ptr<TimeActivityNew> ActivityPtr;

  void AddActivityTemplate(const ActivityInfoConfig& config);
  void AddActivityTemplate(const sy::TimeActivityInfo& info);

  TimeActivityNew* GetAs(int32_t type) {
    for (std::vector<ActivityPtr>::iterator iter = this->activity_.begin();
         iter != this->activity_.end(); ++iter) {
      if ((*iter)->type() == type &&
          GetVirtualSeconds() >= (*iter)->begin_time() &&
          GetVirtualSeconds() <= (*iter)->end_time()) {
        return iter->get();
      }
    }
    return NULL;
  }

  TimeActivityNew* FindAs(int32_t type, int64_t id) {
    for (std::vector<ActivityPtr>::iterator iter = this->activity_.begin();
         iter != this->activity_.end(); ++iter) {
      if ((*iter)->type() == type && (*iter)->id() == id) return iter->get();
    }
    return NULL;
  }

  void ClearGMActivity() { this->ClearActivityByType(0); }
  void ClearServerActivity() { this->ClearActivityByType(3); }
  void ClearWeeklyActivity() { this->ClearActivityByType(2); }

  //0:GM后台活动
  //1:开服活动
  //2:每周活动
  void ClearActivityByType(int32_t type);
  void ClearMessage() { this->message_.Clear(); }

  sy::MessageResponseTimeActivity* GetMessage();

  void PrintLog();
  int64_t GetActivityID(sy::TimeActivityType type);

  void GetAllActivity(sy::TimeActivityType type, std::vector<ActivityPtr>& vec);

  void SetActivityRecordCount(sy::TimeActivityType type, int32_t value,
                              LogicPlayer* player);
  void AddActivityRecordCount(sy::TimeActivityType type, int32_t value,
                              LogicPlayer* player);

  void SetActivityRecord(sy::ActivityRecord& record, LogicPlayer* player);

 private:
  std::vector<ActivityPtr> activity_;
  sy::MessageResponseTimeActivity message_;
};

#define ACTIVITY  ActivityManagerNew::Instance()

#endif

