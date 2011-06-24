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
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <ma/async_connect.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>

namespace {

const std::string text_to_write = "Hello, World!";

void handle_connect(boost::asio::ip::tcp::socket& socket, 
    const boost::system::error_code& error);

void handle_write(const boost::system::error_code& error, 
    std::size_t bytes_transferred);

void handle_connect(boost::asio::ip::tcp::socket& socket, 
    const boost::system::error_code& error)
{
  if (error)
  {
    std::cout << "async_connect completed with error: " 
        << error.message() << std::endl;
    return;
  }
  std::cout << "async_connect completed with success" << std::endl;  
  socket.async_write_some(boost::asio::buffer(text_to_write), 
      boost::bind(&handle_write, boost::asio::placeholders::error, 
          boost::asio::placeholders::bytes_transferred));
}

void handle_write(const boost::system::error_code& error, 
    std::size_t /*bytes_transferred*/)
{
  if (error)
  {
    std::cout << "socket.async_write_some completed with error: " 
        << error.message() << std::endl;
    return;
  }
  std::cout << "socket.async_write_some completed with success" << std::endl;
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
    ma::in_place_handler_allocator<512> handler_allocator;

    boost::asio::io_service io_service;

    boost::asio::ip::address peer_address(
        boost::asio::ip::address_v4::loopback());
    boost::asio::ip::tcp::endpoint peer_endpoint(peer_address, 7);

    boost::asio::ip::tcp::socket socket(io_service);

    ma::async_connect(socket, peer_endpoint, 
        ma::make_custom_alloc_handler(handler_allocator, 
            boost::bind(&handle_connect, boost::ref(socket), 
                boost::asio::placeholders::error)));

    io_service.run();

    return EXIT_SUCCESS;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected error: " << e.what() << std::endl;
  }
  return EXIT_FAILURE;
}
