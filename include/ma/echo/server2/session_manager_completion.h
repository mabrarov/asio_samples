//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER2_SESSION_MANAGER_COMPLETION_H
#define MA_ECHO_SERVER2_SESSION_MANAGER_COMPLETION_H

#include <boost/system/error_code.hpp>
#include <boost/call_traits.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/echo/server2/session_manager_proxy_fwd.h>

namespace ma
{    
  namespace echo
  {
    namespace server2
    {             
      namespace session_manager_completion
      {        
        class raw_handler
        {
        private:
          typedef raw_handler this_type;
          this_type& operator=(const this_type&);        

        public:                
          typedef void (*func_type)(const session_manager_proxy_ptr&, const boost::system::error_code&);

          explicit raw_handler(func_type func,            
            const session_manager_proxy_ptr& session_manager_proxy)
            : func_(func)            
            , session_manager_proxy_(session_manager_proxy)
          {
          }

          void operator()(const boost::system::error_code& error)
          {
            func_(session_manager_proxy_, error);
          }

        private:
          func_type func_;          
          session_manager_proxy_ptr session_manager_proxy_;        
        }; // class raw_handler

        typedef in_place_handler_allocator<256> handler_allocator;
        typedef custom_alloc_handler<handler_allocator, raw_handler> handler;

        inline handler make_handler(handler_allocator& allocator, raw_handler::func_type func, 
          const session_manager_proxy_ptr& session_manager_proxy)
        {
          return make_custom_alloc_handler(allocator, raw_handler(func, session_manager_proxy));
        }

      } // namespace session_manager_completion
    } // namespace server2
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER2_SESSION_MANAGER_COMPLETION_H
