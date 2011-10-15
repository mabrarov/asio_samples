//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_MANAGER_HPP
#define MA_ECHO_SERVER_SESSION_MANAGER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <ma/config.hpp>
#include <ma/handler_storage.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/bind_asio_handler.hpp>
#include <ma/context_alloc_handler.hpp>
#include <ma/echo/server/session_fwd.hpp>
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session_manager_config.hpp>
#include <ma/echo/server/session_manager_fwd.hpp>

#if defined(MA_HAS_RVALUE_REFS)
#include <utility>
#include <ma/type_traits.hpp>
#endif // defined(MA_HAS_RVALUE_REFS)

namespace ma {   

namespace echo {

namespace server {

class session_manager 
  : private boost::noncopyable
  , public boost::enable_shared_from_this<session_manager>
{
private:
  typedef session_manager this_type;        

public:
  typedef boost::asio::ip::tcp protocol_type;

  session_manager(boost::asio::io_service& io_service, 
      boost::asio::io_service& session_io_service, 
      const session_manager_config& config);

  ~session_manager()
  {        
  }

  void reset(bool free_recycled_sessions = true);

#if defined(MA_HAS_RVALUE_REFS)

#if defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Handler>
  void async_start(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_start<handler_type>;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        forward_handler_binder<handler_type>(func, shared_from_this())));
  }

  template <typename Handler>
  void async_stop(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_stop<handler_type>;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        forward_handler_binder<handler_type>(func, shared_from_this())));
  }

  template <typename Handler>
  void async_wait(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_wait<handler_type>;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        forward_handler_binder<handler_type>(func, shared_from_this())));
  }

#else // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Handler>
  void async_start(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_start<handler_type>;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler),
        boost::bind(func, shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_stop(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_stop<handler_type>;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        boost::bind(func, shared_from_this(), _1))); 
  }

  template <typename Handler>
  void async_wait(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_wait<handler_type>;

    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        boost::bind(func, shared_from_this(), _1)));  
  }

#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_start(const Handler& handler)
  {
    typedef Handler handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_start<handler_type>;

    strand_.post(make_context_alloc_handler2(handler, 
        boost::bind(func, shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_stop(const Handler& handler)
  {
    typedef Handler handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_stop<handler_type>;

    strand_.post(make_context_alloc_handler2(handler, 
        boost::bind(func, shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_wait(const Handler& handler)
  {
    typedef Handler handler_type;
    typedef void (this_type::*func_type)(const handler_type&);

    func_type func = &this_type::start_extern_wait<handler_type>;

    strand_.post(make_context_alloc_handler2(handler, 
        boost::bind(func, shared_from_this(), _1)));
  }

#endif // defined(MA_HAS_RVALUE_REFS)

private:        
  struct  session_wrapper;
  typedef boost::shared_ptr<session_wrapper> wrapped_session_ptr;
  typedef boost::weak_ptr<session_wrapper>   wrapped_session_weak_ptr;
  
#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  // Home-grown binders to support move semantic
  class accept_handler_binder;
  class session_dispatch_binder;
  class session_handler_binder;

  template <typename Arg>
  class forward_handler_binder
  {
  private:
    typedef forward_handler_binder this_type;

  public:
    typedef void result_type;

    typedef void (session_manager::*func_type)(const Arg&);

    template <typename SessionManagerPtr>
    forward_handler_binder(func_type func, SessionManagerPtr&& session_manager)
      : func_(func)
      , session_manager_(std::forward<SessionManagerPtr>(session_manager))
    {
    }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR) || !defined(NDEBUG)

    forward_handler_binder(this_type&& other)
      : func_(other.func_)
      , session_manager_(std::move(other.session_manager_))
    {
    }

    forward_handler_binder(const this_type& other)
      : func_(other.func_)
      , session_manager_(other.session_manager_)
    {
    }

#endif

    void operator()(const Arg& arg)
    {
      ((*session_manager_).*func_)(arg);
    }

  private:
    func_type func_;
    session_manager_ptr session_manager_;          
  }; // class forward_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS) 
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)           

  class session_list : private boost::noncopyable
  {
  public:
    session_list()
      : size_(0)
    {
    }

    ~session_list()
    {
      clear();
    }

    void push_front(const wrapped_session_ptr& value);
    void erase(const wrapped_session_ptr& value);          
    void clear();

    std::size_t size() const
    {
      return size_;
    }

    bool empty() const
    {
      return 0 == size_;
    }

    wrapped_session_ptr front() const
    {
      return front_;
    }

  private:
    std::size_t size_;
    wrapped_session_ptr front_;
  }; // class session_list

  struct extern_state
  {
    enum value_t {ready, work, stop, stopped};
  };

  struct intern_state
  {
    enum value_t {work, stop, stopped};
  };

  struct accept_state
  {
    enum value_t {wait, in_progress, stopped};
  };
  
  template <typename Handler>
  void start_extern_start(const Handler& handler)
  {
    boost::system::error_code error = do_start_extern_start();
    io_service_.post(detail::bind_handler(handler, error));          
  }

  template <typename Handler>
  void start_extern_stop(const Handler& handler)
  {
    if (boost::optional<boost::system::error_code> result = 
        do_start_extern_stop())
    {
      io_service_.post(detail::bind_handler(handler, *result));
    }
    else
    {
      extern_stop_handler_.reset(handler);
    }
  }

  template <typename Handler>
  void start_extern_wait(const Handler& handler)
  {
    if (boost::optional<boost::system::error_code> result = 
        do_start_extern_wait())
    {          
      io_service_.post(detail::bind_handler(handler, *result));
    }
    else
    {
      extern_wait_handler_.reset(handler);
    }  
  }

  boost::system::error_code                  do_start_extern_start();
  boost::optional<boost::system::error_code> do_start_extern_stop();
  boost::optional<boost::system::error_code> do_start_extern_wait();
  void complete_extern_stop(const boost::system::error_code&);
  void complete_extern_wait(const boost::system::error_code&);  

  void continue_work();  

  void handle_accept(const wrapped_session_ptr&, 
      const boost::system::error_code&);
  void handle_accept_at_work(const wrapped_session_ptr&, 
      const boost::system::error_code&);
  void handle_accept_at_stop(const wrapped_session_ptr&, 
      const boost::system::error_code&);

  void handle_session_start(const wrapped_session_ptr&,
      const boost::system::error_code&);
  void handle_session_start_at_work(const wrapped_session_ptr&,
      const boost::system::error_code&);
  void handle_session_start_at_stop(const wrapped_session_ptr&,
      const boost::system::error_code&);

  void handle_session_wait(const wrapped_session_ptr&,
      const boost::system::error_code&);
  void handle_session_wait_at_work(const wrapped_session_ptr&,
      const boost::system::error_code&);
  void handle_session_wait_at_stop(const wrapped_session_ptr&,
      const boost::system::error_code&);

  void handle_session_stop(const wrapped_session_ptr&,
      const boost::system::error_code&);
  void handle_session_stop_at_work(const wrapped_session_ptr&,
      const boost::system::error_code&);
  void handle_session_stop_at_stop(const wrapped_session_ptr&,
      const boost::system::error_code&);

  bool is_out_of_work() const;
  void start_stop(const boost::system::error_code&);
  void continue_stop();
            
  void start_accept_session(const wrapped_session_ptr&);  
  void start_session_start(const wrapped_session_ptr&);
  void start_session_stop(const wrapped_session_ptr&);
  void start_session_wait(const wrapped_session_ptr&);

  void recycle(const wrapped_session_ptr&);
  wrapped_session_ptr create_session(boost::system::error_code& error);

  boost::system::error_code close_acceptor();

  static void dispatch_handle_session_start(const session_manager_weak_ptr&,
      const wrapped_session_ptr&, const boost::system::error_code&);
  static void dispatch_handle_session_wait(const session_manager_weak_ptr&,
      const wrapped_session_ptr&, const boost::system::error_code&);
  static void dispatch_handle_session_stop(const session_manager_weak_ptr&,
      const wrapped_session_ptr&, const boost::system::error_code&);

  static void open(protocol_type::acceptor& acceptor, 
      const protocol_type::endpoint& endpoint, int backlog, 
      boost::system::error_code& error);  

  const protocol_type::endpoint accepting_endpoint_;
  const int                     listen_backlog_;
  const std::size_t             max_session_count_;
  const std::size_t             recycled_session_count_;
  const session_config          managed_session_config_;

  extern_state::value_t extern_state_;
  intern_state::value_t intern_state_;
  accept_state::value_t accept_state_;
  std::size_t           pending_operations_;  

  boost::asio::io_service&        io_service_;
  boost::asio::io_service&        session_io_service_;
  boost::asio::io_service::strand strand_;      
  protocol_type::acceptor         acceptor_;
  session_list                    active_sessions_;
  session_list                    recycled_sessions_;
  boost::system::error_code       extern_wait_error_;

  handler_storage<boost::system::error_code> extern_wait_handler_;
  handler_storage<boost::system::error_code> extern_stop_handler_;
                
  in_place_handler_allocator<512> accept_allocator_;
}; // class session_manager

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_HPP
