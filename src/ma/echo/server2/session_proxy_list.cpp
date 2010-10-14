//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ma/echo/server2/session_proxy.h>
#include <ma/echo/server2/session_proxy_list.h>

namespace ma
{    
  namespace echo
  {
    namespace server2
    {      
      session_proxy_list::session_proxy_list()
        : size_(0)
      {
      } // session_proxy_list::session_proxy_list      

      void session_proxy_list::push_front(const session_proxy_ptr& value)
      {
        value->next_ = front_;
        value->prev_.reset();
        if (front_)
        {
          front_->prev_ = value;
        }
        front_ = value;
        ++size_;
      } // session_proxy_list::push_front

      void session_proxy_list::erase(const session_proxy_ptr& value)
      {
        if (front_ == value)
        {
          front_ = front_->next_;
        }
        session_proxy_ptr prev = value->prev_.lock();
        if (prev)
        {
          prev->next_ = value->next_;
        }
        if (value->next_)
        {
          value->next_->prev_ = prev;
        }
        value->prev_.reset();
        value->next_.reset();
        --size_;
      } // session_proxy_list::erase      
      
    } // namespace server2
  } // namespace echo
} // namespace ma