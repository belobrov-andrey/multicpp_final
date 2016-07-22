//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//



#include "server.hpp"

#include <algorithm>

#include <boost/asio/ip/tcp.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <iostream>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
//typedef InetPton inet_pton;
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef int SOCKET;

#endif



#include <signal.h>


namespace http {
namespace server {


server::server( const std::string& address, const std::string& port,
    const std::string& doc_root)
    : io_service_(),
    acceptor_( io_service_ ),
    request_handler_( doc_root ),
    address_( address ),
    port_( port  ),
    isStopped_( false )
{
}

void server::run()
{
  // The io_service::run() call will block until all asynchronous operations
  // have finished. While the server is running, there is always at least one
  // asynchronous operation outstanding: the asynchronous accept call waiting
  // for new incoming connections.
  //io_service_.run();

     // Start working threads

     //boost::lock_guard<boost::mutex> lk( workerThreadsMutex_ );
     // matching function for call to 'accept'
     //
               
     SOCKET listeningSocket = socket( AF_INET, SOCK_STREAM, 0 );
     if( listeningSocket == 0 )
     {
          std::cerr << "Unable to create listening socket." << std::endl;
          return;
     }

     sockaddr_in serv_addr;
     serv_addr.sin_family = AF_INET;

     int portNum = atoi( port_.c_str() );

     if( !portNum )
     {
         
          std::cerr << "Unable to get portnum." << port_ <<  portNum << std::endl;
          return;
     }

     serv_addr.sin_port = htons( portNum );
     long adddrLong = inet_addr( address_.c_str() );
     memcpy( &serv_addr.sin_addr, &adddrLong, sizeof( adddrLong ) );
     memset( &serv_addr.sin_zero, 0, sizeof( serv_addr.sin_zero ) );

     int enableReuseAddress = 1;
     if( setsockopt( listeningSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&enableReuseAddress,
          sizeof( enableReuseAddress ) ) != 0 )
     {
          std::cerr << "Unable to set socket REUSEADDR option." << std::endl;
          return;
     }
     
     int err = bind( listeningSocket, (sockaddr*)&serv_addr, sizeof( serv_addr ) );
     if( err != 0 )
     {
          std::cerr << "Bind ERROR" << std::endl;
          return;
     }

     if( listen( listeningSocket, SOMAXCONN ) != 0 )
     {
          std::cerr << "Listen ERROR" << std::endl;
          return;
     }

             
     // создание клиентской sockaddr_in где есть IP & Port клиента


     unsigned int hardwareConcurrency = boost::thread::hardware_concurrency();
     unsigned int threadsNumber = std::max( 2u, hardwareConcurrency );     for( int i = 0; i < threadsNumber; i++ )
     {
          workerThreads_.push_back( boost::make_shared< boost::thread >(
               boost::bind( &server::worker_func, this ) ) );
     }
          fd_set allreads;
     fd_set readmask;
     FD_ZERO( &allreads );
     FD_SET( listeningSocket, &allreads );     struct timeval tv;     tv.tv_sec = 0;     tv.tv_usec = 100 * 1000; //  100 ms
     while( !isStopped_ )
     {
          readmask = allreads;
          int rc = select( listeningSocket + 1, &readmask, NULL, NULL, &tv );
          if( rc < 0 )
          {
               break; // Some error on select
          }
          else if( rc == 0 ) // Timeout
          {
               continue;
          }
          

          if( FD_ISSET( listeningSocket, &readmask ) )
          {
               sockaddr_in client_addr;
               socklen_t size_client_addr = sizeof( client_addr );

               SOCKET client_sock;
               
               client_sock = accept( listeningSocket, (struct sockaddr *)&client_addr, &size_client_addr );

               if( !client_sock )
               {
                    break;
               }

               connection_ptr newConnection_( new connection( io_service_, request_handler_ ) );

               boost::system::error_code ec;
               newConnection_->socket().assign( boost::asio::ip::tcp::v4(), client_sock, ec );

               if( ec != boost::system::errc::success )
               {
                    break;
               }

               boost::lock_guard< boost::mutex > lk( connectionsMutex_ );
               newConnections_.push( newConnection_ );
               connectionsCondition.notify_one();      
          } 
          else
          {
                break;
          }
     }

     boost::asio::ip::tcp::acceptor a( io_service_, boost::asio::ip::tcp::v4(), listeningSocket );
     a.close();

     connectionsCondition.notify_all();


     for( size_t i = 0; i < workerThreads_.size(); ++i )
     {
          const int gentleExitWaitTime = 5; // sec
          if( !workerThreads_[i]->timed_join( boost::posix_time::seconds( gentleExitWaitTime ) ) )
          {
               std::cerr << "Unable to wait until client session ends." << std::endl;
          }
     }

}

//void server::start_accept()
//{
//  connection_ptr newConnection_( new connection( io_service_,
//        connection_manager_, request_handler_) ) ;
//
//  boost::system::error_code ec;
//
//  acceptor_.accept( newConnection_->socket( ), ec );
//
//  //handle_accept( ec );
//}

//void server::handle_accept(const boost::system::error_code& e)
//{
//  // Check whether the server was stopped by a signal before this completion
//  // handler had a chance to run.
//  if (!acceptor_.is_open())
//  {
//    return;
//  }
//
//  if (!e)
//  {
//    //connection_manager_.start(new_connection_);
//  }
//
//  start_accept();
//}

void server::stop()
{
  isStopped_ = true;
}

void server::worker_func()
{
     while( !isStopped_ )
     {
          boost::unique_lock<boost::mutex> lk( connectionsMutex_ );

          while( newConnections_.empty( ) && !isStopped_ )
          {
               connectionsCondition.wait( lk );
          }

          if( !isStopped_ )
          {
               connection_ptr newConnection = newConnections_.front( );
               newConnections_.pop();

               lk.unlock();
              
               newConnection->process();
          }
     }

}

} // namespace server
} // namespace http
