//
// main.cpp
// ~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/function/function0.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include "server.hpp"

#ifndef _WIN32
#include "become_daemon.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#endif

namespace po = boost::program_options;

#if defined(_WIN32)

boost::function0<void> console_ctrl_function;

BOOL WINAPI console_ctrl_handler( DWORD ctrl_type )
{
     switch( ctrl_type )
     {
     case CTRL_C_EVENT:
     case CTRL_BREAK_EVENT:
     case CTRL_CLOSE_EVENT:
     case CTRL_SHUTDOWN_EVENT:
          console_ctrl_function();
          return TRUE;
     default:
          return FALSE;
     }
}
#endif // defined(_WIN32)

static const char *optString = "h:p:d:";

int main(int argc, char* argv[])
{
    std::string hostAddress;
    std::string hostPort;
    std::string rootDirectory;
    bool demonize = true;

#ifdef _WIN32        
    
    po::options_description desc( "Allowed options" );
    desc.add_options()
         ("host,h", po::value<std::string>( &hostAddress )->default_value( "127.0.0.1" ), "Host address")
         ("port,p", po::value<std::string>( &hostPort )->default_value( "8080" ), "Host port")
         ("directory,d", po::value<std::string>( &rootDirectory ), "Root directory")
         ("daemonize", po::value< bool >( &demonize )->default_value( true ), "Run as daemon");

    po::variables_map vm;
    po::parsed_options parsed = po::command_line_parser( argc, argv ).options( desc ).run();
    po::store( parsed, vm );
    po::notify( vm );

    if( !vm.count( "directory" ) )
    {
         std::cout << "Root directory not set." << std::endl;
         std::cout << desc << "\n";
         return 1;
    }
#else

    int opt;

    opt = getopt( argc, argv, optString  );

    while( opt != -1 )
    {
        switch (opt )
        {
            case 'h':
                hostAddress = optarg;
                break;

            case 'p':
                hostPort = optarg;
                break;
            case 'd':
                rootDirectory = optarg;
                break;
            default:
                break;
        }

        opt = getopt( argc, argv, optString  );
    }
#endif

#if defined( _WIN32 )
         
    // Initialise the server.
    http::server::server s( hostAddress, hostPort, rootDirectory );

    // Set console control handler to allow server to be stopped.
    console_ctrl_function = boost::bind( &http::server::server::stop, &s );

    SetConsoleCtrlHandler( console_ctrl_handler, TRUE );

    // Run the server until stopped.
    s.run();
#else

if ( demonize )
{    
    becomeDaemon( BD_NO_CHDIR | BD_NO_CLOSE_FILES | BD_NO_REOPEN_STD_FDS ); 
}
    
sigset_t new_mask;
sigfillset(&new_mask);
sigset_t old_mask;
pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

// Run server in background thread.
 http::server::server s( hostAddress, hostPort, rootDirectory );
 boost::thread t(boost::bind(&http::server::server::run, &s));
//
// // Restore previous signals.
 pthread_sigmask(SIG_SETMASK, &old_mask, 0);
//
// // Wait for signal indicating time to shut down.
sigset_t wait_mask;
sigemptyset(&wait_mask);
sigaddset(&wait_mask, SIGINT);
sigaddset(&wait_mask, SIGQUIT);
sigaddset(&wait_mask, SIGTERM);
pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
int sig = 0;
sigwait(&wait_mask, &sig);
//
// // Stop the server.
s.stop();
t.join();
#endif

  return 0;
}
