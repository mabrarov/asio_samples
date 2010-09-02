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
#include <ma/echo/server/session_manager.hpp>

namespace ma
{    
  namespace echo
  {    
    namespace server
    {    
      session_manager::session_proxy::session_proxy(boost::asio::io_service& io_service,
        const session::settings& session_settings)
        : state_(ready_to_start)
        , pending_operations_(0)
        , session_(boost::make_shared<session>(boost::ref(io_service), session_settings))
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

      session_manager::settings::settings(const boost::asio::ip::tcp::endpoint& endpoint,
        std::size_t max_sessions, std::size_t recycled_sessions,
        int listen_backlog, const session::settings& session_settings)
        : listen_backlog_(listen_backlog)
        , max_sessions_(max_sessions)
        , recycled_sessions_(recycled_sessions)
        , endpoint_(endpoint)                
        , session_settings_(session_settings)
      {
        if (1 > max_sessions_)
        {
          boost::throw_exception(std::invalid_argument("maximum sessions number must be >= 1"));
        }
      } // session_manager::settings::settings
        
      session_manager::session_manager(boost::asio::io_service& io_service,
        boost::asio::io_service& session_io_service, const settings& settings)
        : accept_in_progress_(false)
        , state_(ready_to_start)
        , pending_operations_(0)
        , io_service_(io_service)
        , session_io_service_(session_io_service)
        , strand_(io_service)
        , acceptor_(io_service)        
        , wait_handler_(io_service)
        , stop_handler_(io_service)
        , settings_(settings)
        
      {          
      } // session_manager::session_manager

      session_manager::~session_manager()
      {        
      } // session_manager::session_manager  

      void session_manager::start_service(boost::system::error_code& error)
      {
        state_ = start_in_progress;        
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
      } // session_manager::start_service

      void session_manager::stop_service()
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
        if (wait_handler_.has_target())
        {
          wait_handler_.post(boost::asio::error::operation_aborted);
        }
      } // session_manager::stop_service

      void session_manager::accept_new_session()
      {
        // Get new ready to start session
        session_proxy_ptr new_session_proxy;
        if (recycled_session_proxies_.empty())
        {
          new_session_proxy = boost::make_shared<session_proxy>(
            boost::ref(session_io_service_), settings_.session_settings_);
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
      } // handle_accept        

      bool session_manager::may_complete_stop() const
      {
        return 0 == pending_operations_ && active_session_proxies_.empty();
      } // session_manager::may_complete_stop

      void session_manager::start_session(const session_proxy_ptr& accepted_session_proxy)
      {        
        accepted_session_proxy->session_->async_start
        (
          make_custom_alloc_handler
          (
            accepted_session_proxy->start_wait_allocator_,
            boost::bind
            (
              &this_type::dispatch_session_start,
              session_manager_weak_ptr(shared_from_this()),
              accepted_session_proxy,
              _1
            )
          )           
        );  
        ++pending_operations_;
        ++accepted_session_proxy->pending_operations_;
        accepted_session_proxy->state_ = session_proxy::start_in_progress;
      } // session_manager::start_session

      void session_manager::stop_session(const session_proxy_ptr& started_session_proxy)
      {
        started_session_proxy->session_->async_stop
        (
          make_custom_alloc_handler
          (
            started_session_proxy->stop_allocator_,
            boost::bind
            (
              &this_type::dispatch_session_stop,
              session_manager_weak_ptr(shared_from_this()),
              started_session_proxy,
              _1
            )
          )
        );
        ++pending_operations_;
        ++started_session_proxy->pending_operations_;
        started_session_proxy->state_ = session_proxy::stop_in_progress;
      } // stop_session

      void session_manager::wait_session(const session_proxy_ptr& started_session_proxy)
      {
        started_session_proxy->session_->async_wait
        (
          make_custom_alloc_handler
          (
            started_session_proxy->start_wait_allocator_,
            boost::bind
            (
              &this_type::dispatch_session_wait,
              session_manager_weak_ptr(shared_from_this()),
              started_session_proxy,
              _1
            )
          )
        );
        ++pending_operations_;
        ++started_session_proxy->pending_operations_;
      } // session_manager::wait_session

      void session_manager::dispatch_session_start(const session_manager_weak_ptr& weak_session_manager,
        const session_proxy_ptr& started_session_proxy, const boost::system::error_code& error)
      {
        if (session_manager_ptr this_ptr = weak_session_manager.lock())
        {
          this_ptr->strand_.dispatch
          (
            make_custom_alloc_handler
            (
              started_session_proxy->start_wait_allocator_,
              boost::bind
              (
                &this_type::handle_session_start,
                this_ptr,
                started_session_proxy,
                error
              )
            )
          );
        }
      } // session_manager::dispatch_session_start

      void session_manager::handle_session_start(const session_proxy_ptr& started_session_proxy,
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
                stop_handler_.post(stop_error_);
              }
            }
            else if (last_accept_error_ && active_session_proxies_.empty()) 
            {
              wait_handler_.post(last_accept_error_);
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
            stop_handler_.post(stop_error_);
          }
        }
        else
        {
          recycle_session(started_session_proxy);
        }        
      } // session_manager::handle_session_start

      void session_manager::dispatch_session_wait(const session_manager_weak_ptr& weak_session_manager,
        const session_proxy_ptr& waited_session_proxy, const boost::system::error_code& error)
      {          
        if (session_manager_ptr this_ptr = weak_session_manager.lock())
        {
          this_ptr->strand_.dispatch
          (
            make_custom_alloc_handler
            (
              waited_session_proxy->start_wait_allocator_,
              boost::bind
              (
                &this_type::handle_session_wait,
                this_ptr,
                waited_session_proxy,
                error
              )
            )
          );
        }
      } // session_manager::dispatch_session_wait

      void session_manager::handle_session_wait(const session_proxy_ptr& waited_session_proxy,
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
            stop_handler_.post(stop_error_);
          }
        }
        else
        {
          recycle_session(waited_session_proxy);
        }
      } // session_manager::handle_session_wait

      void session_manager::dispatch_session_stop(const session_manager_weak_ptr& weak_session_manager,
        const session_proxy_ptr& stopped_session_proxy, const boost::system::error_code& error)
      {          
        if (session_manager_ptr this_ptr = weak_session_manager.lock())
        {
          this_ptr->strand_.dispatch
          (
            make_custom_alloc_handler
            (
              stopped_session_proxy->stop_allocator_,
              boost::bind
              (
                &this_type::handle_session_stop,
                this_ptr,
                stopped_session_proxy,
                error
              )
            )
          );
        }
      } // session_manager::dispatch_session_stop

      void session_manager::handle_session_stop(const session_proxy_ptr& stopped_session_proxy,
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
              stop_handler_.post(stop_error_);
            }
          }
          else if (last_accept_error_ && active_session_proxies_.empty()) 
          {
            wait_handler_.post(last_accept_error_);
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
            stop_handler_.post(stop_error_);
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
      } // session_manager::recycle_session        

    } // namespace server
  } // namespace echo
} // namespace ma

