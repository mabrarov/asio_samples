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
#include <boost/utility/addressof.hpp>
#include <ma/config.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/handler_invoke_helpers.hpp>
#include <ma/strand_wrapped_handler.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/session.hpp>
#include <ma/echo/server/session_manager.hpp>

namespace ma {

namespace echo {

namespace server {

#if defined(MA_HAS_RVALUE_REFS) \
  && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

// Home-grown binders to support move semantic
class session_manager::accept_handler_binder
{
private:
  typedef accept_handler_binder this_type;

public:
  typedef void result_type;

  typedef void (session_manager::*func_type)(
      const session_manager::session_wrapper_ptr&,
      const boost::system::error_code&);

  template <typename SessionManagerPtr, typename SessionWrapperPtr>
  accept_handler_binder(func_type func, SessionManagerPtr&& session_manager,
      SessionWrapperPtr&& session)
    : func_(func)
    , session_manager_(std::forward<SessionManagerPtr>(session_manager))
    , session_(std::forward<SessionWrapperPtr>(session))
  {
  }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  accept_handler_binder(this_type&& other)
    : func_(other.func_)
    , session_manager_(std::move(other.session_manager_))
    , session_(std::move(other.session_))
  {
  }

  accept_handler_binder(const this_type& other)
    : func_(other.func_)
    , session_manager_(other.session_manager_)
    , session_(other.session_)
  {
  }

#endif

  void operator()(const boost::system::error_code& error)
  {
    ((*session_manager_).*func_)(session_, error);
  }

private:
  func_type func_;
  session_manager_ptr session_manager_;
  session_manager::session_wrapper_ptr session_;
}; // class session_manager::accept_handler_binder

class session_manager::session_dispatch_binder
{
private:
  typedef session_dispatch_binder this_type;

public:
  typedef void result_type;

  typedef void (*func_type)(const session_manager_weak_ptr&,
    const session_manager::session_wrapper_ptr&, 
    const boost::system::error_code&);

  template <typename SessionManagerPtr, typename SessionWrapperPtr>
  session_dispatch_binder(func_type func, SessionManagerPtr&& session_manager,
      SessionWrapperPtr&& session)
    : func_(func)
    , session_manager_(std::forward<SessionManagerPtr>(session_manager))
    , session_(std::forward<SessionWrapperPtr>(session))
  {
  }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  session_dispatch_binder(this_type&& other)
    : func_(other.func_)
    , session_manager_(std::move(other.session_manager_))
    , session_(std::move(other.session_))
  {
  }

  session_dispatch_binder(const this_type& other)
    : func_(other.func_)
    , session_manager_(other.session_manager_)
    , session_(other.session_)
  {
  }

#endif

  void operator()(const boost::system::error_code& error)
  {
    func_(session_manager_, session_, error);
  }

private:
  func_type func_;
  session_manager_weak_ptr session_manager_;
  session_manager::session_wrapper_ptr session_;
}; // class session_manager::session_dispatch_binder      

class session_manager::session_handler_binder
{
private:
  typedef session_handler_binder this_type;

public:
  typedef void result_type;

  typedef void (session_manager::*func_type)(
      const session_manager::session_wrapper_ptr&,
      const boost::system::error_code&);

  template <typename SessionManagerPtr, typename SessionWrapperPtr>
  session_handler_binder(func_type func, SessionManagerPtr&& session_manager,
      SessionWrapperPtr&& session, const boost::system::error_code& error)
    : func_(func)
    , session_manager_(std::forward<SessionManagerPtr>(session_manager))
    , session_(std::forward<SessionWrapperPtr>(session))
    , error_(error)
  {
  }                  

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

  session_handler_binder(this_type&& other)
    : func_(other.func_)
    , session_manager_(std::move(other.session_manager_))
    , session_(std::move(other.session_))
    , error_(std::move(other.error_))
  {
  }

  session_handler_binder(const this_type& other)
    : func_(other.func_)
    , session_manager_(other.session_manager_)
    , session_(other.session_)
    , error_(other.error_)
  {
  }

#endif

  void operator()()
  {
    ((*session_manager_).*func_)(session_, error_);
  }

private:
  func_type func_;
  session_manager_ptr session_manager_;
  session_manager::session_wrapper_ptr session_;
  boost::system::error_code error_;
}; // class session_manager::session_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS) 
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

struct session_manager::session_wrapper : private boost::noncopyable
{
  typedef session_manager::protocol_type::endpoint endpoint_type;
  typedef server::session::protocol_type::socket   socket_type;

  struct state_type
  {
    enum value_t {ready, start, work, stop, stopped};
  };

  session_manager::session_wrapper_weak_ptr prev;
  session_manager::session_wrapper_ptr      next;

  endpoint_type       remote_endpoint;
  server::session_ptr session;
  state_type::value_t state;
  std::size_t         pending_operations;

  in_place_handler_allocator<144> start_wait_allocator;
  in_place_handler_allocator<144> stop_allocator;
  
  session_wrapper(boost::asio::io_service& io_service, 
      const session_config& config)
    : session(boost::make_shared<server::session>(
          boost::ref(io_service), config))
    , state(state_type::ready)
    , pending_operations(0)
  {
  }

#if !defined(NDEBUG)
  ~session_wrapper()
  {
    BOOST_ASSERT(!next && !prev.lock());
  }
#endif

  socket_type& socket()
  {
    return session->socket();
  }

  bool has_pending_operations() const
  {
    return 0 != pending_operations;
  }

  bool is_starting() const
  {
    return state_type::start == state;
  }

  bool is_stopping() const
  {
    return state_type::stop == state;
  }

  bool is_working() const
  {
    return state_type::work == state;
  }

  void handle_operation_completion()
  {
    --pending_operations;
  }

  void mark_as_stopped()
  {
    state = state_type::stopped;
  }

  void mark_as_working()
  {
    state = state_type::work;
  }

  void reset()
  {      
    session->reset();
    state = state_type::ready;
    pending_operations = 0;
  }

#if defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_start(Handler&& handler)
  {    
    session->async_start(std::forward<Handler>(handler));
    state = state_type::start;
    ++pending_operations;
  }

  template <typename Handler>
  void async_stop(Handler&& handler)
  {
    session->async_stop(std::forward<Handler>(handler));      
    state = state_type::stop;
    ++pending_operations;
  }

  template <typename Handler>
  void async_wait(Handler&& handler)
  {
    session->async_wait(std::forward<Handler>(handler));
    ++pending_operations;
  }

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_start(const Handler& handler)
  {    
    session->async_start(handler);
    state = state_type::start;
    ++pending_operations;
  }

  template <typename Handler>
  void async_stop(const Handler& handler)
  {
    session->async_stop(handler);      
    state = state_type::stop;
    ++pending_operations;
  }

  template <typename Handler>
  void async_wait(const Handler& handler)
  {
    session->async_wait(handler);
    ++pending_operations;
  }

#endif // defined(MA_HAS_RVALUE_REFS)    
    
}; // struct session_manager::session_wrapper

void session_manager::session_list::push_front(
    const session_wrapper_ptr& value)
{
  BOOST_ASSERT((!value->next) && (!value->prev.lock()));

  value->next = front_;  
  if (front_)
  {
    front_->prev = value;
  }
  front_ = value;  
  ++size_;
}

void session_manager::session_list::erase(const session_wrapper_ptr& value)
{
  if (value == front_)
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

  BOOST_ASSERT((!value->next) && (!value->prev.lock()));
}

void session_manager::session_list::clear()
{
  // We don't want to have recusrive calls of wrapped_session's destructor
  // because the deep of such recursion may be equal to the size of list.
  // The last can be too great for the stack.
  while (front_)
  {
    session_wrapper_ptr tmp = front_->next;
    front_->prev.reset();
    front_->next.reset();
    front_ = tmp;    
  }
  size_ = 0;
}

session_manager::session_manager(boost::asio::io_service& io_service, 
    boost::asio::io_service& session_io_service,
    const session_manager_config& config)
  : accepting_endpoint_(config.accepting_endpoint)
  , listen_backlog_(config.listen_backlog)
  , max_session_count_(config.max_session_count)
  , recycled_session_count_(config.recycled_session_count)
  , managed_session_config_(config.managed_session_config)
  , extern_state_(extern_state::ready)
  , intern_state_(intern_state::work)
  , accept_state_(accept_state::ready)
  , pending_operations_(0)
  , io_service_(io_service)
  , session_io_service_(session_io_service)
  , strand_(io_service)
  , acceptor_(io_service)        
  , extern_wait_handler_(io_service)
  , extern_stop_handler_(io_service)        
{          
}

void session_manager::reset(bool free_recycled_sessions)
{
  extern_state_ = extern_state::ready;
  intern_state_ = intern_state::work;
  accept_state_ = accept_state::ready;  
  pending_operations_ = 0;  
  
  close_acceptor();

  active_sessions_.clear();
  if (free_recycled_sessions)
  {
    recycled_sessions_.clear();
  }

  extern_wait_error_.clear();
}

boost::system::error_code session_manager::do_start_extern_start()
{
  // Check exetrnal state consistency
  if (extern_state::ready != extern_state_)
  {
    return server_error::invalid_state;
  }
    
  // Internal states have right values already
  extern_state_ = extern_state::work;
  continue_work();

  if (intern_state_ == intern_state::stopped)
  {
    extern_state_ = extern_state::stopped;
    return extern_wait_error_;
  }

  // Notify start handler about success
  return boost::system::error_code();
}

boost::optional<boost::system::error_code>
session_manager::do_start_extern_stop()
{
  // Check exetrnal state consistency
  if ((extern_state::stopped == extern_state_)
      || (extern_state::stop == extern_state_))
  {          
    return boost::system::error_code(server_error::invalid_state);
  }
  
  // Switch external SM
  extern_state_ = extern_state::stop;  
  complete_extern_wait(server_error::operation_aborted);
  
  if (intern_state::work == intern_state_)
  {
    start_stop(server_error::operation_aborted);
  }

  // intern_state_ can be changed by start_stop
  if (intern_state::stopped == intern_state_)
  {
    extern_state_ = extern_state::stopped;
    // Notify stop handler about success
    return boost::system::error_code();
  }

  // Park stop handler for the late call
  return boost::optional<boost::system::error_code>();  
}

boost::optional<boost::system::error_code>
session_manager::do_start_extern_wait()
{
  // Check exetrnal state consistency
  if ((extern_state::work != extern_state_)
      || extern_wait_handler_.has_target())
  {
    return boost::system::error_code(server_error::invalid_state);
  }
  
  if (intern_state::work != intern_state_)
  {
    return extern_wait_error_;
  }

  // Park wait handler for the late call
  return boost::optional<boost::system::error_code>();
}

void session_manager::complete_extern_stop(
    const boost::system::error_code& error)
{
  if (extern_stop_handler_.has_target())
  {
    extern_stop_handler_.post(error);
  }
}

void session_manager::complete_extern_wait(
    const boost::system::error_code& error)
{
  // Register error if there was no work completion error registered before
  if (!extern_wait_error_)
  {
    extern_wait_error_ = error;
  }
  if (extern_wait_handler_.has_target())
  {
    extern_wait_handler_.post(extern_wait_error_);
  }
}

void session_manager::continue_work()
{
  BOOST_ASSERT_MSG(intern_state::work == intern_state_,
      "invalid internal state");

  if (is_out_of_work())
  {
    start_stop(server_error::out_of_work);      
    return;
  }

  if (accept_state::ready != accept_state_)
  {
    // Can't start more accept operations - no ready acceptors
    return;
  }

  if (active_sessions_.size() >= max_session_count_)
  {    
    // Can't start more accept operations - no space
    if (acceptor_.is_open())
    {
      close_acceptor();
    }
    return;
  }

  // Prepare (open) acceptor
  if (!acceptor_.is_open())
  {
    if (boost::system::error_code error = open_acceptor())
    {
      accept_state_ = accept_state::stopped;
      if (is_out_of_work())
      {
        start_stop(server_error::out_of_work);
      }
      return;
    }
  }

  // Get new, ready to start session
  boost::system::error_code error;
  session_wrapper_ptr session = create_session(error);
  if (error)
  {
    if (session)
    {
      recycle(session);
    }
    if (!active_sessions_.empty())
    {
      // Try later
      return;
    }  
    accept_state_ = accept_state::stopped;
    if (is_out_of_work())
    {
      start_stop(server_error::out_of_work);
    }
    return;
  }

  start_accept_session(session);
}

void session_manager::handle_accept(const session_wrapper_ptr& session,
    const boost::system::error_code& error)
{
  BOOST_ASSERT_MSG(accept_state::in_progress == accept_state_,
      "invalid accept state");

  // Split handler based on current internal state 
  // that might change during accept operation
  switch (intern_state_)
  {
  case intern_state::work:
    handle_accept_at_work(session, error);
    break;  

  case intern_state::stop:
    handle_accept_at_stop(session, error);
    break;

  default:
    BOOST_ASSERT_MSG(false, "invalid internal state");
    break;
  }
}

void session_manager::handle_accept_at_work(const session_wrapper_ptr& session,
    const boost::system::error_code& error)
{
  BOOST_ASSERT_MSG(intern_state::work == intern_state_, 
      "invalid internal state");

  BOOST_ASSERT_MSG(accept_state::in_progress == accept_state_,
      "invalid accept state");

  // Unregister pending operation
  --pending_operations_;
  accept_state_ = accept_state::ready;
    
  if (error)
  {
    accept_state_ = accept_state::stopped;
    recycle(session);
    continue_work();
    return;
  }

  if (active_sessions_.size() >= max_session_count_)
  {
    recycle(session);
    continue_work();
    return;
  }     

  active_sessions_.push_front(session);
  start_session_start(session);
  continue_work();
}

void session_manager::handle_accept_at_stop(const session_wrapper_ptr& session,
    const boost::system::error_code& /*error*/)
{
  BOOST_ASSERT_MSG(intern_state::stop == intern_state_, 
      "invalid internal state");

  BOOST_ASSERT_MSG(accept_state::in_progress == accept_state_,
      "invalid accept state");
  
  --pending_operations_;
  accept_state_ = accept_state::stopped;
  
  recycle(session);
  continue_stop();  
}

void session_manager::handle_session_start(const session_wrapper_ptr& session, 
    const boost::system::error_code& error)
{
  // Split handler based on current internal state 
  // that might change during session start
  switch (intern_state_)
  {
  case intern_state::work:
    handle_session_start_at_work(session, error);
    break;  

  case intern_state::stop:
    handle_session_start_at_stop(session, error);
    break;

  default:
    BOOST_ASSERT_MSG(false, "invalid internal state");
    break;
  }
}

void session_manager::handle_session_start_at_work(
    const session_wrapper_ptr& session, const boost::system::error_code& error)
{
  BOOST_ASSERT_MSG(intern_state::work == intern_state_, 
      "invalid internal state");
  
  --pending_operations_;
  session->handle_operation_completion();  
  
  if (!session->is_starting())
  {
    // Handler is called too late
    recycle(session);
    continue_work();
    return;
  }

  if (error)
  {
    // Failed to start accepted session
    session->mark_as_stopped();
    active_sessions_.erase(session);
    recycle(session);
    continue_work();
    return;
  }

  // Accepted session started successfully
  session->mark_as_working();
  start_session_wait(session);
  continue_work();
}

void session_manager::handle_session_start_at_stop(
    const session_wrapper_ptr& session, const boost::system::error_code& error)
{
  BOOST_ASSERT_MSG(intern_state::stop == intern_state_, 
      "invalid internal state");
  
  --pending_operations_;
  session->handle_operation_completion();
  
  if (!session->is_starting())
  {
    // Handler is called too late
    recycle(session);
    continue_stop();
    return;
  }

  if (error)
  {
    // Failed to start accepted session
    session->mark_as_stopped();
    active_sessions_.erase(session);
    recycle(session);
    continue_stop();
    return;
  }

  // Accepted session started successfully
  start_session_stop(session);
  continue_stop();
}

void session_manager::handle_session_wait(const session_wrapper_ptr& session,
    const boost::system::error_code& error)
{
  // Split handler based on current internal state 
  // that might change during session wait
  switch (intern_state_)
  {
  case intern_state::work:
    handle_session_wait_at_work(session, error);
    break;  

  case intern_state::stop:
    handle_session_wait_at_stop(session, error);
    break;

  default:
    BOOST_ASSERT_MSG(false, "invalid internal state");
    break;
  }
}

void session_manager::handle_session_wait_at_work(
    const session_wrapper_ptr& session,
    const boost::system::error_code& /*error*/)
{
  BOOST_ASSERT_MSG(intern_state::work == intern_state_, 
      "invalid internal state");
  
  --pending_operations_;
  session->handle_operation_completion();
   
  if (!session->is_working())
  {
    // Handler is called too late
    recycle(session);
    continue_work();
    return;
  }

  // Session run out of work - stop it
  start_session_stop(session);
  continue_work();
}

void session_manager::handle_session_wait_at_stop(
    const session_wrapper_ptr& session, 
    const boost::system::error_code& /*error*/)
{
  BOOST_ASSERT_MSG(intern_state::stop == intern_state_, 
      "invalid internal state");
  
  --pending_operations_;
  session->handle_operation_completion();
  
  if (!session->is_working())
  {
    // Handler is called too late
    recycle(session); 
    continue_stop();
    return;
  }

  // Session run out of work - stop it
  start_session_stop(session);
  continue_stop();
}

void session_manager::handle_session_stop(const session_wrapper_ptr& session,
    const boost::system::error_code& error)
{
  // Split handler based on current internal state 
  // that might change during session stop
  switch (intern_state_)
  {
  case intern_state::work:
    handle_session_stop_at_work(session, error);
    break;  

  case intern_state::stop:
    handle_session_stop_at_stop(session, error);
    break;

  default:
    BOOST_ASSERT_MSG(false, "invalid internal state");
    break;
  }
}

void session_manager::handle_session_stop_at_work(
    const session_wrapper_ptr& session, const boost::system::error_code& error)
{
  BOOST_ASSERT_MSG(intern_state::work == intern_state_, 
      "invalid internal state");
  
  --pending_operations_;
  session->handle_operation_completion();
  
  if (!session->is_stopping())
  {
    // Handler is called too late
    recycle(session);
    continue_work();
    return;
  }

  // Failed to stop working session.
  // The only reason is "double stop operations".
  // It is prevented by usage of session_wrapper::state.
  BOOST_ASSERT_MSG(!error, "session::async_stop failed");
  // Prevent warning at release build
  (void) error;
  
  // Session has stopped successfully
  session->mark_as_stopped();
  active_sessions_.erase(session);
  recycle(session);
  continue_work();
}

void session_manager::handle_session_stop_at_stop(
    const session_wrapper_ptr& session, const boost::system::error_code& error)
{
  BOOST_ASSERT_MSG(intern_state::stop == intern_state_, 
      "invalid internal state");
  
  --pending_operations_;
  session->handle_operation_completion();

  if (!session->is_stopping())
  {
    // Handler is called too late
    recycle(session);
    continue_stop();
    return;
  }

  // Failed to stop working session.
  // The only reason is "double stop operations".
  // It is prevented by usage of session_wrapper::state.
  BOOST_ASSERT_MSG(!error, "session::async_stop failed");
  // Prevent warning at release build
  (void) error;

  session->mark_as_stopped();
  active_sessions_.erase(session);
  recycle(session);
  continue_stop();
}

bool session_manager::is_out_of_work() const
{
  return active_sessions_.empty() && (accept_state::stopped == accept_state_);
}

void session_manager::start_stop(const boost::system::error_code& error)
{
  // Switch general internal SM
  intern_state_ = intern_state::stop;

  // Close acceptors. Additionally it will help to stop accept operations.
  if (acceptor_.is_open())
  {
    close_acceptor();
  }

  // Stop all active sessions
  session_wrapper_ptr session = active_sessions_.front();
  while (session)
  {
    if (!session->is_stopping())
    {
      start_session_stop(session);
    }
    session = session->next;
  }  

  // Switch all internal SMs to the right states
  if (accept_state::ready == accept_state_)
  {
    accept_state_ = accept_state::stopped;
  }

  // Notify external wait handler if need
  if (extern_state::work == extern_state_)
  {
    complete_extern_wait(error);
  }

  continue_stop();  
}

void session_manager::continue_stop()
{
  BOOST_ASSERT_MSG(intern_state::stop == intern_state_,
      "invalid internal state");

  if (!pending_operations_)
  {
    BOOST_ASSERT_MSG(accept_state::stopped  == accept_state_,
      "invalid accept state");

    BOOST_ASSERT_MSG(active_sessions_.empty(),
      "there are still some active sessions");
    
    // Internal stop completed
    intern_state_ = intern_state::stopped;
    
    // Notify external stop handler if need
    if (extern_state::stop == extern_state_)
    {
      extern_state_ = extern_state::stopped;
      complete_extern_stop(boost::system::error_code());
    }    
  }
}

void session_manager::start_accept_session(const session_wrapper_ptr& session)
{
#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  acceptor_.async_accept(session->socket(), session->remote_endpoint, 
      MA_STRAND_WRAP(strand_, make_custom_alloc_handler(accept_allocator_, 
          accept_handler_binder(&this_type::handle_accept, shared_from_this(),
              session))));

#else

  acceptor_.async_accept(session->socket(), session->remote_endpoint, 
      MA_STRAND_WRAP(strand_, make_custom_alloc_handler(accept_allocator_, 
          boost::bind(&this_type::handle_accept, shared_from_this(), session, 
              boost::asio::placeholders::error))));

#endif

  accept_state_ = accept_state::in_progress;
  ++pending_operations_;
}

void session_manager::start_session_start(const session_wrapper_ptr& session)
{ 
  // Asynchronously start wrapped session
#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  session->async_start(make_custom_alloc_handler(session->start_wait_allocator, 
      session_dispatch_binder(&this_type::dispatch_handle_session_start, 
          shared_from_this(), session)));

#else

  session->async_start(make_custom_alloc_handler(session->start_wait_allocator, 
      boost::bind(&this_type::dispatch_handle_session_start, 
          session_manager_weak_ptr(shared_from_this()), session, _1)));

#endif

  ++pending_operations_;
}

void session_manager::start_session_stop(const session_wrapper_ptr& session)
{
  // Asynchronously stop wrapped session
#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  session->async_stop(make_custom_alloc_handler(session->stop_allocator, 
      session_dispatch_binder(&this_type::dispatch_handle_session_stop, 
          shared_from_this(), session)));

#else

  session->async_stop(make_custom_alloc_handler(session->stop_allocator, 
      boost::bind(&this_type::dispatch_handle_session_stop, 
          session_manager_weak_ptr(shared_from_this()), session, _1)));

#endif

  ++pending_operations_;
}

void session_manager::start_session_wait(const session_wrapper_ptr& session)
{
  // Asynchronously wait on wrapped session
#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  session->async_wait(make_custom_alloc_handler(session->start_wait_allocator, 
      session_dispatch_binder(&this_type::dispatch_handle_session_wait, 
          shared_from_this(), session)));

#else

  session->async_wait(make_custom_alloc_handler(session->start_wait_allocator, 
      boost::bind(&this_type::dispatch_handle_session_wait, 
          session_manager_weak_ptr(shared_from_this()), session, _1)));

#endif

  ++pending_operations_;
}

void session_manager::recycle(const session_wrapper_ptr& session)
{
  BOOST_ASSERT_MSG(session, "session must be not null");

  bool is_recyclable = !session->has_pending_operations();
  bool has_recycle_space = recycled_sessions_.size() < recycled_session_count_;
  
  if (is_recyclable && has_recycle_space)
  {
    session->reset();
    // Put the session to "recycle bin"
    recycled_sessions_.push_front(session);
  } 
}

session_manager::session_wrapper_ptr session_manager::create_session(
    boost::system::error_code& error)
{        
  if (!recycled_sessions_.empty())
  {
    session_wrapper_ptr session = recycled_sessions_.front();
    recycled_sessions_.erase(session);          
    error = boost::system::error_code();
    return session;
  }

  try 
  {   
    session_wrapper_ptr session = boost::make_shared<session_wrapper>(
        boost::ref(session_io_service_), managed_session_config_);
    error = boost::system::error_code();
    return session;
  }
  catch (const std::bad_alloc&) 
  {
    error = server_error::no_memory;
    return session_wrapper_ptr();
  }
}

boost::system::error_code session_manager::open_acceptor()
{
  boost::system::error_code error;
  open(acceptor_, accepting_endpoint_, listen_backlog_, error);  
  return error;
}

boost::system::error_code session_manager::close_acceptor()
{
  boost::system::error_code error;
  acceptor_.close(error);
  return error;
}

void session_manager::dispatch_handle_session_start(
    const session_manager_weak_ptr& this_weak_ptr, 
    const session_wrapper_ptr& session, 
    const boost::system::error_code& error)
{
  // Try to lock the session manager
  if (session_manager_ptr this_ptr = this_weak_ptr.lock())
  {
    // Forward completion
#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

    this_ptr->strand_.dispatch(make_custom_alloc_handler(
        session->start_wait_allocator, session_handler_binder(
            &session_manager::handle_session_start, this_ptr, session,
                error)));

#else

    this_ptr->strand_.dispatch(make_custom_alloc_handler(
        session->start_wait_allocator, boost::bind(
            &session_manager::handle_session_start, this_ptr, session, 
                error)));

#endif
  }
}

void session_manager::dispatch_handle_session_wait(
    const session_manager_weak_ptr& this_weak_ptr,
    const session_wrapper_ptr& session, 
    const boost::system::error_code& error)
{
  if (session_manager_ptr this_ptr = this_weak_ptr.lock())
  {
    // Forward completion
#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

    this_ptr->strand_.dispatch(make_custom_alloc_handler(
        session->start_wait_allocator, session_handler_binder(
            &session_manager::handle_session_wait, this_ptr, session, error)));

#else

    this_ptr->strand_.dispatch(make_custom_alloc_handler(
        session->start_wait_allocator, boost::bind(
            &session_manager::handle_session_wait, this_ptr, session, error)));

#endif
  }
}

void session_manager::dispatch_handle_session_stop(
    const session_manager_weak_ptr& this_weak_ptr,
    const session_wrapper_ptr& session, 
    const boost::system::error_code& error)
{
  if (session_manager_ptr this_ptr = this_weak_ptr.lock())
  {
    // Forward completion
#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

    this_ptr->strand_.dispatch(make_custom_alloc_handler(
        session->stop_allocator, session_handler_binder(
            &session_manager::handle_session_stop, this_ptr, session, error)));

#else

    this_ptr->strand_.dispatch(make_custom_alloc_handler(
        session->stop_allocator, boost::bind(
            &session_manager::handle_session_stop, this_ptr, session, error)));

#endif
  }
}

void session_manager::open(protocol_type::acceptor& acceptor, 
    const protocol_type::endpoint& endpoint, int backlog, 
    boost::system::error_code& error)
{
  boost::system::error_code local_error;

  acceptor.open(endpoint.protocol(), local_error);
  if (local_error)
  {
    error = local_error;
    return;
  }

  class acceptor_guard : private boost::noncopyable
  {
  public:
    typedef protocol_type::acceptor guarded_type;

    acceptor_guard(guarded_type& guarded)
      : guarded_(boost::addressof(guarded))
    {
    }

    ~acceptor_guard()
    {
      if (guarded_)
      {
        boost::system::error_code ignored;
        guarded_->close(ignored);
      }            
    }

    void release()
    {
      guarded_ = 0;
    }

  private:    
    guarded_type* guarded_;
  }; // class acceptor_guard

  acceptor_guard closing_guard(acceptor);                

  protocol_type::acceptor::reuse_address reuse_address_opt(true);
  acceptor.set_option(reuse_address_opt, local_error);
  if (local_error)
  {       
    error = local_error;
    return;
  }

  acceptor.bind(endpoint, local_error);
  if (local_error)
  {     
    error = local_error;
    return;
  }

  acceptor.listen(backlog, error);

  if (!error)
  {
    closing_guard.release();
  }
}

} // namespace server
} // namespace echo
} // namespace ma
