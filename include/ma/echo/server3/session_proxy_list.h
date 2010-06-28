//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER3_SESSION_PROXY_LIST_H
#define MA_ECHO_SERVER3_SESSION_PROXY_LIST_H

#include <boost/smart_ptr.hpp>
#include <ma/echo/server3/session_proxy_fwd.h>

namespace ma
{    
  namespace echo
  {    
    namespace server3
    {                               
      class session_proxy_list : private boost::noncopyable
      {
      public:
        explicit session_proxy_list();
        ~session_proxy_list();

        void push_front(const session_proxy_ptr& value);
        void erase(const session_proxy_ptr& value);
        std::size_t size() const;
        bool empty() const;
        session_proxy_ptr front() const;

      private:
        std::size_t size_;
        session_proxy_ptr front_;
      }; // session_proxy_list

    } // namespace server3
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER3_SESSION_PROXY_LIST_H
