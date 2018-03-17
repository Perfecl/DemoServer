#include "time_activity.h"
#include <array_stream.h>
#include "logic_player.h"
#include "str_util.h"
#include "server.h"

void StringToValue(std::string str, int32_t& value) {
  value = atoi(str.c_str());
}

void StringToValue(std::string str, int64_t& value) {
  value = atoll(str.c_str());
}

void StringToValue(std::string str, std::string& value) { value = str; }

void StringToValue(std::string str,
                   std::vector<ValuePair2<int32_t, int32_t> >& value) {
  value.clear();
  if (str.empty()) return;

  StringSlice s(str);
  char split[] = {',', '\0'};

  ValuePair2<int32_t, int32_t> v;
  size_t pos1, pos2;
  pos2 = s.find(split);
  pos1 = 0;
  while (std::string::npos != pos2) {
    if (!v.From(s.substr(pos1, pos2 - pos1), '|')) return;
    value.push_back(v);
    pos1 = pos2 + 1;
    pos2 = s.find(split, pos1);
  }

  if (pos1 != s.length()) {
    if (!v.From(s.substr(pos1), '|')) return;
    value.push_back(v);
  }
}


void ActivityRowNew::FillHeader(PbStringArray* header) const {
  header->Clear();
  for (VectorMap<std::string, std::string>::const_iterator it =
           field_map_.begin();
       it != field_map_.end(); ++it)
    *header->Add() = it->first;
}

void ActivityRowNew::FillRow(PbStringArray* row) const {
  for (VectorMap<std::string, std::string>::const_iterator it =
           field_map_.begin();
       it != field_map_.end(); ++it)
    *row->Add() = it->second;
}

void TimeActivityNew::set_info(const ActivityInfoConfig& info) {
  this->begin_time(info.begin_time);
  this->end_time(info.end_time);
  this->description(info.desc);

  for (size_t i = 0; i < info.raw_content.size(); i++) {
    ActivityRowNew row_new;
    for (size_t j = 0;
         j < info.raw_field.size() && j < info.raw_content[i].size(); j++) {
      row_new.field_map_[info.raw_field[j]] = info.raw_content[i][j];
      if (info.raw_field[j] == "content")
        StringToValue(info.raw_content[i][j], row_new.award_);
    }
    this->rows_.push_back(row_new);
  }
}

void TimeActivityNew::set_info(const sy::TimeActivityInfo& info) {
  this->begin_time(info.begin_time());
  this->end_time(info.end_time());
  this->description(info.description());

  for (int32_t i = 0; i < info.rows_size(); i++) {
    ActivityRowNew row_new;
    for (int32_t j = 0;
         j < info.headers_size() && j < info.rows(i).columns_size(); j++) {
      row_new.field_map_[info.headers(j)] = info.rows(i).columns(j);
      if (info.headers(j) == "content")
        StringToValue(info.rows(i).columns(j), row_new.award_);
    }
    this->rows_.push_back(row_new);
  }
}

sy::MessageResponseTimeActivity* ActivityManagerNew::GetMessage() {
  if (!this->message_.infos_size()) {
    for (std::vector<ActivityPtr>::iterator iter = this->activity_.begin();
         iter != this->activity_.end(); ++iter) {
      ActivityPtr ptr = *iter;
      if (GetVirtualSeconds() < ptr->begin_time() ||
          GetVirtualSeconds() >= ptr->end_time())
        continue;

      if (Setting::GetValueInVec2(Setting::activity_open_day, ptr->type()) >
          server->GetServerStartDays())
        continue;

      sy::TimeActivityInfo* info = this->message_.add_infos();
      info->set_type(ptr->type());
      info->set_begin_time(ptr->begin_time());
      info->set_end_time(ptr->end_time());
      info->set_description(ptr->description());
      int32_t count = ptr->row_count();
      for (int32_t i = 0; i < count; ++i) {
        if (!i) ptr->row(i)->FillHeader(info->mutable_headers());
        sy::TimeActivityRow* row = info->add_rows();
        ptr->row(i)->FillRow(row->mutable_columns());
      }
    }
  }
  return &this->message_;
}

void ActivityManagerNew::PrintLog() {
  DEBUG_LOG(logger)("TimeActivityLogStart");
  ArrayStream<1024> stream;
  for (std::vector<ActivityPtr>::iterator iter = this->activity_.begin();
       iter != this->activity_.end(); ++iter) {
    ActivityPtr ptr = *iter;
    TRACE_LOG(logger)("type:%d, id:%ld, begin_time:%d, end_time:%d, description:%s",
        ptr->type(), ptr->id(), ptr->begin_time(), ptr->end_time(), ptr->description().c_str());
    for (int32_t i = 0; i < ptr->row_count(); ++i) {
      stream.clear();
      for (VectorMap<std::string, std::string>::const_iterator it =
               ptr->row(i)->field_map_.begin();
           it != ptr->row(i)->field_map_.end(); ++it) {
        stream.Append("%s:%s,", it->first.c_str(), it->second.c_str());
      }
      TRACE_LOG(logger)(stream.str().c_str(), "");
    }
  }
  DEBUG_LOG(logger)("TimeActivityLogEnd");
}

const ActivityRowNew* TimeActivityNew::row(int32_t index) const {
  static ActivityRowNew empty;
  if (index >= 0 && index < this->row_count()) {
    return &this->rows_[index];
  }
  return &empty;
}

void ActivityManagerNew::GetAllActivity(sy::TimeActivityType type,
                                        std::vector<ActivityPtr>& vec) {

  vec.clear();
  int32_t mapped_type = type % 100;
  if (mapped_type >= sy::TIME_ACTIVITY_FESTIVAL_RECHARGE &&
      mapped_type <= sy::TIME_ACTIVITY_FESTIVAL_END &&
      server->GetServerStartDays() < GetSettingValue(festival_time0))
    return;

  for (size_t i = 0; i < activity_.size(); i++) {
    if (mapped_type == activity_[i]->type() % 100) {
      if (GetVirtualSeconds() < activity_[i]->begin_time() ||
          GetVirtualSeconds() > activity_[i]->end_time())
        continue;
      if (Setting::GetValueInVec2(Setting::activity_open_day, mapped_type) >
          server->GetServerStartDays())
        continue;
      vec.push_back(activity_[i]);
    }
  }
}

void ActivityManagerNew::SetActivityRecordCount(sy::TimeActivityType type,
                                                int32_t value,
                                                LogicPlayer* player) {
  if (!player) return;
  std::vector<ActivityPtr> vec;
  GetAllActivity(type, vec);

  for (std::vector<ActivityPtr>::iterator it = vec.begin(); it != vec.end();
       ++it) {
    TimeActivityNew* activity = it->get();
    if (!activity) continue;
    if (value > player->GetActivityRecordNew(type, activity->id(), 0))
      player->SetActivityRecordNew(type, activity->id(), 0, value);
  }
}

void ActivityManagerNew::AddActivityRecordCount(sy::TimeActivityType type,
                                                int32_t value,
                                                LogicPlayer* player) {
  if (!player) return;
  std::vector<ActivityPtr> vec;
  GetAllActivity(type, vec);

  for (std::vector<ActivityPtr>::iterator it = vec.begin(); it != vec.end();
       ++it) {
    TimeActivityNew* activity = it->get();
    if (!activity) continue;
    if (activity->type() == sy::TIME_ACTIVITY_FESTIVAL_RECHARGE) {
      for (int32_t j = 0; j < activity->row_count(); ++j) {
        if (activity->row(j)->GetInt32("money") == value) {
          player->AddActivityRecordNew(activity->type(), activity->id(),
                                       -activity->row(j)->GetInt32("id"), 1);
          break;
        }
      }
    } else {
      player->AddActivityRecordNew(activity->type(), activity->id(), 0, value);
    }
  }
}

int32_t ActivityRowNew::GetInt32(const std::string& key) const {
  VectorMap<std::string, std::string>::const_iterator it = field_map_.find(key);
  if (it == field_map_.end()) return 0;
  return atoi(it->second.c_str());
}

int64_t ActivityRowNew::GetInt64(const std::string& key) const {
  VectorMap<std::string, std::string>::const_iterator it = field_map_.find(key);
  if (it == field_map_.end()) return 0;
  return atoll(it->second.c_str());
}

std::string ActivityRowNew::GetString(const std::string& key) const {
  VectorMap<std::string, std::string>::const_iterator it = field_map_.find(key);
  if (it == field_map_.end()) return "";
  return it->second;
}

int64_t ActivityManagerNew::GetActivityID(sy::TimeActivityType type) {
  for (size_t i = 0; i < activity_.size(); i++) {
    if (type == activity_[i]->type()) {
      if (GetVirtualSeconds() >= activity_[i]->begin_time() &&
          GetVirtualSeconds() <= activity_[i]->end_time())
        return activity_[i]->id();
    }
  }
  return 0;
}

void ActivityManagerNew::SetActivityRecord(sy::ActivityRecord& record,
                                           LogicPlayer* player) {
  record.clear_award();
  if (record.type() == sy::TIME_ACTIVITY_RECHARGE) return;
  VectorMap<int32_t, int32_t> awards;

  for (size_t i = 0; i < activity_.size(); i++) {
    ActivityPtr& act = activity_[i];
    if (!act) continue;
    if (act->type() == record.type() && act->id() == record.id()) {
      for (int32_t j = 0; j < act->row_count(); j++) {
        const ActivityRowNew* row = act->row(j);

        if (act->type() == sy::TIME_ACTIVITY_FESTIVAL_RECHARGE) {
          int32_t max_count = row->GetInt32("count");
          int32_t count =
              player->GetActivityRecordNew((sy::TimeActivityType)record.type(),
                                           record.id(), -row->GetInt32("id"));
          count = count < max_count ? count : max_count;
          count =
              count -
              player->GetActivityRecordNew((sy::TimeActivityType)record.type(),
                                           record.id(), row->GetInt32("id"));
          for (int32_t i = 0; i < count; i++) {
            for (ValuePair2Vec::const_iterator it = row->award_.begin();
                 it != row->award_.end(); ++it) {
              if (awards.find(it->v1) == awards.end()) awards[it->v1] = 0;
              awards[it->v1] += it->v2;
            }
          }
        } else if (act->type() == sy::TIME_ACTIVITY_FESTIVAL_LOGIN) {
          int32_t rec = player->GetActivityRecordNew(
              (sy::TimeActivityType)record.type(), record.id(), 0);
          int32_t day = row->GetInt32("count");
          if (day > 0 && (rec & (1 << (day - 1)))) {
            for (ValuePair2Vec::const_iterator it = row->award_.begin();
                 it != row->award_.end(); ++it) {
              if (awards.find(it->v1) == awards.end()) awards[it->v1] = 0;
              awards[it->v1] += it->v2;
            }
          }
        } else {
          if (player->GetActivityRecordNew((sy::TimeActivityType)record.type(),
                                           record.id(),
                                           0) >= row->GetInt32("count") &&
              !player->GetActivityRecordNew((sy::TimeActivityType)record.type(),
                                            record.id(), row->GetInt32("id"))) {
            for (ValuePair2Vec::const_iterator it = row->award_.begin();
                 it != row->award_.end(); ++it) {
              if (awards.find(it->v1) == awards.end()) awards[it->v1] = 0;
              awards[it->v1] += it->v2;
            }
          }
        }
      }
      break;
    }
  }
  if (!awards.empty()) {
    std::string temp = ":";
    for (VectorMap<int32_t, int32_t>::iterator it = awards.begin();
         it != awards.end(); ++it) {
      if (temp.length() > 1) temp += ",";
      char buff[64] = {0};
      sprintf(buff, "%d|%d", it->first, it->second);
      temp += buff;
    }
    record.add_award(temp);
  }
}

void ActivityManagerNew::ClearActivityByType(int32_t type) {
  this->message_.Clear();
  for (std::vector<ActivityPtr>::iterator iter = this->activity_.begin();
       iter != this->activity_.end(); ) {
    const ActivityPtr& ptr = *iter;
    if (!ptr) {
      iter = this->activity_.erase(iter);
      continue;
    }
    if (ptr->type() >= type * 100 && ptr->type() < type * 100 + 100) {
      iter = this->activity_.erase(iter);
      continue;
    }

    ++iter;
  }
  std::vector<ActivityPtr> activity_;
}

void ActivityManagerNew::AddActivityTemplate(const ActivityInfoConfig& config) {
  boost::shared_ptr<TimeActivityNew> ptr(
      new TimeActivityNew((sy::TimeActivityType)config.activity_type, config.activity_id));
  ptr->set_info(config);
  if (ptr->row_count()) this->activity_.push_back(ptr);
}

void ActivityManagerNew::AddActivityTemplate(const sy::TimeActivityInfo& info) {
  boost::shared_ptr<TimeActivityNew> ptr(
          new TimeActivityNew((sy::TimeActivityType)info.type(), info.id()));
  ptr->set_info(info);
  if (ptr->row_count()) this->activity_.push_back(ptr);
}
