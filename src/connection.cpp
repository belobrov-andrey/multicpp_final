//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection.hpp"
#include <vector>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include "request_handler.hpp"

namespace http {
namespace server {

connection::connection(boost::asio::io_service& io_service, request_handler& handler)
  : socket_(io_service),
    request_handler_(handler)
{
}

boost::asio::ip::tcp::socket& connection::socket()
{
  return socket_;
}

void connection::process()
{
     boost::system::error_code errorCode;
     bool parseResult = false;
     read( socket_, boost::asio::buffer( buffer_ ),
          boost::bind( &connection::readCompletion, this, boost::ref( parseResult ), _1, _2 ), errorCode );

     if( !errorCode )
     {
          boost::system::error_code ignored_ec;
          if( parseResult )
          {
               request_handler_.handle_request( request_, reply_);

               boost::asio::write( socket_, reply_.to_buffers( ), ignored_ec );
          }
          else
          {
               reply_ = reply::stock_reply( reply::bad_request );
               boost::asio::write( socket_, reply_.to_buffers( ), ignored_ec );
          }
     }

     stop();
}

void connection::stop()
{
     // Initiate graceful connection closure.
  boost::system::error_code ignored_ec;
  socket_.shutdown( boost::asio::ip::tcp::socket::shutdown_both, ignored_ec );
  socket_.close();
}

std::size_t connection::readCompletion( bool &parseResult, const boost::system::error_code& error,
     std::size_t bytes_transferred )
{
     if( !error )
     {
          boost::tribool result;
          boost::tie( result, boost::tuples::ignore ) = request_parser_.parse(
               request_, buffer_.data(), buffer_.data() + bytes_transferred );

          if( result )
          {
               parseResult = true;
               return 0;
          }
          else if( !result )
          {
               parseResult = false;
               return 0;
          }
          else
          {
               // Read no finished
               return buffer_.size() - bytes_transferred;
          }
     }
     else
     {
          return 0;
     }

}

connection::~connection()
{
     
}

} // namespace server
} // namespace http
