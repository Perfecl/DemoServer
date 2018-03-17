#include <string>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include "str_util.h"
#include "md5.h"
#include <sstream>

std::string encode64(const std::string& str) {
  std::string output;
  typedef boost::archive::iterators::base64_from_binary<boost::archive::iterators::transform_width<std::string::const_iterator, 6, 8> > Base64EncodeIterator;
  std::stringstream result;
  copy(Base64EncodeIterator(str.begin()), Base64EncodeIterator(str.end()),
       std::ostream_iterator<char>(result));
  size_t equal_count = (3 - str.length() % 3) % 3;
  for (size_t i = 0; i < equal_count; i++) {
    result.put('=');
  }
  output = result.str();

  return output;
}

std::string decode64(const std::string& in) {
  std::string str = in;
  std::string output;
  typedef boost::archive::iterators::transform_width<boost::archive::iterators::binary_from_base64<std::string::const_iterator>, 8, 6> Base64DecodeIterator;

  str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
  str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
  std::replace(str.begin(), str.end(), '=', 'A');  // replace '=' by base64 encoding of '\0'
  std::stringstream result;
  try {
    copy(Base64DecodeIterator(str.begin()), Base64DecodeIterator(str.end()),
         std::ostream_iterator<char>(result));
  } catch (...) {
    return "";
  }
  output = result.str();
  return output;
}

std::string GetMD5(const void* begin, size_t len) {
  MD5 md5(begin, len);
  return md5.str();
}

std::string GetMD5(const std::string& str) {
  return GetMD5(&*str.begin(), str.length());
}
