//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_FWD_HPP
#define MA_ECHO_SERVER_SESSION_FWD_HPP

#include <boost/smart_ptr.hpp>

namespace ma
{    
  namespace echo
  {
    namespace server
    {    
      class session;
      typedef boost::shared_ptr<session> session_ptr;

    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_FWD_HPP
