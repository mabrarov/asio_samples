//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <ma/echo/server/qt/execution_config.h>

namespace ma
{    
namespace echo
{
namespace server
{    
namespace qt 
{    
  execution_config::execution_config(
    std::size_t the_session_manager_thread_count, 
    std::size_t the_session_thread_count)
    : session_manager_thread_count(the_session_manager_thread_count)
    , session_thread_count(the_session_thread_count)    
  {
    if (the_session_manager_thread_count < 1)
    {
      boost::throw_exception(std::invalid_argument("the_session_manager_thread_count must be >= 1"));
    }
    if (the_session_thread_count < 1)
    {
      boost::throw_exception(std::invalid_argument("the_session_thread_count must be >= 1"));
    }    
  }
              
} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma

