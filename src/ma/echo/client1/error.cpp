//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/echo/client1/error.hpp>

namespace ma
{    
  namespace echo
  {    
    namespace client1
    {   
      const client1_error_category_impl client1_error_category_instance;
       
      const boost::system::error_category& client1_error_category()
      {
        return client1_error_category_instance;
      }
    } // namespace server
  } // namespace echo
} // namespace ma

