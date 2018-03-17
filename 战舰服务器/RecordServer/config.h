#pragma once
#include <vector>
#include <common_define.h>
#include <config_file.h>
#include <logger.h>

class ServerConfig : public XmlConfigFile {
 public:
  ServerConfig(const std::string& file_name) : XmlConfigFile(file_name) {}

  //first 是port
  //second是监听端口的类型(Player, GM之类的)
  typedef std::pair<uint16_t, uint16_t> ListenInfo;
  //监听的端口
  const std::vector<ListenInfo>& ports() const { return listen_ports_;}
  //日志等级
  LoggerLevel log_level() const { return log_level_; }
  //认证数据库的地址
  const MySQLParams& mysql() const { return mysql_; }

 protected:
  virtual bool Parse();
 private:
  std::vector<ListenInfo> listen_ports_;
  LoggerLevel log_level_;
  MySQLParams mysql_;
};
