//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#endif

#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <ma/async_connect.hpp>

namespace {

void handle_connect(const boost::system::error_code& /*error*/)
{
  //todo
}

} // anonymous namespace

#if defined(WIN32)
int _tmain(int /*argc*/, _TCHAR* /*argv*/[])
#else
int main(int /*argc*/, char* /*argv*/[])
#endif
{     
  try
  {  
    //todo
    boost::asio::io_service io_service;


    boost::asio::ip::address peer_address(boost::asio::ip::address_v4::loopback());
    boost::asio::ip::tcp::endpoint peer_endpoint(peer_address, 7);

    boost::asio::ip::tcp::socket socket(io_service);

    ma::async_connect(socket, peer_endpoint, &handle_connect);

    io_service.run();

    return EXIT_SUCCESS;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
  }
  return EXIT_FAILURE;
}
