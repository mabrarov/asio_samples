//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_ERROR_HPP
#define MA_ECHO_SERVER_ERROR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/system/error_code.hpp>
#include <boost/type_traits.hpp>

namespace ma {

namespace echo {

namespace server {

/**
 * See 
 * http://blog.think-async.com/2010/04/system-error-support-in-c0x-part-4.html
 */
const boost::system::error_category& server_error_category();

namespace server_error {

enum error_t 
{
  invalid_state         = 100,
  operation_aborted     = 200,
  inactivity_timeout    = 300,
  no_memory_for_session = 400
}; // enum error_t 

inline boost::system::error_code make_error_code(error_t e)
{  
  return boost::system::error_code(static_cast<int>(e), 
      server_error_category());
}

inline boost::system::error_condition make_error_condition(error_t e)
{  
  return boost::system::error_condition(static_cast<int>(e), 
      server_error_category());
}

} // namespace server_error
      
class server_error_category_impl : public boost::system::error_category
{
public:
  virtual const char* name() const
  {
    return "ma::echo::server";
  }

  virtual std::string message(int ev) const
  {
    switch (ev)
    {
    case server_error::invalid_state:
      return "Invalid state.";
    case server_error::operation_aborted:
      return "Operation aborted.";
    case server_error::inactivity_timeout:
      return "Inactivity timeout.";
    case server_error::no_memory_for_session:
      return "No memory for session.";
    default:
      return "Unknown ma::echo::server error.";
    }
  }

}; // class server_error_category_impl
            
} // namespace server
} // namespace echo
} // namespace ma

namespace boost {  

namespace system {

template <>  
struct is_error_code_enum<ma::echo::server::server_error::error_t> 
  : public boost::true_type 
{
}; // struct is_error_code_enum

} // namespace system
} // namespace boost
  
#endif // MA_ECHO_SERVER_ERROR_HPP
