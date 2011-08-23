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
    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        forward_handler_binder<handler_type>(
            &this_type::begin_start<handler_type>, shared_from_this())));
  }

  template <typename Handler>
  void async_stop(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        forward_handler_binder<handler_type>(
            &this_type::begin_stop<handler_type>, shared_from_this()))); 
  }

  template <typename Handler>
  void async_wait(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        forward_handler_binder<handler_type>(
            &this_type::begin_wait<handler_type>, shared_from_this())));  
  }

#else // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  template <typename Handler>
  void async_start(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        boost::bind(&this_type::begin_start<handler_type>, 
            shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_stop(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        boost::bind(&this_type::begin_stop<handler_type>, 
            shared_from_this(), _1))); 
  }

  template <typename Handler>
  void async_wait(Handler&& handler)
  {
    typedef typename ma::remove_cv_reference<Handler>::type handler_type;
    strand_.post(make_context_alloc_handler2(std::forward<Handler>(handler), 
        boost::bind(&this_type::begin_wait<handler_type>, 
            shared_from_this(), _1)));  
  }

#endif // defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

#else // defined(MA_HAS_RVALUE_REFS)

  template <typename Handler>
  void async_start(const Handler& handler)
  {
    strand_.post(make_context_alloc_handler2(handler, 
        boost::bind(&this_type::begin_start<Handler>, 
            shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_stop(const Handler& handler)
  {
    strand_.post(make_context_alloc_handler2(handler, 
        boost::bind(&this_type::begin_stop<Handler>, shared_from_this(), _1)));
  }

  template <typename Handler>
  void async_wait(const Handler& handler)
  {
    strand_.post(make_context_alloc_handler2(handler, 
        boost::bind(&this_type::begin_wait<Handler>, shared_from_this(), _1)));
  }

#endif // defined(MA_HAS_RVALUE_REFS)

private:        
  struct  session_data;
  typedef boost::shared_ptr<session_data> session_data_ptr;
  typedef boost::weak_ptr<session_data>   session_data_weak_ptr;      

#if defined(MA_HAS_RVALUE_REFS) \
    && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)

  // Home-grown binders to support move semantic
  class accept_handler_binder;
  class session_dispatch_binder;
  class session_handler_binder;

  friend class accept_handler_binder;
  friend class session_dispatch_binder;
  friend class session_handler_binder;

  template <typename Arg>
  class forward_handler_binder
  {
  private:
    typedef forward_handler_binder this_type;

  public:
    typedef void result_type;
    typedef void (session_manager::*function_type)(const Arg&);

    template <typename SessionManagerPtr>
    forward_handler_binder(function_type function, 
        SessionManagerPtr&& the_session_manager)
      : function_(function)
      , session_manager_(std::forward<SessionManagerPtr>(the_session_manager))
    {
    }

#if defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

    forward_handler_binder(this_type&& other)
      : function_(other.function_)
      , session_manager_(std::move(other.session_manager_))
    {
    }

    forward_handler_binder(const this_type& other)
      : function_(other.function_)
      , session_manager_(other.session_manager_)
    {
    }

#endif // defined(MA_USE_EXPLICIT_MOVE_CONSTRUCTOR)

    void operator()(const Arg& arg)
    {
      ((*session_manager_).*function_)(arg);
    }

  private:
    function_type function_;
    session_manager_ptr session_manager_;          
  }; // class forward_handler_binder

#endif // defined(MA_HAS_RVALUE_REFS) 
       //     && defined(MA_BOOST_BIND_HAS_NO_MOVE_CONTRUCTOR)
        
  struct session_data : private boost::noncopyable
  {
    enum state_type
    {
      ready,
      start,
      work,
      stop,
      stopped
    }; // enum state_type

    state_type  state;
    std::size_t pending_operations;

    session_data_weak_ptr prev;
    session_data_ptr      next;

    session_ptr             managed_session;        
    protocol_type::endpoint remote_endpoint;

    in_place_handler_allocator<144> start_wait_allocator;
    in_place_handler_allocator<144> stop_allocator;

    session_data(boost::asio::io_service&, const session_config&);

    ~session_data()
    {
    }
  }; // struct session_data

  class session_data_list : private boost::noncopyable
  {
  public:
    session_data_list()
      : size_(0)
    {
    }

    ~session_data_list()
    {
    }

    void push_front(const session_data_ptr& value);
    void erase(const session_data_ptr& value);          
    void clear();

    std::size_t size() const
    {
      return size_;
    }

    bool empty() const
    {
      return 0 == size_;
    }

    session_data_ptr front() const
    {
      return front_;
    }

  private:
    std::size_t size_;
    session_data_ptr front_;
  }; // class session_data_list

  struct external_state
  {
    enum value_t
    {
      ready,
      start,
      work,
      stop,
      stopped
    };
  }; // struct external_state
  
  template <typename Handler>
  void begin_start(const Handler& handler)
  {
    boost::system::error_code error = start();
    io_service_.post(detail::bind_handler(handler, error));          
  }

  template <typename Handler>
  void begin_stop(const Handler& handler)
  {          
    if (boost::optional<boost::system::error_code> result = stop())
    {          
      io_service_.post(detail::bind_handler(handler, *result));
    }
    else
    {
      stop_handler_.put(handler);
    }
  }

  template <typename Handler>
  void begin_wait(const Handler& handler)
  {          
    if (boost::optional<boost::system::error_code> result = wait())
    {          
      io_service_.post(detail::bind_handler(handler, *result));
    }
    else
    {          
      wait_handler_.put(handler);
    }  
  }

  boost::system::error_code start();
  boost::optional<boost::system::error_code> stop();
  boost::optional<boost::system::error_code> wait();

  session_data_ptr create_session(boost::system::error_code& error);
  void accept_session(const session_data_ptr&);
  void continue_accept();
  void handle_session_accept(const session_data_ptr&, 
      const boost::system::error_code&);

  static void open(protocol_type::acceptor& acceptor, 
      const protocol_type::endpoint& endpoint, int backlog, 
      boost::system::error_code& error);

  bool may_complete_stop() const;
  bool may_complete_wait() const;
  bool may_continue_accept() const;
  void complete_stop();  
  void set_wait_error(const boost::system::error_code&);
        
  void start_session(const session_data_ptr&);
  void stop_session(const session_data_ptr&);
  void wait_session(const session_data_ptr&);

  static void dispatch_session_start(const session_manager_weak_ptr&,
      const session_data_ptr&, const boost::system::error_code&);
  void handle_session_start(const session_data_ptr&, 
      const boost::system::error_code&);

  static void dispatch_session_wait(const session_manager_weak_ptr&,
      const session_data_ptr&, const boost::system::error_code&);
  void handle_session_wait(const session_data_ptr&, 
      const boost::system::error_code&);

  static void dispatch_session_stop(const session_manager_weak_ptr&,
      const session_data_ptr&, const boost::system::error_code&);
  void handle_session_stop(const session_data_ptr&, 
      const boost::system::error_code&);

  void recycle_session(const session_data_ptr&);
  void post_stop_handler();

  protocol_type::endpoint accepting_endpoint_;
  const int               listen_backlog_;
  const std::size_t       max_session_count_;
  const std::size_t       recycled_session_count_;
  const session_config    managed_session_config_;

  bool                    accept_in_progress_;
  std::size_t             pending_operations_;
  external_state::value_t external_state_;        

  boost::asio::io_service&        io_service_;
  boost::asio::io_service&        session_io_service_;
  boost::asio::io_service::strand strand_;      
  protocol_type::acceptor         acceptor_;
  session_data_list               active_sessions_;
  session_data_list               recycled_sessions_;
  boost::system::error_code       wait_error_;
  boost::system::error_code       stop_error_;

  handler_storage<boost::system::error_code> wait_handler_;
  handler_storage<boost::system::error_code> stop_handler_;
                
  in_place_handler_allocator<512> accept_allocator_;
}; // class session_manager

} // namespace server
} // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_HPP
