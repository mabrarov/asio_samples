//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <stdexcept>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/throw_exception.hpp>
#include <boost/make_shared.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/echo/server3/session_proxy.h>
#include <ma/echo/server3/session_manager_handler.h>
#include <ma/echo/server3/session_manager.h>

namespace ma
{    
  namespace echo
  {
    namespace server3
    {
      session_manager::settings::settings(const boost::asio::ip::tcp::endpoint& endpoint,
        std::size_t max_sessions,
        std::size_t recycled_sessions,
        int listen_backlog,
        const session::settings& session_settings)
        : endpoint_(endpoint)
        , max_sessions_(max_sessions)
        , recycled_sessions_(recycled_sessions)
        , listen_backlog_(listen_backlog)
        , session_settings_(session_settings)
      {
        if (1 > max_sessions_)
        {
          boost::throw_exception(std::invalid_argument("maximum sessions number must be >= 1"));
        }
      } // session_manager::settings::settings

      session_manager::session_manager(boost::asio::io_service& io_service,
        boost::asio::io_service& session_io_service,
        const settings& settings)
        : strand_(io_service)
        , acceptor_(io_service)
        , has_wait_handler_(false)
        , session_io_service_(session_io_service)                        
        , settings_(settings)
        , pending_operations_(0)
        , state_(ready_to_start)
        , accept_in_progress_(false)
      {          
      } // session_manager::session_manager

      session_manager::~session_manager()
      {        
      } // session_manager::~session_manager

      void session_manager::async_start(const allocator_ptr& operation_allocator,
        const session_manager_start_handler_weak_ptr& handler)
      {
        strand_.dispatch
        (
          make_custom_alloc_handler
          (
            *operation_allocator, 
            boost::bind
            (
              &this_type::do_start,
              shared_from_this(),
              operation_allocator,
              handler
            )
          )
        );
      } // session_manager::async_start
      
      void session_manager::async_stop(const allocator_ptr& operation_allocator,
        const session_manager_stop_handler_weak_ptr& handler)
      {
        strand_.dispatch
        (
          make_custom_alloc_handler
          (
            *operation_allocator, 
            boost::bind
            (
              &this_type::do_stop,
              shared_from_this(),
              operation_allocator,
              handler
            )
          )
        );
      } // session_manager::async_stop
      
      void session_manager::async_wait(const allocator_ptr& operation_allocator,
        const session_manager_wait_handler_weak_ptr& handler)
      {
        strand_.dispatch
        (
          make_custom_alloc_handler
          (
            *operation_allocator, 
            boost::bind
            (
              &this_type::do_wait,
              shared_from_this(),
              operation_allocator,
              handler
            )
          )
        );
      } // session_manager::async_wait

      boost::asio::io_service::strand& session_manager::strand()
      {
        return strand_;
      }

      void session_manager::do_start(const allocator_ptr& operation_allocator,
        const session_manager_start_handler_weak_ptr& handler)
      {
        if (stopped == state_ || stop_in_progress == state_)
        {          
          session_manager_start_handler::invoke(handler, operation_allocator, 
            boost::asio::error::operation_aborted);
        } 
        else if (ready_to_start != state_)
        {          
          session_manager_start_handler::invoke(handler, operation_allocator, 
            boost::asio::error::operation_not_supported);
        }
        else
        {
          state_ = start_in_progress;
          boost::system::error_code error;
          acceptor_.open(settings_.endpoint_.protocol(), error);
          if (!error)
          {
            acceptor_.bind(settings_.endpoint_, error);
            if (!error)
            {
              acceptor_.listen(settings_.listen_backlog_, error);
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
          session_manager_start_handler::invoke(handler, operation_allocator, error);
        }        
      } // session_manager::do_start
      
      void session_manager::do_stop(const allocator_ptr& operation_allocator,
        const session_manager_stop_handler_weak_ptr& handler)
      {
        if (stopped == state_ || stop_in_progress == state_)
        {          
          session_manager_stop_handler::invoke(handler, operation_allocator, 
            boost::asio::error::operation_aborted);
        }
        else
        {
          // Start shutdown
          state_ = stop_in_progress;

          // Do shutdown - abort inner operations          
          acceptor_.close(stop_error_); 

          // Start stop for all active sessions
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
          if (has_wait_handler_)
          {
            session_manager_wait_handler::invoke(wait_handler_.first, wait_handler_.second, 
              boost::asio::error::operation_aborted);            
          }

          // Check for shutdown continuation
          if (may_complete_stop())
          {
            state_ = stopped;          
            // Signal shutdown completion
            session_manager_stop_handler::invoke(handler, operation_allocator, stop_error_);
          }
          else
          { 
            stop_handler_.first = handler;
            stop_handler_.second = operation_allocator;
          }
        }
      } // session_manager::do_stop
      
      void session_manager::do_wait(const allocator_ptr& operation_allocator,
        const session_manager_wait_handler_weak_ptr& handler)
      {
        if (stopped == state_ || stop_in_progress == state_)
        {          
          session_manager_wait_handler::invoke(handler, operation_allocator, 
            boost::asio::error::operation_aborted);
        } 
        else if (started != state_)
        {          
          session_manager_wait_handler::invoke(handler, operation_allocator, 
            boost::asio::error::operation_not_supported);
        }
        else if (last_accept_error_ && active_session_proxies_.empty())
        {
          session_manager_wait_handler::invoke(handler, operation_allocator, last_accept_error_);          
        }
        else if (has_wait_handler_)
        {
          session_manager_wait_handler::invoke(handler, operation_allocator, 
            boost::asio::error::operation_not_supported);          
        }
        else
        {          
          wait_handler_.first = handler;
          wait_handler_.second = operation_allocator;
        }  
      } // session_manager::do_wait

      void session_manager::accept_new_session()
      {
        // Get new ready to start session
        session_proxy_ptr new_session_proxy;
        if (recycled_session_proxies_.empty())
        {
          new_session_proxy = boost::make_shared<session_proxy>(
            boost::ref(session_io_service_), 
            boost::weak_ptr<this_type>(shared_from_this()),
            settings_.session_settings_);
        }
        else
        {
          new_session_proxy = recycled_session_proxies_.front();
          recycled_session_proxies_.erase(new_session_proxy);
        }
        // Start session acceptation
        acceptor_.async_accept
        (
          new_session_proxy->session_->socket(),
          new_session_proxy->endpoint_, 
          strand_.wrap
          (
            make_custom_alloc_handler
            (
              accept_allocator_,
              boost::bind
              (
                &this_type::handle_accept,
                shared_from_this(),
                new_session_proxy,
                boost::asio::placeholders::error
              )
            )
          )
        );
        ++pending_operations_;
        accept_in_progress_ = true;
      } // session_manager::accept_new_session

      void session_manager::handle_accept(const session_proxy_ptr& new_session_proxy,
        const boost::system::error_code& error)
      {
        --pending_operations_;
        accept_in_progress_ = false;
        if (stop_in_progress == state_)
        {
          if (may_complete_stop())  
          {
            state_ = stopped;
            // Signal shutdown completion
            session_manager_stop_handler::invoke(stop_handler_.first, stop_handler_.second, stop_error_);            
          }
        }
        else if (error)
        {   
          last_accept_error_ = error;
          if (active_session_proxies_.empty()) 
          {
            // Server can't work more time
            if (has_wait_handler_)
            {
              session_manager_wait_handler::invoke(wait_handler_.first, stop_handler_.second, error);
            }
          }
        }
        else if (active_session_proxies_.size() < settings_.max_sessions_)
        { 
          // Start accepted session 
          start_session(new_session_proxy);          
          // Save session as active
          active_session_proxies_.push_front(new_session_proxy);
          // Continue session acceptation if can
          if (active_session_proxies_.size() < settings_.max_sessions_)
          {
            accept_new_session();
          }          
        }
        else
        {
          recycle_session(new_session_proxy);
        }
      } // session_manager::handle_accept        

      bool session_manager::may_complete_stop() const
      {
        return 0 == pending_operations_ && active_session_proxies_.empty();
      } // session_manager::may_complete_stop

      void session_manager::start_session(const session_proxy_ptr& accepted_session_proxy)
      {        
        accepted_session_proxy->session_->async_start
        (
          accepted_session_proxy->start_wait_allocator_,
          session_proxy_weak_ptr(accepted_session_proxy)
        );          
        accepted_session_proxy->state_ = session_proxy::start_in_progress;
        ++accepted_session_proxy->pending_operations_;
        ++pending_operations_;        
      } // session_manager::start_session

      void session_manager::stop_session(const session_proxy_ptr& started_session_proxy)
      {
        started_session_proxy->session_->async_stop
        (
          started_session_proxy->stop_allocator_,
          session_proxy_weak_ptr(started_session_proxy)
        );
        started_session_proxy->state_ = session_proxy::stop_in_progress;
        ++started_session_proxy->pending_operations_;
        ++pending_operations_;
      } // session_manager::stop_session

      void session_manager::wait_session(const session_proxy_ptr& started_session_proxy)
      {
        started_session_proxy->session_->async_wait
        (
          started_session_proxy->start_wait_allocator_,
          session_proxy_weak_ptr(started_session_proxy)
        );
        ++started_session_proxy->pending_operations_;
        ++pending_operations_;        
      } // session_manager::wait_session      

      void session_manager::handle_session_start(const session_proxy_ptr& started_session_proxy,
        const allocator_ptr& /*operation_allocator*/,
        const boost::system::error_code& error)
      {
        --pending_operations_;
        --started_session_proxy->pending_operations_;
        if (session_proxy::start_in_progress == started_session_proxy->state_)
        {          
          if (error)
          {
            started_session_proxy->state_ = session_proxy::stopped;
            active_session_proxies_.erase(started_session_proxy);            
            if (stop_in_progress == state_)
            {
              if (may_complete_stop())  
              {
                state_ = stopped;
                // Signal shutdown completion
                session_manager_stop_handler::invoke(stop_handler_.first, stop_handler_.second, stop_error_);
              }
            }
            else if (last_accept_error_ && active_session_proxies_.empty()) 
            {
              if (has_wait_handler_)
              {
                session_manager_wait_handler::invoke(wait_handler_.first, wait_handler_.second,
                  last_accept_error_);
              }              
            }
            else
            {
              recycle_session(started_session_proxy);
              // Continue session acceptation if can
              if (!accept_in_progress_ && !last_accept_error_
                && active_session_proxies_.size() < settings_.max_sessions_)
              {                
                accept_new_session();
              }                
            }
          }
          else // !error
          {
            started_session_proxy->state_ = session_proxy::started;
            if (stop_in_progress == state_)  
            {                            
              stop_session(started_session_proxy);
            }
            else
            { 
              // Wait until session needs to stop
              wait_session(started_session_proxy);
            }            
          }  
        } 
        else if (stop_in_progress == state_) 
        {
          if (may_complete_stop())  
          {
            state_ = stopped;
            // Signal shutdown completion
            session_manager_stop_handler::invoke(stop_handler_.first, stop_handler_.second, stop_error_);
          }
        }
        else
        {
          recycle_session(started_session_proxy);
        }        
      } // session_manager::handle_session_start      

      void session_manager::handle_session_wait(const session_proxy_ptr& waited_session_proxy,
        const allocator_ptr& /*operation_allocator*/,
        const boost::system::error_code& /*error*/)
      {
        --pending_operations_;
        --waited_session_proxy->pending_operations_;
        if (session_proxy::started == waited_session_proxy->state_)
        {
          stop_session(waited_session_proxy);
        }
        else if (stop_in_progress == state_)
        {
          if (may_complete_stop())  
          {
            state_ = stopped;
            // Signal shutdown completion
            session_manager_stop_handler::invoke(stop_handler_.first, stop_handler_.second, stop_error_);
          }
        }
        else
        {
          recycle_session(waited_session_proxy);
        }
      } // session_manager::handle_session_wait      

      void session_manager::handle_session_stop(const session_proxy_ptr& stopped_session_proxy,
        const allocator_ptr& /*operation_allocator*/,
        const boost::system::error_code& /*error*/)
      {
        --pending_operations_;
        --stopped_session_proxy->pending_operations_;
        if (session_proxy::stop_in_progress == stopped_session_proxy->state_)        
        {
          stopped_session_proxy->state_ = session_proxy::stopped;
          active_session_proxies_.erase(stopped_session_proxy);            
          if (stop_in_progress == state_)
          {
            if (may_complete_stop())  
            {
              state_ = stopped;
              // Signal shutdown completion
              session_manager_stop_handler::invoke(stop_handler_.first, stop_handler_.second, stop_error_);
            }
          }
          else if (last_accept_error_ && active_session_proxies_.empty()) 
          {
            if (has_wait_handler_)
            {
              session_manager_wait_handler::invoke(wait_handler_.first, wait_handler_.second, 
                last_accept_error_);              
            }
          }
          else
          {
            recycle_session(stopped_session_proxy);
            // Continue session acceptation if can
            if (!accept_in_progress_ && !last_accept_error_
              && active_session_proxies_.size() < settings_.max_sessions_)
            {                
              accept_new_session();
            }              
          }
        }
        else if (stop_in_progress == state_)
        {
          if (may_complete_stop())  
          {
            state_ = stopped;
            // Signal shutdown completion
            session_manager_stop_handler::invoke(stop_handler_.first, stop_handler_.second, stop_error_);
          }
        }
        else
        {
          recycle_session(stopped_session_proxy);
        }
      } // session_manager::handle_session_stop

      void session_manager::recycle_session(const session_proxy_ptr& recycled_session_proxy)
      {
        if (0 == recycled_session_proxy->pending_operations_
          && recycled_session_proxies_.size() < settings_.recycled_sessions_)
        {
          recycled_session_proxy->session_->reset();
          recycled_session_proxy->state_ = session_proxy::ready_to_start;
          recycled_session_proxies_.push_front(recycled_session_proxy);
        }        
      } // recycle_session

    } // namespace server3
  } // namespace echo
} // namespace ma