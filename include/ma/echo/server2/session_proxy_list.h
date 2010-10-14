//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER2_SESSION_PROXY_LIST_H
#define MA_ECHO_SERVER2_SESSION_PROXY_LIST_H

#include <ma/echo/server2/session_proxy_fwd.h>

namespace ma
{    
  namespace echo
  {    
    namespace server2
    {                               
      class session_proxy_list : private boost::noncopyable
      {
      public:
        explicit session_proxy_list();

        ~session_proxy_list()
        {
        } // ~session_proxy_list

        void push_front(const session_proxy_ptr& value);
        void erase(const session_proxy_ptr& value);

        std::size_t size() const
        {
          return size_;
        } // size

        bool empty() const
        {
          return 0 == size_;
        } // empty

        session_proxy_ptr front() const
        {
          return front_;
        } // front

      private:
        std::size_t size_;
        session_proxy_ptr front_;
      }; // session_proxy_list

    } // namespace server2
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER2_SESSION_PROXY_LIST_H
