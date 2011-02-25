//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <new>
#include <boost/ref.hpp>
#include <boost/assert.hpp>
#include <boost/make_shared.hpp>
#include <ma/config.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session.hpp>
#include <ma/echo/server/session_manager.hpp>

namespace ma
{    
  namespace echo
  {    
    namespace server
    {  
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
      class session_manager::accept_handler_binder
      {
      private:
        typedef accept_handler_binder this_type;
        this_type& operator=(const this_type&);

      public:
        typedef void result_type;
        typedef void (session_manager::*function_type)(
          const session_manager::session_wrapper_ptr&,
          const boost::system::error_code&);

        template <typename SessionManagerPtr, typename SessionWrapperPtr>
        accept_handler_binder(function_type function, 
          SessionManagerPtr&& the_session_manager,
          SessionWrapperPtr&& the_wrapped_session)
          : function_(function)
          , session_manager_(std::forward<SessionManagerPtr>(the_session_manager))
          , wrapped_session_(std::forward<SessionWrapperPtr>(the_wrapped_session))
        {
        }

#if defined(_DEBUG)
        accept_handler_binder(const this_type& other)
          : function_(other.function_)
          , session_manager_(other.session_manager_)
          , wrapped_session_(other.wrapped_session_)
        {
        }
#endif // defined(_DEBUG)

        accept_handler_binder(this_type&& other)
          : function_(other.function_)
          , session_manager_(std::move(other.session_manager_))
          , wrapped_session_(std::move(other.wrapped_session_))
        {
        }

        void operator()(const boost::system::error_code& error)
        {
          ((*session_manager_).*function_)(wrapped_session_, error);
        }

      private:
        function_type function_;
        session_manager_ptr session_manager_;
        session_manager::session_wrapper_ptr wrapped_session_;
      }; // class session_manager::accept_handler_binder

      class session_manager::session_dispatch_binder
      {
      private:
        typedef session_dispatch_binder this_type;
        this_type& operator=(const this_type&);

      public:
        typedef void (*function_type)(
          const session_manager_weak_ptr&,
          const session_manager::session_wrapper_ptr&, 
          const boost::system::error_code&);

        template <typename SessionManagerPtr, typename SessionWrapperPtr>
        session_dispatch_binder(function_type function, 
          SessionManagerPtr&& the_session_manager,
          SessionWrapperPtr&& the_wrapped_session)
          : function_(function)
          , session_manager_(std::forward<SessionManagerPtr>(the_session_manager))
          , wrapped_session_(std::forward<SessionWrapperPtr>(the_wrapped_session))
        {
        }

#if defined(_DEBUG)
        session_dispatch_binder(const this_type& other)
          : function_(other.function_)
          , session_manager_(other.session_manager_)
          , wrapped_session_(other.wrapped_session_)
        {
        }
#endif // defined(_DEBUG)

        session_dispatch_binder(this_type&& other)
          : function_(other.function_)
          , session_manager_(std::move(other.session_manager_))
          , wrapped_session_(std::move(other.wrapped_session_))
        {
        }

        void operator()(const boost::system::error_code& error)
        {
          function_(session_manager_, wrapped_session_, error);
        }

      private:
        function_type function_;
        session_manager_weak_ptr session_manager_;
        session_manager::session_wrapper_ptr wrapped_session_;
      }; // class session_manager::session_dispatch_binder      

      class session_manager::session_handler_binder
      {
      private:
        typedef session_handler_binder this_type;
        this_type& operator=(const this_type&);

      public:
        typedef void result_type;
        typedef void (session_manager::*function_type)(
          const session_manager::session_wrapper_ptr&,
          const boost::system::error_code&);

        template <typename SessionManagerPtr, typename SessionWrapperPtr>
        session_handler_binder(function_type function, 
          SessionManagerPtr&& the_session_manager,
          SessionWrapperPtr&& the_wrapped_session,
          const boost::system::error_code& error)
          : function_(function)
          , session_manager_(std::forward<SessionManagerPtr>(the_session_manager))
          , wrapped_session_(std::forward<SessionWrapperPtr>(the_wrapped_session))
          , error_(error)
        {
        }                  

#if defined(_DEBUG)
        session_handler_binder(const this_type& other)
          : function_(other.function_)
          , session_manager_(other.session_manager_)
          , wrapped_session_(other.wrapped_session_)
          , error_(other.error_)
        {
        }
#endif // defined(_DEBUG)

        session_handler_binder(this_type&& other)
          : function_(other.function_)
          , session_manager_(std::move(other.session_manager_))
          , wrapped_session_(std::move(other.wrapped_session_))
          , error_(std::move(other.error_))
        {
        }

        void operator()()
        {
          ((*session_manager_).*function_)(wrapped_session_, error_);
        }

      private:
        function_type function_;
        session_manager_ptr session_manager_;
        session_manager::session_wrapper_ptr wrapped_session_;
        boost::system::error_code error_;
      }; // class session_manager::session_handler_binder
#endif // defined(MA_HAS_RVALUE_REFS)

      session_manager::session_wrapper::session_wrapper(
        boost::asio::io_service& io_service,
        const session_config& holded_session_config)
        : state(ready_to_start)
        , pending_operations(0)
        , the_session(boost::make_shared<session>(boost::ref(io_service), holded_session_config))
      {
      }
      
      session_manager::session_wrapper_list::session_wrapper_list()
        : size_(0)
      {
      }

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
      }

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
      }

      void session_manager::session_wrapper_list::clear()
      {
        front_.reset();
      }
        
      session_manager::session_manager(
        boost::asio::io_service& io_service, 
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
      }

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
              session_wrapper_ptr wrapped_session = create_session(error);
              if (!error) 
              {
                accept_session(wrapped_session); 
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
      }

      boost::optional<boost::system::error_code> session_manager::stop()
      {        
        if (stopped == state_ || stop_in_progress == state_)
        {          
          return boost::system::error_code(server_error::invalid_state);
        }        
        // Start shutdown
        state_ = stop_in_progress;
        // Do shutdown - abort inner operations          
        acceptor_.close(stop_error_);
        // Stop all active sessions
        session_wrapper_ptr active_session = active_sessions_.front();
        while (active_session)
        {
          if (session_wrapper::stop_in_progress != active_session->state)
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
      }

      boost::optional<boost::system::error_code> session_manager::wait()
      {        
        if (started != state_ || wait_handler_.has_target())
        {
          return boost::system::error_code(server_error::invalid_state);
        }        
        if (may_complete_wait())
        {
          return wait_error_;
        }        
        return boost::optional<boost::system::error_code>();
      }

      session_manager::session_wrapper_ptr session_manager::create_session(boost::system::error_code& error)
      {        
        if (!recycled_sessions_.empty())
        {          
          session_wrapper_ptr wrapped_session = recycled_sessions_.front();
          recycled_sessions_.erase(wrapped_session);
          error = boost::system::error_code();
          return wrapped_session;
        }        
        try 
        {   
          session_wrapper_ptr wrapped_session = boost::make_shared<session_wrapper>(
              boost::ref(session_io_service_), config_.managed_session_config);
          error = boost::system::error_code();
          return wrapped_session;
        }
        catch (const std::bad_alloc&) 
        {
          error = boost::system::errc::make_error_code(boost::system::errc::not_enough_memory);
          return session_wrapper_ptr();
        }        
      }

      void session_manager::accept_session(const session_wrapper_ptr& wrapped_session)
      {
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
        acceptor_.async_accept(wrapped_session->the_session->socket(), wrapped_session->remote_endpoint, 
          MA_STRAND_WRAP(strand_, make_custom_alloc_handler(accept_allocator_,
			      accept_handler_binder(&this_type::handle_accept, shared_from_this(), wrapped_session))));
#else
        acceptor_.async_accept(wrapped_session->the_session->socket(), wrapped_session->remote_endpoint, 
          MA_STRAND_WRAP(strand_, make_custom_alloc_handler(accept_allocator_,
			      boost::bind(&this_type::handle_accept, shared_from_this(), 
              wrapped_session, boost::asio::placeholders::error))));
#endif
        // Register pending operation
        ++pending_operations_;
        accept_in_progress_ = true;
      }

      void session_manager::accept_new_session()
      {
        // Get new, ready to start session
        boost::system::error_code error;
        session_wrapper_ptr wrapped_session = create_session(error);
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
        accept_session(wrapped_session);
      }

      void session_manager::handle_accept(
        const session_wrapper_ptr& wrapped_session, 
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
          recycle_session(wrapped_session);
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
          recycle_session(wrapped_session);
          if (may_continue_accept())
          {
            accept_new_session();
          }
          return;
        }
        if (active_sessions_.size() < config_.max_session_count)
        { 
          // Start accepted session 
          start_session(wrapped_session);          
          // Save session as active
          active_sessions_.push_front(wrapped_session);
          // Continue session acceptation if can
          if (may_continue_accept())
          {
            accept_new_session();
          }
          return;
        }
        recycle_session(wrapped_session);
      }

      bool session_manager::may_complete_stop() const
      {
        return 0 == pending_operations_ && active_sessions_.empty();
      }

      bool session_manager::may_complete_wait() const
      {
        return wait_error_ && active_sessions_.empty();
      }

      void session_manager::complete_stop()
      {
        state_ = stopped;  
      }

      bool session_manager::may_continue_accept() const
      {
        return !accept_in_progress_ && !wait_error_
          && active_sessions_.size() < config_.max_session_count;
      }

      void session_manager::start_session(const session_wrapper_ptr& wrapped_session)
      { 
        // Asynchronously start wrapped session
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
        wrapped_session->the_session->async_start(
          make_custom_alloc_handler(wrapped_session->start_wait_allocator, 
            session_dispatch_binder(&this_type::dispatch_session_start, shared_from_this(), wrapped_session)));
#else
        wrapped_session->the_session->async_start(
          make_custom_alloc_handler(wrapped_session->start_wait_allocator, 
            boost::bind(&this_type::dispatch_session_start, 
              session_manager_weak_ptr(shared_from_this()), wrapped_session, _1)));
#endif
        wrapped_session->state = session_wrapper::start_in_progress;
        // Register pending operation
        ++pending_operations_;
        ++wrapped_session->pending_operations;        
      }

      void session_manager::stop_session(const session_wrapper_ptr& wrapped_session)
      {
        // Asynchronously stop wrapped session
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
        wrapped_session->the_session->async_stop(
          make_custom_alloc_handler(wrapped_session->stop_allocator,
            session_dispatch_binder(&this_type::dispatch_session_stop, shared_from_this(), wrapped_session)));
#else
        wrapped_session->the_session->async_stop(
          make_custom_alloc_handler(wrapped_session->stop_allocator,
            boost::bind(&this_type::dispatch_session_stop, 
              session_manager_weak_ptr(shared_from_this()), wrapped_session, _1)));
#endif
        wrapped_session->state = session_wrapper::stop_in_progress;
        // Register pending operation
        ++pending_operations_;
        ++wrapped_session->pending_operations;        
      }

      void session_manager::wait_session(
        const session_wrapper_ptr& wrapped_session)
      {
        // Asynchronously wait on wrapped session
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
        wrapped_session->the_session->async_wait(
          make_custom_alloc_handler(wrapped_session->start_wait_allocator,
            session_dispatch_binder(&this_type::dispatch_session_wait, shared_from_this(), wrapped_session)));
#else
        wrapped_session->the_session->async_wait(
          make_custom_alloc_handler(wrapped_session->start_wait_allocator,
            boost::bind(&this_type::dispatch_session_wait, 
              session_manager_weak_ptr(shared_from_this()), wrapped_session, _1)));
#endif
        // Register pending operation
        ++pending_operations_;
        ++wrapped_session->pending_operations;
      }

      void session_manager::dispatch_session_start(
        const session_manager_weak_ptr& this_weak_ptr,
        const session_wrapper_ptr& wrapped_session, 
        const boost::system::error_code& error)
      {
        // Try to lock the manager
        if (session_manager_ptr this_ptr = this_weak_ptr.lock())
        {
          // Forward invocation
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
          this_ptr->strand_.dispatch(
            make_custom_alloc_handler(wrapped_session->start_wait_allocator,
              session_handler_binder(&this_type::handle_session_start, this_ptr, wrapped_session, error)));
#else
          this_ptr->strand_.dispatch(
            make_custom_alloc_handler(wrapped_session->start_wait_allocator,
              boost::bind(&this_type::handle_session_start, this_ptr, wrapped_session, error)));
#endif
        }
      }

      void session_manager::handle_session_start(
        const session_wrapper_ptr& wrapped_session, 
        const boost::system::error_code& error)
      {
        // Unregister pending operation
        --pending_operations_;
        --wrapped_session->pending_operations;
        // Check if handler is not called too late
        if (session_wrapper::start_in_progress == wrapped_session->state)
        {
          if (error) 
          {
            wrapped_session->state = session_wrapper::stopped;
            active_sessions_.erase(wrapped_session);
            recycle_session(wrapped_session);
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
          wrapped_session->state = session_wrapper::started;
          // Check for pending session manager stop operation 
          if (stop_in_progress == state_)  
          {                            
            stop_session(wrapped_session);
            return;
          }
          // Wait until session needs to stop
          wait_session(wrapped_session);          
          return;
        }
        // Handler is called too late - complete handler's waiters
        recycle_session(wrapped_session);
        // Check for pending session manager stop operation 
        if (stop_in_progress == state_) 
        {
          if (may_complete_stop())  
          {
            complete_stop();
            post_stop_handler();
          }
        }        
      }

      void session_manager::dispatch_session_wait(
        const session_manager_weak_ptr& this_weak_ptr,
        const session_wrapper_ptr& wrapped_session, 
        const boost::system::error_code& error)
      {
        // Try to lock the manager
        if (session_manager_ptr this_ptr = this_weak_ptr.lock())
        {
          // Forward invocation
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
          this_ptr->strand_.dispatch(
            make_custom_alloc_handler(wrapped_session->start_wait_allocator,
              session_handler_binder(&this_type::handle_session_wait, this_ptr, wrapped_session, error)));
#else
          this_ptr->strand_.dispatch(
            make_custom_alloc_handler(wrapped_session->start_wait_allocator,
              boost::bind(&this_type::handle_session_wait, this_ptr, wrapped_session, error)));
#endif
        }
      }

      void session_manager::handle_session_wait(
        const session_wrapper_ptr& wrapped_session, 
        const boost::system::error_code& /*error*/)
      {
        // Unregister pending operation
        --pending_operations_;
        --wrapped_session->pending_operations;
        // Check if handler is not called too late
        if (session_wrapper::started == wrapped_session->state)
        {
          stop_session(wrapped_session);
          return;
        }
        // Handler is called too late - complete handler's waiters
        recycle_session(wrapped_session);
        // Check for pending session manager stop operation
        if (stop_in_progress == state_)
        {
          if (may_complete_stop())  
          {
            complete_stop();
            post_stop_handler();
          }          
        }        
      }

      void session_manager::dispatch_session_stop(
        const session_manager_weak_ptr& this_weak_ptr,
        const session_wrapper_ptr& wrapped_session, 
        const boost::system::error_code& error)
      {     
        // Try to lock the manager
        if (session_manager_ptr this_ptr = this_weak_ptr.lock())
        {
          // Forward invocation
#if defined(MA_HAS_RVALUE_REFS) && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
          this_ptr->strand_.dispatch(
            make_custom_alloc_handler(wrapped_session->stop_allocator,
              session_handler_binder(&this_type::handle_session_stop, this_ptr, wrapped_session, error)));
#else
          this_ptr->strand_.dispatch(
            make_custom_alloc_handler(wrapped_session->stop_allocator,
              boost::bind(&this_type::handle_session_stop, this_ptr, wrapped_session, error)));
#endif
        }
      }

      void session_manager::handle_session_stop(
        const session_wrapper_ptr& wrapped_session,
        const boost::system::error_code& /*error*/)
      {
        // Unregister pending operation
        --pending_operations_;
        --wrapped_session->pending_operations;
        // Check if handler is not called too late
        if (session_wrapper::stop_in_progress == wrapped_session->state)        
        {
          wrapped_session->state = session_wrapper::stopped;
          active_sessions_.erase(wrapped_session);
          recycle_session(wrapped_session);
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
        recycle_session(wrapped_session);
        // Check for pending session manager stop operation
        if (stop_in_progress == state_)
        {
          if (may_complete_stop())  
          {
            complete_stop();
            post_stop_handler();
          }          
        }                
      }

      void session_manager::recycle_session(const session_wrapper_ptr& wrapped_session)
      {
        // Check session's pending operation number and recycle bin size
        if (0 == wrapped_session->pending_operations
          && recycled_sessions_.size() < config_.recycled_session_count)
        {
          // Reset session state
          wrapped_session->the_session->reset();
          // Reset wrapper
          wrapped_session->state = session_wrapper::ready_to_start;
          // Add to recycle bin
          recycled_sessions_.push_front(wrapped_session);
        }
      }

      void session_manager::post_stop_handler()
      {
        if (stop_handler_.has_target()) 
        {
          // Signal shutdown completion
          stop_handler_.post(stop_error_);
        }
      }

    } // namespace server
  } // namespace echo
} // namespace ma

