//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session.hpp>
#include <ma/echo/server/io_service_set.hpp>
#include <ma/echo/server/session_manager.hpp>

namespace ma
{    
  namespace echo
  {    
    namespace server
    {    
      session_manager::session_proxy::session_proxy(boost::asio::io_service& io_service,
        const session_config& holded_session_config)
        : state_(ready_to_start)
        , pending_operations_(0)
        , session_(boost::make_shared<session>(boost::ref(io_service), holded_session_config))
      {
      } // session_manager::session_proxy::session_proxy

      session_manager::session_proxy::~session_proxy()
      {
      } // session_manager::session_proxy::session_proxy

      session_manager::session_proxy_list::session_proxy_list()
        : size_(0)
      {
      } // session_manager::session_proxy_list::session_proxy_list

      void session_manager::session_proxy_list::push_front(const session_proxy_ptr& value)
      {
        value->next_ = front_;
        value->prev_.reset();
        if (front_)
        {
          front_->prev_ = value;
        }
        front_ = value;
        ++size_;
      } // session_manager::session_proxy_list::push_front

      void session_manager::session_proxy_list::erase(const session_proxy_ptr& value)
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
      } // session_manager::session_proxy_list::erase

      std::size_t session_manager::session_proxy_list::size() const
      {
        return size_;
      } // session_manager::session_proxy_list::size

      bool session_manager::session_proxy_list::empty() const
      {
        return 0 == size_;
      } // session_manager::session_proxy_list::empty

      session_manager::session_proxy_ptr session_manager::session_proxy_list::front() const
      {
        return front_;
      } // session_manager::session_proxy_list::front      
        
      session_manager::session_manager(io_service_set& io_services, 
        const session_manager_config& config)
        : accept_in_progress_(false)
        , state_(ready_to_start)
        , pending_operations_(0)
        , io_service_(io_services.session_manager_io_service())
        , session_io_service_(io_services.session_io_service())
        , strand_(io_services.session_manager_io_service())
        , acceptor_(io_services.session_manager_io_service())        
        , wait_handler_(io_services.session_manager_io_service())
        , stop_handler_(io_services.session_manager_io_service())
        , config_(config)
        
      {          
      } // session_manager::session_manager

      session_manager::~session_manager()
      {        
      } // session_manager::session_manager  

      void session_manager::start(boost::system::error_code& error)
      {
        if (stopped == state_ || stop_in_progress == state_)
        {          
          error = boost::asio::error::operation_aborted;
        } 
        else if (ready_to_start != state_)
        {          
          error = boost::asio::error::operation_not_supported;                      
        }
        else
        {
          state_ = start_in_progress;        
          acceptor_.open(config_.endpoint_.protocol(), error);
          if (!error)
          {
            acceptor_.bind(config_.endpoint_, error);
            if (!error)
            {
              acceptor_.listen(config_.listen_backlog_, error);
            }          
          }          
          if (error)
          {
            boost::system::error_code ignored;
            acceptor_.close(ignored);
            state_ = stopped;
          }
          else
          {            
            accept_new_session();            
            state_ = started;
          }
        }        
      } // session_manager::start

      void session_manager::stop(boost::system::error_code& error, bool& completed)
      {
        completed = true;
        if (stopped == state_ || stop_in_progress == state_)
        {          
          error = boost::asio::error::operation_aborted;
        }
        else
        {
          // Start shutdown
          state_ = stop_in_progress;
          // Do shutdown - abort inner operations          
          acceptor_.close(stop_error_); 
          // Stop all active sessions
          session_proxy_ptr curr_session_proxy(active_session_proxies_.front());
          while (curr_session_proxy)
          {
            if (stop_in_progress != curr_session_proxy->state_)
            {
              stop_session(curr_session_proxy);
            }
            curr_session_proxy = curr_session_proxy->next_;
          }
          // Do shutdown - abort outer operations
          if (wait_handler_.has_target())
          {
            wait_handler_.post(boost::asio::error::operation_aborted);
          }            
          // Check for shutdown continuation
          if (may_complete_stop())
          {
            complete_stop();
            error = stop_error_;
            completed = true;              
          }
          else
          { 
            completed = false;
          }
        }
      } // session_manager::stop

      void session_manager::wait(boost::system::error_code& error, bool& completed)
      {
        completed = true;
        if (stopped == state_ || stop_in_progress == state_)
        {          
          error = boost::asio::error::operation_aborted;
        } 
        else if (started != state_)
        {
          error = boost::asio::error::operation_not_supported;
        }
        else if (last_accept_error_ && active_session_proxies_.empty())
        {
          error = last_accept_error_;
        }
        else if (wait_handler_.has_target())
        {
          error = boost::asio::error::operation_not_supported;
        }
        else
        {          
          completed = false;
        } 
      } // session_manager::wait

      void session_manager::accept_new_session()
      {
        // Get new ready to start session
        session_proxy_ptr proxy;
        if (recycled_session_proxies_.empty())
        {
          proxy = boost::make_shared<session_proxy>(
            boost::ref(session_io_service_), config_.session_config_);
        }
        else
        {
          proxy = recycled_session_proxies_.front();
          recycled_session_proxies_.erase(proxy);
        }
        // Start session acceptation
        acceptor_.async_accept(proxy->session_->socket(), proxy->endpoint_, 
          strand_.wrap(make_custom_alloc_handler(accept_allocator_,
            boost::bind(&this_type::handle_accept, shared_from_this(), proxy, boost::asio::placeholders::error))));
        ++pending_operations_;
        accept_in_progress_ = true;
      } // session_manager::accept_new_session

      void session_manager::handle_accept(const session_proxy_ptr& proxy, 
        const boost::system::error_code& error)
      {
        --pending_operations_;
        accept_in_progress_ = false;
        if (stop_in_progress == state_)
        {
          if (may_complete_stop())  
          {
            complete_stop();
            // Signal shutdown completion
            stop_handler_.post(stop_error_);
          }
        }
        else if (error)
        {   
          last_accept_error_ = error;
          if (active_session_proxies_.empty()) 
          {
            // Server can't work more time
            wait_handler_.post(error);
          }
        }
        else if (active_session_proxies_.size() < config_.max_sessions_)
        { 
          // Start accepted session 
          start_session(proxy);          
          // Save session as active
          active_session_proxies_.push_front(proxy);
          // Continue session acceptation if can
          if (active_session_proxies_.size() < config_.max_sessions_)
          {
            accept_new_session();
          }          
        }
        else
        {
          recycle_session(proxy);
        }
      } // handle_accept        

      bool session_manager::may_complete_stop() const
      {
        return 0 == pending_operations_ && active_session_proxies_.empty();
      } // session_manager::may_complete_stop

      void session_manager::complete_stop()
      {
        state_ = stopped;  
      } // session_manager::complete_stop

      void session_manager::start_session(const session_proxy_ptr& accepted_session_proxy)
      {        
        accepted_session_proxy->session_->async_start(make_custom_alloc_handler(accepted_session_proxy->start_wait_allocator_, 
          boost::bind(&this_type::dispatch_session_start, session_manager_weak_ptr(shared_from_this()), accepted_session_proxy, _1)));  
        ++pending_operations_;
        ++accepted_session_proxy->pending_operations_;
        accepted_session_proxy->state_ = session_proxy::start_in_progress;
      } // session_manager::start_session

      void session_manager::stop_session(const session_proxy_ptr& proxy)
      {
        proxy->session_->async_stop(make_custom_alloc_handler(proxy->stop_allocator_,
          boost::bind(&this_type::dispatch_session_stop, session_manager_weak_ptr(shared_from_this()), proxy, _1)));
        ++pending_operations_;
        ++proxy->pending_operations_;
        proxy->state_ = session_proxy::stop_in_progress;
      } // stop_session

      void session_manager::wait_session(const session_proxy_ptr& proxy)
      {
        proxy->session_->async_wait(make_custom_alloc_handler(proxy->start_wait_allocator_,
          boost::bind(&this_type::dispatch_session_wait, session_manager_weak_ptr(shared_from_this()), proxy, _1)));
        ++pending_operations_;
        ++proxy->pending_operations_;
      } // session_manager::wait_session

      void session_manager::dispatch_session_start(const session_manager_weak_ptr& weak_this_ptr,
        const session_proxy_ptr& proxy, const boost::system::error_code& error)
      {
        if (session_manager_ptr this_ptr = weak_this_ptr.lock())
        {
          this_ptr->strand_.dispatch(make_custom_alloc_handler(proxy->start_wait_allocator_,
            boost::bind(&this_type::handle_session_start, this_ptr, proxy, error)));
        }
      } // session_manager::dispatch_session_start

      void session_manager::handle_session_start(const session_proxy_ptr& proxy, 
        const boost::system::error_code& error)
      {
        --pending_operations_;
        --proxy->pending_operations_;
        if (session_proxy::start_in_progress == proxy->state_)
        {          
          if (error)
          {
            proxy->state_ = session_proxy::stopped;
            active_session_proxies_.erase(proxy);            
            if (stop_in_progress == state_)
            {
              if (may_complete_stop())  
              {
                complete_stop();
                // Signal shutdown completion
                stop_handler_.post(stop_error_);
              }
            }
            else if (last_accept_error_ && active_session_proxies_.empty()) 
            {
              wait_handler_.post(last_accept_error_);
            }
            else
            {
              recycle_session(proxy);
              // Continue session acceptation if can
              if (!accept_in_progress_ && !last_accept_error_
                && active_session_proxies_.size() < config_.max_sessions_)
              {                
                accept_new_session();
              }                
            }
          }
          else // !error
          {
            proxy->state_ = session_proxy::started;
            if (stop_in_progress == state_)  
            {                            
              stop_session(proxy);
            }
            else
            { 
              // Wait until session needs to stop
              wait_session(proxy);
            }            
          }  
        } 
        else if (stop_in_progress == state_) 
        {
          if (may_complete_stop())  
          {
            complete_stop();
            // Signal shutdown completion
            stop_handler_.post(stop_error_);
          }
        }
        else
        {
          recycle_session(proxy);
        }        
      } // session_manager::handle_session_start

      void session_manager::dispatch_session_wait(const session_manager_weak_ptr& weak_this_ptr,
        const session_proxy_ptr& proxy, const boost::system::error_code& error)
      {          
        if (session_manager_ptr this_ptr = weak_this_ptr.lock())
        {
          this_ptr->strand_.dispatch(make_custom_alloc_handler(proxy->start_wait_allocator_,
            boost::bind(&this_type::handle_session_wait, this_ptr, proxy, error)));
        }
      } // session_manager::dispatch_session_wait

      void session_manager::handle_session_wait(const session_proxy_ptr& proxy, 
        const boost::system::error_code& /*error*/)
      {
        --pending_operations_;
        --proxy->pending_operations_;
        if (session_proxy::started == proxy->state_)
        {
          stop_session(proxy);
        }
        else if (stop_in_progress == state_)
        {
          if (may_complete_stop())  
          {
            complete_stop();
            // Signal shutdown completion
            stop_handler_.post(stop_error_);
          }
        }
        else
        {
          recycle_session(proxy);
        }
      } // session_manager::handle_session_wait

      void session_manager::dispatch_session_stop(const session_manager_weak_ptr& weak_this_ptr,
        const session_proxy_ptr& proxy, const boost::system::error_code& error)
      {          
        if (session_manager_ptr this_ptr = weak_this_ptr.lock())
        {
          this_ptr->strand_.dispatch(make_custom_alloc_handler(proxy->stop_allocator_,
            boost::bind(&this_type::handle_session_stop, this_ptr, proxy, error)));
        }
      } // session_manager::dispatch_session_stop

      void session_manager::handle_session_stop(const session_proxy_ptr& proxy,
        const boost::system::error_code& /*error*/)
      {
        --pending_operations_;
        --proxy->pending_operations_;
        if (session_proxy::stop_in_progress == proxy->state_)        
        {
          proxy->state_ = session_proxy::stopped;
          active_session_proxies_.erase(proxy);            
          if (stop_in_progress == state_)
          {
            if (may_complete_stop())  
            {
              complete_stop();
              // Signal shutdown completion
              stop_handler_.post(stop_error_);
            }
          }
          else if (last_accept_error_ && active_session_proxies_.empty()) 
          {
            wait_handler_.post(last_accept_error_);
          }
          else
          {
            recycle_session(proxy);
            // Continue session acceptation if can
            if (!accept_in_progress_ && !last_accept_error_
              && active_session_proxies_.size() < config_.max_sessions_)
            {                
              accept_new_session();
            }              
          }
        }
        else if (stop_in_progress == state_)
        {
          if (may_complete_stop())  
          {
            complete_stop();
            // Signal shutdown completion
            stop_handler_.post(stop_error_);
          }
        }
        else
        {
          recycle_session(proxy);
        }
      } // session_manager::handle_session_stop

      void session_manager::recycle_session(const session_proxy_ptr& proxy)
      {
        if (0 == proxy->pending_operations_
          && recycled_session_proxies_.size() < config_.recycled_sessions_)
        {
          proxy->session_->reset();
          proxy->state_ = session_proxy::ready_to_start;
          recycled_session_proxies_.push_front(proxy);
        }        
      } // session_manager::recycle_session        

    } // namespace server
  } // namespace echo
} // namespace ma

