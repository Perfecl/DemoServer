//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_REQUEST_HANDLER_HPP
#define HTTP_SERVER2_REQUEST_HANDLER_HPP

#include <boost/shared_ptr.hpp>
#include <string>
#include <boost/noncopyable.hpp>

namespace http {
namespace server2 {

struct reply;
struct request;
class request_handler;
typedef boost::shared_ptr<request_handler> http_handler_ptr;
class connection;

/// The common handler for all incoming requests.
class request_handler
  : private boost::noncopyable
{
public:
  /// Construct with a directory containing files to be served.
  request_handler();
  virtual ~request_handler() {}

  // Handle a request and produce a reply.
  // result = 0, means OK
  // else wait
  virtual int32_t handle_request(const request& req, reply& rep,
                                 const boost::shared_ptr<connection>& conn);
};

} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_REQUEST_HANDLER_HPP
