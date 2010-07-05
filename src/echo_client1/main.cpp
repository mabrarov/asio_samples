//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <boost/smart_ptr.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/echo/client1/session.h>
#include <ma/console_controller.hpp>

int _tmain(int argc, _TCHAR* argv[])
{ 
  boost::program_options::options_description options_description("Allowed options");
  options_description.add_options()
    (
      "help", 
      "produce help message"
    );

  int exit_code = EXIT_SUCCESS;
  try 
  {
    boost::program_options::variables_map options_values;  
    boost::program_options::store(
      boost::program_options::parse_command_line(argc, argv, options_description), 
      options_values);
    boost::program_options::notify(options_values);

    if (options_values.count("help"))
    {
      std::cout << options_description;
    }
    else
    {
      std::size_t cpu_count = boost::thread::hardware_concurrency();
      std::size_t session_thread_count = cpu_count > 1 ? cpu_count : 2;
      //todo
    }
  }
  catch (const boost::program_options::error&)
  {
    exit_code = EXIT_FAILURE;
    std::cout << "Invalid options.\n" << options_description;      
  }  
  return exit_code;
}