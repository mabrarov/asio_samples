//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
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
#include <ma/echo/client1/session.hpp>
#include <ma/console_controller.hpp>

int _tmain(int argc, _TCHAR* argv[])
{   
  try 
  {
    boost::program_options::options_description options_description("Allowed options");
    options_description.add_options()
      (
        "help", 
        "produce help message"
      );

    boost::program_options::variables_map options_values;  
    boost::program_options::store(
      boost::program_options::parse_command_line(argc, argv, options_description), 
      options_values);
    boost::program_options::notify(options_values);

    if (options_values.count("help"))
    {
      std::cout << options_description;
      return EXIT_SUCCESS;
    }
    
    //std::size_t cpu_num = boost::thread::hardware_concurrency();
    //std::size_t session_thread_num = cpu_num > 1 ? cpu_num : 2;
    //todo
    return EXIT_SUCCESS;
  }
  catch (const boost::program_options::error& e)
  {    
    std::cerr << "Error reading options: " << e.what() << std::endl;      
    return EXIT_FAILURE;
  }
  catch (const std::exception& e)
  {    
    std::cerr << "Unexpected exception: " << e.what() << std::endl;      
    return EXIT_FAILURE;
  }
}