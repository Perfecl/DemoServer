#pragma once
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <system.h>
#include <vector>
#include <string>
#include <noncopyable.h>
#include <common_define.h>
#include <singleton.h>
#include <pugixml.hpp>
#include <slice.h>
#include <string.h>
#include <mutex.h>
#include <logger.h>

class ConfigFile : NonCopyable {
 public:
  enum { Success, Fail, Ignore };
  ConfigFile(const std::string& file_name);
  virtual ~ConfigFile();

  //Success, Fail, Ignore
  int32_t Load();
  const std::string& file_name() const { return file_name_; }
  bool parse_success() { return parse_success_; }

 protected:
  virtual void BeforeLoad() {}
  virtual bool Parse() = 0;
  virtual void AfterLoad() {}
  bool parse_success_;

 private:
  const std::string file_name_;
  time_t file_changed_time_;
};

struct MySQLParams {
 std::string ip;
 uint16_t port;
 std::string db_name;
 std::string user_name;
 std::string password;
};

class XmlConfigFile : public ConfigFile {
 public:
  XmlConfigFile(const std::string& file_name);
  virtual ~XmlConfigFile();

  pugi::xml_document& doc() { return doc_; }

  bool ParseMySQLParam(const char* path, __OUT__ MySQLParams& param);

  template <typename Base, typename EntryManager>
  void ParseTable(const char* xpath, const char* id_name) {
    typedef boost::shared_ptr<Base> Ptr;

    std::lock_guard<EntryManager> guard(EntryManager::Instance());

    pugi::xpath_node_set nodes = this->doc_.select_nodes(xpath);
    for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
         iter != nodes.end(); ++iter) {
      pugi::xml_node node = iter->node();
      int64_t id = node.attribute(id_name).as_llong();
      if (id < 0) continue;

      Ptr ptr = EntryManager::Instance().GetEntryInCache(id);
      if (!ptr) {
        ptr = boost::make_shared<Base>();
        ptr->set_id(id);
      }
      if (!ptr->Fill(node)) {
        ERROR_LOG(logger)("load config file:%s, xpath:%s, id:%ld, fail",
                          this->file_name().c_str(), xpath, id);
        this->parse_success_ = false;
        continue;
      }
      if (!EntryManager::Instance().AddEntry(id, ptr)) {
        ERROR_LOG(logger)("load config file:%s, xpath:%s, id:%ld existed",
                          this->file_name().c_str(), xpath, id);
        this->parse_success_ = false;
      }
    }
  }

 protected:
  virtual void BeforeLoad();
  virtual void AfterLoad();

 private:
  pugi::xml_document doc_;
};

class ConfigFileManager : NonCopyable, public Singleton<ConfigFileManager> {
 public:
  ~ConfigFileManager();

  //会按照添加先后顺序来load
  void AddConfigFile(ConfigFile *file);

  //Load失败之后, 会中断, reload不会
  //true, Load成功
  //false, Load失败
  bool Load();
  int32_t Reload();
 private:
  typedef std::pair<std::string, ConfigFile*> value_type;
  typedef std::vector<value_type> ContainerType;
  ContainerType map_;
};

//true是成功
//false是没找到
template <typename T>
inline bool GetXmlAttr(T& value, pugi::xml_node& node, const char* attr);

template <>
inline bool GetXmlAttr(std::string& value, pugi::xml_node& node, const char* attr) {
  value.clear();
  pugi::xml_attribute xml_attr = node.attribute(attr);
  if (!xml_attr) return false;
  value = xml_attr.as_string("");
  return true;
}

template <>
inline bool GetXmlAttr(int8_t& value, pugi::xml_node& node, const char* attr) {
  value = 0;
  pugi::xml_attribute xml_attr = node.attribute(attr);
  if (!xml_attr) return false;
  value = xml_attr.as_int();
  return true;
}

template <>
inline bool GetXmlAttr(int16_t& value, pugi::xml_node& node, const char* attr) {
  value = 0;
  pugi::xml_attribute xml_attr = node.attribute(attr);
  if (!xml_attr) return false;
  value = xml_attr.as_int();
  return true;
}

template <>
inline bool GetXmlAttr(int32_t& value, pugi::xml_node& node, const char* attr) {
  value = 0;
  pugi::xml_attribute xml_attr = node.attribute(attr);
  if (!xml_attr) return false;
  value = xml_attr.as_int();
  return true;
}

template <>
inline bool GetXmlAttr(int64_t& value, pugi::xml_node& node, const char* attr) {
  value = 0;
  pugi::xml_attribute xml_attr = node.attribute(attr);
  if (!xml_attr) return false;
  value = xml_attr.as_llong();
  return true;
}

template <typename T>
inline T ConvertTo(const char*);

template <>
inline int8_t ConvertTo(const char* str) {
  return atoi(str);
}

template <>
inline int16_t ConvertTo(const char* str) {
  return atoi(str);
}

template <>
inline int32_t ConvertTo(const char* str) {
  return atoi(str);
}

template <>
inline int64_t ConvertTo(const char* str) {
  return atoll(str);
}

template <typename T1, typename T2>
struct ValuePair2 {
  typedef T1 value_type1;
  typedef T2 value_type2;

  T1 v1;
  T2 v2;

  ValuePair2() : v1(), v2() {}
  ValuePair2(T1 a, T2 b) :v1(a), v2(b) {}

  bool From(StringSlice s, char split) {
    if (!s.length()) return true;

    StringSlice split_slice(&split, 1);
    size_t pos1, pos2;
    pos2 = s.find(split_slice);
    pos1 = 0;
    if (pos2 == StringSlice::npos) return false;

    v1 = ConvertTo<value_type1>(s.substr(pos1, pos2 - pos1).data());
    pos1 = pos2 + 1;
    pos2 = s.find(split_slice, pos1);
    if (pos1 >= s.length()) return false;

    v2 = ConvertTo<value_type2>(s.substr(pos1).data());
    return true;
  }
};

template <typename T1, typename T2>
static inline bool Fill(std::pair<T1, T2>& v, StringSlice s, char split) {
    if (!s.length()) return true;

    StringSlice split_slice(&split, 1);
    size_t pos1, pos2;
    pos2 = s.find(split_slice);
    pos1 = 0;
    if (pos2 == StringSlice::npos) return false;

    v.first = ConvertTo<T1>(s.substr(pos1, pos2 - pos1).data());
    pos1 = pos2 + 1;
    pos2 = s.find(split_slice, pos1);
    if (pos1 >= s.length()) return false;

    v.second = ConvertTo<T2>(s.substr(pos1).data());
    return true;
}

template <typename T1, typename T2, typename T3>
struct ValuePair3 {
  typedef T1 value_type1;
  typedef T2 value_type2;
  typedef T3 value_type3;

  T1 v1;
  T2 v2;
  T3 v3;

  ValuePair3() : v1(), v2(), v3() {}

  bool From(StringSlice s, char split) {
    if (!s.length()) return true;

    StringSlice split_slice(&split, 1);
    size_t pos1, pos2;
    pos2 = s.find(split_slice);
    pos1 = 0;
    if (pos2 == StringSlice::npos) return false;

    v1 = ConvertTo<value_type1>(s.substr(pos1, pos2 - pos1).data());
    pos1 = pos2 + 1;
    pos2 = s.find(split_slice, pos1);
    if (pos2 == StringSlice::npos) return false;

    v2 = ConvertTo<value_type2>(s.substr(pos1, pos2 - pos1).data());
    pos1 = pos2 + 1;
    pos2 = s.find(split_slice, pos1);
    if (pos1 >= s.length()) return false;

    v3 = ConvertTo<value_type3>(s.substr(pos1).data());
    return true;
  }
};

template <typename T1, typename T2>
inline bool GetXmlAttr(ValuePair2<T1, T2>& value, pugi::xml_node& node,
                const char* attr, char split1 = '|') {
  value = ValuePair2<T1, T2>();
  pugi::xml_attribute xml_attr = node.attribute(attr);
  if (!xml_attr) return false;

  const char* str = xml_attr.as_string();
  StringSlice s(str, strlen(str));
  if (!value.From(s, split1)) return false;

  return true;
}

template <typename T1, typename T2, typename T3>
inline bool GetXmlAttr(ValuePair3<T1, T2, T3>& value, pugi::xml_node& node,
                const char* attr, char split1 = '|') {
  value = ValuePair3<T1, T2, T3>();
  pugi::xml_attribute xml_attr = node.attribute(attr);
  if (!xml_attr) return false;

  const char* str = xml_attr.as_string();
  StringSlice s(str, strlen(str));
  if (!value.From(s, split1)) return false;

  return true;
}

template <typename T1, typename T2>
inline bool GetXmlAttr(std::vector<ValuePair2<T1, T2> >& value, pugi::xml_node& node,
                const char* attr, char split1 = ',', char split2= '|') {
  value.clear();
  pugi::xml_attribute xml_attr = node.attribute(attr);
  if (!xml_attr) return false;

  const char* str = xml_attr.as_string();
  StringSlice s(str, strlen(str));
  char split[] = {split1, '\0'};

  ValuePair2<T1, T2> v;
  size_t pos1, pos2;
  pos2 = s.find(split);
  pos1 = 0;
  while (std::string::npos != pos2) {
    if (!v.From(s.substr(pos1, pos2 - pos1), split2)) return false;
    value.push_back(v);

    pos1 = pos2 + 1;
    pos2 = s.find(split, pos1);
  }
  if (pos1 != s.length()) {
    if (!v.From(s.substr(pos1), split2)) return false;
    value.push_back(v);
  }

  return true;
}

template <typename T1, typename T2>
inline bool GetXmlAttr(std::vector<std::pair<T1, T2> >& value, pugi::xml_node& node,
                const char* attr, char split1 = ',', char split2= '|') {
  value.clear();
  pugi::xml_attribute xml_attr = node.attribute(attr);
  if (!xml_attr) return false;

  const char* str = xml_attr.as_string();
  StringSlice s(str, strlen(str));
  char split[] = {split1, '\0'};

  std::pair<T1, T2> v;
  size_t pos1, pos2;
  pos2 = s.find(split);
  pos1 = 0;
  while (std::string::npos != pos2) {
    if (!Fill(v, s.substr(pos1, pos2 - pos1), split2)) return false;
    value.push_back(v);

    pos1 = pos2 + 1;
    pos2 = s.find(split, pos1);
  }
  if (pos1 != s.length()) {
    if (!Fill(v, s.substr(pos1), split2)) return false;
    value.push_back(v);
  }

  return true;
}

template <typename T1>
inline bool GetXmlAttr(std::vector<T1>& value, pugi::xml_node& node,
                       const char* attr, char split1 = '|') {
  value.clear();
  pugi::xml_attribute xml_attr = node.attribute(attr);
  if (!xml_attr) return false;

  const char* str = xml_attr.as_string();
  StringSlice s(str, strlen(str));
  char split[] = {split1, '\0'};

  T1 v;
  size_t pos1, pos2;
  pos2 = s.find(split);
  pos1 = 0;
  while (std::string::npos != pos2) {
    v = ConvertTo<T1>(s.substr(pos1, pos2 - pos1).data());
    value.push_back(v);

    pos1 = pos2 + 1;
    pos2 = s.find(split, pos1);
  }
  if (pos1 != s.length()) {
    v = ConvertTo<T1>(s.substr(pos1).data());
    value.push_back(v);
  }

  return true;
}

template <typename T1, typename T2, typename T3>
inline bool GetXmlAttr(std::vector<ValuePair3<T1, T2, T3> >& value, pugi::xml_node& node,
                const char* attr, char split1 = ',', char split2 = '|') {
  value.clear();
  pugi::xml_attribute xml_attr = node.attribute(attr);
  if (!xml_attr) return false;

  const char* str = xml_attr.as_string();
  StringSlice s(str, strlen(str));
  char split[] = {split1, '\0'};

  ValuePair3<T1, T2, T3> v;
  size_t pos1, pos2;
  pos2 = s.find(split);
  pos1 = 0;
  while (std::string::npos != pos2) {
    if (!v.From(s.substr(pos1, pos2 - pos1), split2)) return false;
    value.push_back(v);

    pos1 = pos2 + 1;
    pos2 = s.find(split, pos1);
  }
  if (pos1 != s.length()) {
    if (!v.From(s.substr(pos1), split2)) return false;
    value.push_back(v);
  }

  return true;
}
