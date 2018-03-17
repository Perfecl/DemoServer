#include <config_file.h>
#include <logger.h>
#include <signal.h>

ConfigFile::ConfigFile(const std::string& file_name)
    : parse_success_(true), file_name_(file_name), file_changed_time_(0) {}

ConfigFile::~ConfigFile() {}

int32_t ConfigFile::Load() {
  time_t change_time = GetFileChangedTime(this->file_name_.c_str());
  if (change_time != file_changed_time_) {
    this->BeforeLoad();
    if (!this->Parse()) return Fail;
    file_changed_time_ = change_time;
    this->AfterLoad();
    return Success;
  }
  return Ignore;
}

XmlConfigFile::XmlConfigFile(const std::string& file_name)
    : ConfigFile(file_name) {}

XmlConfigFile::~XmlConfigFile() {
}

void XmlConfigFile::BeforeLoad() {
  this->doc_.load_file(this->file_name().c_str());
}

void XmlConfigFile::AfterLoad() {
  this->doc_.reset();
}

bool XmlConfigFile::ParseMySQLParam(const char* path,
                                    __OUT__ MySQLParams& param) {
  if (!this->doc_) return false;

  pugi::xpath_node_set nodes = this->doc_.select_nodes(path);
  if (nodes.begin() != nodes.end()) {
    pugi::xml_node node = nodes.begin()->node();
    param.ip = node.attribute("ip").as_string();
    param.port = node.attribute("port").as_int();
    param.db_name = node.attribute("db_name").as_string();
    param.user_name = node.attribute("user_name").as_string();
    param.password = node.attribute("password").as_string();
    return true;
  }
  return false;
}

ConfigFileManager::~ConfigFileManager() {
  for (ContainerType::iterator iter = this->map_.begin();
       iter != this->map_.end(); ++iter) {
    delete iter->second;
  }
  this->map_.clear();
}

void ConfigFileManager::AddConfigFile(ConfigFile* file) {
  for (ContainerType::iterator iter = this->map_.begin();
       iter != this->map_.end(); ++iter) {
    if (iter->first == file->file_name()) {
      iter->second = file;
      return;
    }
  }
  this->map_.push_back(std::make_pair(file->file_name(), file));
}

bool ConfigFileManager::Load() {
  bool flag = true;
  for (ContainerType::iterator iter = this->map_.begin();
       iter != this->map_.end(); ++iter) {
    int32_t result = iter->second->Load();
    if (!iter->second->parse_success()) flag = false;
    if (result != ConfigFile::Ignore) {
      TRACE_LOG(logger)("Load Config %s, %s", result ==
          ConfigFile::Success ? "success" : "fail", iter->first.c_str());
    }
    if (result == ConfigFile::Fail) return false;
  }

#ifdef DEBUG
  if (!flag) {
    ERROR_LOG(logger)("Config Load Error");
    raise(SIGUSR2);
  }

#endif

  return true;
}

int32_t ConfigFileManager::Reload() {
  int32_t count = 0;
  bool flag = true;
  for (ContainerType::iterator iter = this->map_.begin();
       iter != this->map_.end(); ++iter) {
    int32_t result = iter->second->Load();
    if (!iter->second->parse_success()) flag = false;
    if (result != ConfigFile::Ignore) {
      ++count;
      TRACE_LOG(logger)("Load Config %s, %s", result ==
          ConfigFile::Success ? "success" : "fail", iter->first.c_str());
    }
  }
#ifdef DEBUG
  if (!flag) {
    ERROR_LOG(logger)("Config Reload Error");
    raise(SIGUSR2);
  }
#endif

  return count;
}
