//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER2_SESSION_MANAGER_FWD_H
#define MA_ECHO_SERVER2_SESSION_MANAGER_FWD_H

#include <boost/smart_ptr.hpp>

namespace ma
{    
  namespace echo
  {
    namespace server2
    {    
      class session_manager;
      typedef boost::shared_ptr<session_manager> session_manager_ptr;
      typedef boost::weak_ptr<session_manager>   session_manager_weak_ptr;

    } // namespace server2
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER2_SESSION_MANAGER_FWD_H
