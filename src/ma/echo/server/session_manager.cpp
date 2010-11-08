//
// Copyright (c) 2010 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <new>
#include <boost/ref.hpp>
#include <boost/make_shared.hpp>
#include <boost/assert.hpp>
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session.hpp>
#include <ma/echo/server/session_manager.hpp>
#include <ma/echo/server/error.hpp>

namespace ma
{    
  namespace echo
  {    
    namespace server
    {    
      session_manager::session_wrapper::session_wrapper(boost::asio::io_service& io_service,
        const session_config& holded_session_config)
        : state(ready_to_start)
        , pending_operations(0)
        , wrapped_session(boost::make_shared<session>(boost::ref(io_service), holded_session_config))
      {
      } // session_manager::session_wrapper::session_wrapper
      
      session_manager::session_wrapper_list::session_wrapper_list()
        : size_(0)
      {
      } // session_manager::session_wrapper_list::session_wrapper_list

      void session_manager::session_wrapper_list::push_front(const session_wrapper_ptr& value)
      {
        BOOST_ASSERT((!value->next && !value->prev.lock()));        
        value->next = front_;
        value->prev.reset();
        if (front_)
        {
          front_->prev = value;
        }
        front_ = value;
        ++size_;
      } // session_manager::session_wrapper_list::push_front

      void session_manager::session_wrapper_list::erase(const session_wrapper_ptr& value)
      {
        if (front_ == value)
        {
          front_ = front_->next;
        }
        session_wrapper_ptr prev = value->prev.lock();
        if (prev)
        {
          prev->next = value->next;
        }
        if (value->next)
        {
          value->next->prev = prev;
        }
        value->prev.reset();
        value->next.reset();
        --size_;
      } // session_manager::session_wrapper_list::erase                  

      void session_manager::session_wrapper_list::clear()
      {
        front_.reset();
      } // session_manager::session_wrapper_list::clear
        
      session_manager::session_manager(boost::asio::io_service& io_service, 
        boost::asio::io_service& session_io_service, 
        const session_manager_config& config)
        : accept_in_progress_(false)
        , state_(ready_to_start)
        , pending_operations_(0)
        , io_service_(io_service)
        , session_io_service_(session_io_service)
        , strand_(io_service)
        , acceptor_(io_service)        
        , wait_handler_(io_service)
        , stop_handler_(io_service)
        , config_(config)        
      {          
      } // session_manager::session_manager

      void session_manager::reset(bool free_recycled_sessions)
      {
        boost::system::error_code ignored;
        acceptor_.close(ignored);
        wait_error_.clear();
        stop_error_.clear();
        state_ = ready_to_start;
        active_sessions_.clear();
        if (free_recycled_sessions)
        {
          recycled_sessions_.clear();
        }
      }

      boost::system::error_code session_manager::start()
      {         
        if (ready_to_start != state_)
        {          
          return server_error::invalid_state;
        }        
        state_ = start_in_progress;
        boost::system::error_code error;
        acceptor_.open(config_.accepting_endpoint.protocol(), error);
        if (!error)
        {
          acceptor_.bind(config_.accepting_endpoint, error);
          if (!error)
          {
            acceptor_.listen(config_.listen_backlog, error);          
            if (!error)
            {
              session_wrapper_ptr the_session = create_session(error);              
              if (!error) 
              {
                accept_session(the_session); 
              }            
            }
          }
          if (error)
          {
            boost::system::error_code ignored;
            acceptor_.close(ignored);          
          }
        }
        if (error)
        {
          state_ = stopped;         
        }
        else
        {          
          state_ = started;
        }       
        return error;
      } // session_manager::start

      boost::optional<boost::system::error_code> session_manager::stop()
      {        
        if (stopped == state_ || stop_in_progress == state_)
        {          
          return server_error::invalid_state;
        }        
        // Start shutdown
        state_ = stop_in_progress;
        // Do shutdown - abort inner operations          
        acceptor_.close(stop_error_);
        // Stop all active sessions
        session_wrapper_ptr active_session = active_sessions_.front();
        while (active_session)
        {
          if (stop_in_progress != active_session->state)
          {
            stop_session(active_session);
          }
          active_session = active_session->next;
        }
        // Do shutdown - abort outer operations
        if (wait_handler_.has_target())
        {
          wait_handler_.post(server_error::operation_aborted);
        }            
        // Check for shutdown continuation
        if (may_complete_stop())
        {
          complete_stop();
          return stop_error_;          
        }
        return boost::optional<boost::system::error_code>();
      } // session_manager::stop

      boost::optional<boost::system::error_code> session_manager::wait()
      {        
        if (started != state_ || wait_handler_.has_target())
        {
          return server_error::invalid_state;
        }        
        if (may_complete_wait())
        {
          return wait_error_;
        }        
        return boost::optional<boost::system::error_code>();
      } // session_manager::wait

      session_manager::session_wrapper_ptr session_manager::create_session(boost::system::error_code& error)      
      {        
        if (!recycled_sessions_.empty())
        {          
          session_wrapper_ptr the_session = recycled_sessions_.front();
          recycled_sessions_.erase(the_session);
          error = boost::system::error_code();
          return the_session;
        }        
        try 
        {   
          session_wrapper_ptr the_session = boost::make_shared<session_wrapper>(
            boost::ref(session_io_service_), config_.managed_session_config);
          error = boost::system::error_code();
          return the_session;
        }
        catch (const std::bad_alloc&) 
        {
          error = boost::system::errc::make_error_code(boost::system::errc::not_enough_memory);
          return session_wrapper_ptr();
        }        
      } // session_manager::create_session

      void session_manager::accept_session(const session_wrapper_ptr& the_session)
      {
        acceptor_.async_accept(the_session->wrapped_session->socket(), the_session->remote_endpoint, 
          strand_.wrap(make_custom_alloc_handler(accept_allocator_,
          boost::bind(&this_type::handle_accept, shared_from_this(), the_session, boost::asio::placeholders::error))));
        // Register pending operation
        ++pending_operations_;
        accept_in_progress_ = true;
      } // session_manager::accept_session

      void session_manager::accept_new_session()
      {
        // Get new, ready to start session
        boost::system::error_code error;
        session_wrapper_ptr the_session = create_session(error);
        if (error)
        {
          // Handle new session creation error
          wait_error_ = error;
          // Notify wait handler
          if (wait_handler_.has_target() && may_complete_wait()) 
          {            
            wait_handler_.post(wait_error_);
          }      
          // Can't do more
          return;
        }        
        // Start session acceptation
        accept_session(the_session);
      } // session_manager::accept_new_session

      void session_manager::handle_accept(const session_wrapper_ptr& the_session, 
        const boost::system::error_code& error)
      {
        // Unregister pending operation
        --pending_operations_;
        accept_in_progress_ = false;
        // Check for pending session manager stop operation 
        if (stop_in_progress == state_)
        {
          if (may_complete_stop())  
          {
            complete_stop();
            post_stop_handler();
          }
          recycle_session(the_session);
          return;
        }
        if (error)
        {   
          wait_error_ = error;
          // Notify wait handler
          if (wait_handler_.has_target() && may_complete_wait()) 
          {            
            wait_handler_.post(wait_error_);
          }
          recycle_session(the_session);
          if (may_continue_accept())
          {
            accept_new_session();
          }
          return;
        }
        if (active_sessions_.size() < config_.max_session_num)
        { 
          // Start accepted session 
          start_session(the_session);          
          // Save session as active
          active_sessions_.push_front(the_session);
          // Continue session acceptation if can
          if (may_continue_accept())
          {
            accept_new_session();
          }
          return;
        }
        recycle_session(the_session);
      } // handle_accept        

      bool session_manager::may_complete_stop() const
      {
        return 0 == pending_operations_ && active_sessions_.empty();
      } // session_manager::may_complete_stop

      bool session_manager::may_complete_wait() const
      {
        return wait_error_ && active_sessions_.empty();
      } // session_manager::may_complete_wait

      void session_manager::complete_stop()
      {
        state_ = stopped;  
      } // session_manager::complete_stop

      bool session_manager::may_continue_accept() const
      {
        return !accept_in_progress_ && !wait_error_
          && active_sessions_.size() < config_.max_session_num;
      } // may_continue_accept

      void session_manager::start_session(const session_wrapper_ptr& accepted_session)
      { 
        // Asynchronously start wrapped session
        accepted_session->wrapped_session->async_start(make_custom_alloc_handler(accepted_session->start_wait_allocator, 
          boost::bind(&this_type::dispatch_session_start, session_manager_weak_ptr(shared_from_this()), accepted_session, _1)));  
        accepted_session->state = session_wrapper::start_in_progress;
        // Register pending operation
        ++pending_operations_;
        ++accepted_session->pending_operations;        
      } // session_manager::start_session

      void session_manager::stop_session(const session_wrapper_ptr& the_session)
      {
        // Asynchronously stop wrapped session
        the_session->wrapped_session->async_stop(make_custom_alloc_handler(the_session->stop_allocator,
          boost::bind(&this_type::dispatch_session_stop, session_manager_weak_ptr(shared_from_this()), the_session, _1)));
        the_session->state = session_wrapper::stop_in_progress;
        // Register pending operation
        ++pending_operations_;
        ++the_session->pending_operations;        
      } // stop_session

      void session_manager::wait_session(const session_wrapper_ptr& the_session)
      {
        // Asynchronously wait on wrapped session
        the_session->wrapped_session->async_wait(make_custom_alloc_handler(the_session->start_wait_allocator,
          boost::bind(&this_type::dispatch_session_wait, session_manager_weak_ptr(shared_from_this()), the_session, _1)));
        // Register pending operation
        ++pending_operations_;
        ++the_session->pending_operations;
      } // session_manager::wait_session

      void session_manager::dispatch_session_start(const session_manager_weak_ptr& this_weak_ptr,
        const session_wrapper_ptr& the_session, const boost::system::error_code& error)
      {
        // Try to lock the manager
        if (session_manager_ptr this_ptr = this_weak_ptr.lock())
        {
          // Forward invocation
          this_ptr->strand_.dispatch(make_custom_alloc_handler(the_session->start_wait_allocator,
            boost::bind(&this_type::handle_session_start, this_ptr, the_session, error)));
        }
      } // session_manager::dispatch_session_start

      void session_manager::handle_session_start(const session_wrapper_ptr& the_session, 
        const boost::system::error_code& error)
      {
        // Unregister pending operation
        --pending_operations_;
        --the_session->pending_operations;
        // Check if handler is not called too late
        if (session_wrapper::start_in_progress == the_session->state)
        {
          if (error) 
          {
            the_session->state = session_wrapper::stopped;
            active_sessions_.erase(the_session);
            recycle_session(the_session);
            // Check for pending session manager stop operation 
            if (stop_in_progress == state_)
            {
              if (may_complete_stop())  
              {
                complete_stop();
                post_stop_handler();
              }            
              return;
            }
            if (wait_handler_.has_target() && may_complete_wait()) 
            {
              wait_handler_.post(wait_error_);            
            }
            // Continue session acceptation if can
            if (may_continue_accept())
            {                
              accept_new_session();
            }                            
            return;
          }
          the_session->state = session_wrapper::started;
          // Check for pending session manager stop operation 
          if (stop_in_progress == state_)  
          {                            
            stop_session(the_session);
            return;
          }
          // Wait until session needs to stop
          wait_session(the_session);          
          return;
        }
        // Handler is called too late - complete handler's waiters
        recycle_session(the_session);
        // Check for pending session manager stop operation 
        if (stop_in_progress == state_) 
        {
          if (may_complete_stop())  
          {
            complete_stop();
            post_stop_handler();
          }
        }        
      } // session_manager::handle_session_start

      void session_manager::dispatch_session_wait(const session_manager_weak_ptr& this_weak_ptr,
        const session_wrapper_ptr& the_session, const boost::system::error_code& error)
      {
        // Try to lock the manager
        if (session_manager_ptr this_ptr = this_weak_ptr.lock())
        {
          // Forward invocation
          this_ptr->strand_.dispatch(make_custom_alloc_handler(the_session->start_wait_allocator,
            boost::bind(&this_type::handle_session_wait, this_ptr, the_session, error)));
        }
      } // session_manager::dispatch_session_wait

      void session_manager::handle_session_wait(const session_wrapper_ptr& the_session, 
        const boost::system::error_code& /*error*/)
      {
        // Unregister pending operation
        --pending_operations_;
        --the_session->pending_operations;
        // Check if handler is not called too late
        if (session_wrapper::started == the_session->state)
        {
          stop_session(the_session);
          return;
        }
        // Handler is called too late - complete handler's waiters
        recycle_session(the_session);
        // Check for pending session manager stop operation
        if (stop_in_progress == state_)
        {
          if (may_complete_stop())  
          {
            complete_stop();
            post_stop_handler();
          }          
        }        
      } // session_manager::handle_session_wait

      void session_manager::dispatch_session_stop(const session_manager_weak_ptr& this_weak_ptr,
        const session_wrapper_ptr& the_session, const boost::system::error_code& error)
      {     
        // Try to lock the manager
        if (session_manager_ptr this_ptr = this_weak_ptr.lock())
        {
          // Forward invocation
          this_ptr->strand_.dispatch(make_custom_alloc_handler(the_session->stop_allocator,
            boost::bind(&this_type::handle_session_stop, this_ptr, the_session, error)));
        }
      } // session_manager::dispatch_session_stop

      void session_manager::handle_session_stop(const session_wrapper_ptr& the_session,
        const boost::system::error_code& /*error*/)
      {
        // Unregister pending operation
        --pending_operations_;
        --the_session->pending_operations;
        // Check if handler is not called too late
        if (session_wrapper::stop_in_progress == the_session->state)        
        {
          the_session->state = session_wrapper::stopped;
          active_sessions_.erase(the_session);
          recycle_session(the_session);
          // Check for pending session manager stop operation
          if (stop_in_progress == state_)
          {
            if (may_complete_stop())  
            {
              complete_stop();
              post_stop_handler();
            }
            return;
          }
          if (wait_handler_.has_target() && may_complete_wait()) 
          {
            wait_handler_.post(wait_error_);
          }
          // Continue session acceptation if can
          if (may_continue_accept())
          {                
            accept_new_session();
          }
          return;
        }
        // Handler is called too late - complete handler's waiters
        recycle_session(the_session);
        // Check for pending session manager stop operation
        if (stop_in_progress == state_)
        {
          if (may_complete_stop())  
          {
            complete_stop();
            post_stop_handler();
          }          
        }                
      } // session_manager::handle_session_stop

      void session_manager::recycle_session(const session_wrapper_ptr& the_session)
      {
        // Check session's pending operation number and recycle bin size
        if (0 == the_session->pending_operations
          && recycled_sessions_.size() < config_.recycled_session_num)
        {
          // Reset session state
          the_session->wrapped_session->reset();
          // Reset wrapper
          the_session->state = session_wrapper::ready_to_start;
          // Add to recycle bin
          recycled_sessions_.push_front(the_session);
        }
      } // session_manager::recycle_session

      void session_manager::post_stop_handler()
      {
        if (stop_handler_.has_target()) 
        {
          // Signal shutdown completion
          stop_handler_.post(stop_error_);
        }
      } // session_manager::post_stop_handler

    } // namespace server
  } // namespace echo
} // namespace ma

