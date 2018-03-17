#ifndef __HTTP_REQUEST_HANDLER_H__
#define __HTTP_REQUEST_HANDLER_H__
#include <net/http/request_handler.hpp>
#include <net/http/connection.hpp>
#include "closure.h"
#include <mutex.h>

namespace http {
namespace server2 {

class HttpRequestHandler : public request_handler {
 public:
  HttpRequestHandler();

  virtual int32_t handle_request(const request& req, reply& rep,
                                 const boost::shared_ptr<connection>& conn);

  //处理充值
  int32_t handle_payment(const request& req, reply& rep, const boost::shared_ptr<connection>& conn);
};
}
}

typedef http::server2::http_handler_ptr HttpHandlerPtr;

#endif

class ConnectionManager : NonCopyable, public Singleton<ConnectionManager> {
 public:
  void AddConnection(http::server2::connection_ptr conn);
  //只能提取一次
  http::server2::connection_ptr FetchConnection(int64_t id);
  void RemoveIdleConnection();

 private:
  boost::unordered_map<int64_t, http::server2::connection_ptr> connection_map_;
  std::mutex lock_;
};
