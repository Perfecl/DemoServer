#include "config.h"
#include <pugixml.hpp>
#include <logger.h>

bool ServerConfig::Parse() {
 pugi::xml_document& doc = this->doc();
 if (!doc) return false;

 {
   pugi::xpath_node_set nodes = doc.select_nodes("/root/listen");
   for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
        iter != nodes.end(); ++iter) {
     pugi::xml_node node = iter->node();
     ListenInfo info;
     info.first = node.attribute("port").as_int();
     std::string type = node.attribute("type").as_string();
     if (type == "server") info.second = ENTRY_TYPE_LOGIC_SERVER;

     if (info.second != 0) this->listen_ports_.push_back(info);
   }
 }

 {
   pugi::xpath_node_set nodes = doc.select_nodes("/root/log_level");
   if (nodes.begin() != nodes.end()) {
    pugi::xml_node node = nodes.begin()->node();
    log_level_ = (LoggerLevel)node.attribute("level").as_int();
    logger->log_level(log_level_);
   }
 }

 if (!this->ParseMySQLParam("/root/mysql", this->mysql_)) {
    ERROR_LOG(logger)("Load /root/mysql fail");
    return false;
 }

 return true;
}
