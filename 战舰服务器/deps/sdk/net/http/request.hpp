//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_REQUEST_HPP
#define HTTP_SERVER2_REQUEST_HPP

#include <string>
#include <vector>
#include <stdlib.h>
#include "header.hpp"

namespace http {
namespace server2 {

struct query_str {
  query_str() {}
  std::string key;
  std::string value;
};

/// Perform URL-decoding on a string. Returns false if the encoding was
/// invalid.
std::string url_decode(const std::string& in);

/// A request received from a client.
struct request
{
  std::string method;
  std::string uri;
  int http_version_major;
  int http_version_minor;
  std::vector<header> headers;
  std::vector<query_str> post_contents;

  void DecodeStrings() {
    uri = url_decode(uri);
    for (std::vector<header>::iterator iter = headers.begin();
         iter != headers.end(); ++iter) {
      iter->name = url_decode(iter->name);
      iter->value= url_decode(iter->value);
    }
    for (std::vector<query_str>::iterator iter = post_contents.begin();
         iter != post_contents.end(); ++iter) {
      iter->key = url_decode(iter->key);
      iter->value = url_decode(iter->value);
    }
  }

  int GetContentLength() const {
    for (std::vector<header>::const_iterator iter = headers.begin();
         iter != headers.end(); ++iter) {
      if (iter->name.compare("Content-Length") == 0) {
        return atoi(iter->value.c_str());
      }
    }
    return 0;
  }
};

} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_REQUEST_HPP
