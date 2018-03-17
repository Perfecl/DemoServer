//
// async_AsyncClient.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <boost/function.hpp>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using boost::asio::ip::tcp;

class AsyncClient
{
public:
 AsyncClient(boost::asio::io_service& io_service)
     : resolver_(io_service), socket_(io_service) {}

 void send_request(const std::string& server, const std::string& path,
                   const std::string& method = "GET") {
    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    std::ostream request_stream(&request_);
    request_stream << method << " " << path << " HTTP/1.0\r\n";
    request_stream << "Host: " << server << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Start an asynchronous resolve to translate the server and service names
    // into a list of endpoints.
    tcp::resolver::query query(server, "http");
    resolver_.async_resolve(query,
        boost::bind(&AsyncClient::handle_resolve, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::iterator));
  }

  boost::function<void(AsyncClient*)> callback;
  const std::string& content() const { return this->response_content_; }
private:
  void handle_end() {
    if (this->callback) {
      this->callback(this);
    }
  }

  void handle_resolve(const boost::system::error_code& err,
      tcp::resolver::iterator endpoint_iterator)
  {
    if (!err)
    {
      // Attempt a connection to each endpoint in the list until we
      // successfully establish a connection.
      boost::asio::async_connect(socket_, endpoint_iterator,
          boost::bind(&AsyncClient::handle_connect, this,
            boost::asio::placeholders::error));
    }
    else
    {
      //std::cout << "Error: " << err.message() << "\n";
      this->handle_end();
    }
  }

  void handle_connect(const boost::system::error_code& err)
  {
    if (!err)
    {
      // The connection was successful. Send the request.
      boost::asio::async_write(socket_, request_,
          boost::bind(&AsyncClient::handle_write_request, this,
            boost::asio::placeholders::error));
    }
    else
    {
      //std::cout << "Error: " << err.message() << "\n";
      this->handle_end();
    }
  }

  void handle_write_request(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Read the response status line. The response_ streambuf will
      // automatically grow to accommodate the entire line. The growth may be
      // limited by passing a maximum size to the streambuf constructor.
      boost::asio::async_read_until(socket_, response_, "\r\n",
          boost::bind(&AsyncClient::handle_read_status_line, this,
            boost::asio::placeholders::error));
    }
    else
    {
      //std::cout << "Error: " << err.message() << "\n";
      this->handle_end();
    }
  }

  void handle_read_status_line(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Check that response is OK.
      std::istream response_stream(&response_);
      std::string http_version;
      response_stream >> http_version;
      unsigned int status_code;
      response_stream >> status_code;
      std::string status_message;
      std::getline(response_stream, status_message);
      if (!response_stream || http_version.substr(0, 5) != "HTTP/")
      {
        //std::cout << "Invalid response\n";
        this->handle_end();
        return;
      }
      if (status_code != 200)
      {
        //std::cout << "Response returned with status code ";
        //std::cout << status_code << "\n";
        this->handle_end();
        return;
      }

      // Read the response headers, which are terminated by a blank line.
      boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
          boost::bind(&AsyncClient::handle_read_headers, this,
            boost::asio::placeholders::error));
    }
    else
    {
      //std::cout << "Error: " << err << "\n";
      this->handle_end();
    }
  }

  void handle_read_headers(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Process the response headers.
      std::istream response_stream(&response_);
      std::string header;
      while (std::getline(response_stream, header) && header != "\r")
        //std::cout << header << "\n"
          ;
      //std::cout << "\n";

      // Write whatever content we already have to output.
      if (response_.size() > 0) {
        std::ostringstream oss;
        oss << &response_;
        this->response_content_ += oss.str();
      }

      // Start reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&AsyncClient::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else
    {
      //std::cout << "Error: " << err << "\n";
      this->handle_end();
    }
  }

  void handle_read_content(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Write all of the data that has been read so far.
      std::ostringstream oss;
      oss << &response_;
      response_content_ += oss.str();

      // Continue reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&AsyncClient::handle_read_content, this,
            boost::asio::placeholders::error));
    } else if (err != boost::asio::error::eof) {
      //std::cout << "Error: " << err << "\n";
      this->handle_end();
    } else if (err == boost::asio::error::eof) {
      this->handle_end();
    }
  }

  tcp::resolver resolver_;
  tcp::socket socket_;
  boost::asio::streambuf request_;
  boost::asio::streambuf response_;
  std::string response_content_;
};

//int main(int argc, char* argv[])
//{
//  try
//  {
//    if (argc != 3)
//    {
//      std::cout << "Usage: async_AsyncClient <server> <path>\n";
//      std::cout << "Example:\n";
//      std::cout << "  async_AsyncClient www.boost.org /LICENSE_1_0.txt\n";
//      return 1;
//    }
//
//    boost::asio::io_service io_service;
//    AsyncClient c(io_service, argv[1], argv[2]);
//    io_service.run();
//  }
//  catch (std::exception& e)
//  {
//    std::cout << "Exception: " << e.what() << "\n";
//  }
//
//  return 0;
//}
