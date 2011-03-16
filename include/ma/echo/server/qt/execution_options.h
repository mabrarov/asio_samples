//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_QT_EXECUTION_OPTIONS_HPP
#define MA_ECHO_SERVER_QT_EXECUTION_OPTIONS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>
#include <boost/assert.hpp>
#include <ma/echo/server/qt/execution_options_fwd.h>

namespace ma
{    
  namespace echo
  {
    namespace server
    {    
      namespace qt 
      {
        class execution_options
        {
        public:          
          execution_options(std::size_t session_manager_thread_count,
            std::size_t session_thread_count)
            : session_manager_thread_count_(session_manager_thread_count)
            , session_thread_count_(session_thread_count)            
          {
            BOOST_ASSERT_MSG(session_manager_thread_count > 0, "session_manager_thread_count must be > 0");
            BOOST_ASSERT_MSG(session_thread_count > 0, "session_thread_count must be > 0");
          }

          std::size_t session_manager_thread_count() const
          {
            return session_manager_thread_count_;
          }

          std::size_t session_thread_count() const
          {
            return session_thread_count_;
          }

        private:
          std::size_t session_manager_thread_count_;
          std::size_t session_thread_count_;
        }; // struct execution_options
        
      } // namespace qt
    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_QT_EXECUTION_OPTIONS_HPP
