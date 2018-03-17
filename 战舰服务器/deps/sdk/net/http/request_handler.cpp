//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_handler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"

namespace http {
namespace server2 {

request_handler::request_handler() {}

int32_t request_handler::handle_request(
    const request& req, reply& rep, const boost::shared_ptr<connection>& conn) {
  (void)rep;
  // Decode url to path.
  const std::string& request_path = req.uri;
  // print post query string
  std::cout << request_path << std::endl;
  for (std::vector<query_str>::const_iterator iter = req.post_contents.begin();
       iter != req.post_contents.end(); ++iter) {
    std::cout << iter->key << " => " << iter->value << std::endl;
  }
  return 0;
}

unsigned char FromHex(unsigned char x) {
  unsigned char y = 0;
  if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
  else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
  else if (x >= '0' && x <= '9') y = x - '0';
  else assert(0);
  return y;
}

std::string url_decode(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  size_t length = in.length();
  for (size_t i = 0; i < length; i++) {
    if (in[i] == '+') {
      out += ' ';
    } else if (in[i] == '%') {
      assert(i + 2 < length);
      unsigned char high = FromHex((unsigned char)in[++i]);
      unsigned char low = FromHex((unsigned char)in[++i]);
      out += high * 16 + low;
    } else {
      out += in[i];
    }
  }
  return out;
}

} // namespace server2
} // namespace http
