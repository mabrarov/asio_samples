//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SESSION_MANAGER_HPP
#define MA_ECHO_SESSION_MANAGER_HPP

#include <boost/utility.hpp>
#include <boost/asio.hpp>

namespace ma
{    
  namespace echo
  {
    class session_manager : private boost::noncopyable 
    {
    public:
      explicit session_manager(boost::asio::io_service& io_service)
        : io_service_(io_service)
      {
        //todo
      }

    private:
      boost::asio::io_service& io_service_;

    }; // class session_factory

  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SESSION_MANAGER_HPP
