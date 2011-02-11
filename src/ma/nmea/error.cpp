//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/nmea/error.hpp>

namespace ma
{    
  namespace nmea
  {    
    const session_error_category_impl session_error_category_instance;
       
    const boost::system::error_category& session_error_category()
    {
      return session_error_category_instance;
    }    
  } // namespace nmea
} // namespace ma

