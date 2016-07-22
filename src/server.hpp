//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <queue>

#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/atomic/atomic.hpp>
#include "connection.hpp"
#include "request_handler.hpp"

namespace http {
namespace server {

/// The top-level class of the HTTP server.
class server
  : private boost::noncopyable
{
public:
  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
     explicit server( const std::string& address, const std::string& port, const std::string& doc_root);

  /// Run the server's io_service loop.
  void run();

  /// Handle a request to stop the server.
  void stop();

private:

  void worker_func();

  /// The io_service used to perform asynchronous operations.
  boost::asio::io_service io_service_;

  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  mutable boost::mutex connectionsMutex_;
  boost::condition_variable connectionsCondition;
  std::queue< connection_ptr > newConnections_;

  /// The handler for all incoming requests.
  request_handler request_handler_;

  boost::atomic< bool > isStopped_;

  std::string address_;
  std::string port_;

  std::vector< boost::shared_ptr< boost::thread > > workerThreads_;
};

} // namespace server
} // namespace http

#endif // HTTP_SERVER_HPP
