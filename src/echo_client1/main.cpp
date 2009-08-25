//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <boost/smart_ptr.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/echo/client1/session.hpp>
#include <console_controller.hpp>

struct session_proxy_type;
typedef boost::shared_ptr<session_proxy_type> session_proxy_ptr;
typedef boost::function<void (void)> exception_handler;

struct session_proxy_type : private boost::noncopyable
{  
  enum state_type
  {
    ready_to_start,
    start_in_progress,
    started,
    stop_in_progress,
    stopped
  };

  boost::mutex mutex_;  
  ma::echo::client1::session_ptr session_;
  state_type state_;
  bool stopped_by_program_exit_;
  boost::condition_variable state_changed_;  
  ma::in_place_handler_allocator<256> start_wait_allocator_;
  ma::in_place_handler_allocator<256> stop_allocator_;

  explicit session_proxy_type(boost::asio::io_service& io_service,    
    const ma::echo::client1::session::settings& settings)
    : session_(new ma::echo::client1::session(io_service, settings))    
    , state_(ready_to_start)
    , stopped_by_program_exit_(false)
  {
  }

  ~session_proxy_type()
  {
  }
}; // session_proxy_type

void start_session(const session_proxy_ptr&);

void wait_session(const session_proxy_ptr&);

void stop_session(const session_proxy_ptr&);

void run_io_service(boost::asio::io_service&, exception_handler);

void handle_work_exception(const session_proxy_ptr&);

void handle_program_exit(const session_proxy_ptr&);

void session_started(const session_proxy_ptr&,
  const boost::system::error_code&);

void session_has_to_stop(const session_proxy_ptr&,
  const boost::system::error_code&);

void session_stopped(const session_proxy_ptr&,
  const boost::system::error_code&);

int _tmain(int argc, _TCHAR* argv[])
{ 
  boost::program_options::options_description options_description("Allowed options");
  options_description.add_options()
    (
      "help", 
      "produce help message"
    );

  int exit_code = EXIT_SUCCESS;
  try 
  {
    boost::program_options::variables_map options_values;  
    boost::program_options::store(
      boost::program_options::parse_command_line(argc, argv, options_description), 
      options_values);
    boost::program_options::notify(options_values);

    if (options_values.count("help"))
    {
      std::cout << options_description;
    }
    else
    {    
      //todo
      std::size_t cpu_count = boost::thread::hardware_concurrency();
      std::size_t session_thread_count = cpu_count ? cpu_count : 2;
      std::size_t session_manager_thread_count = 1;      
    }
  }
  catch (const boost::program_options::error&)
  {
    exit_code = EXIT_FAILURE;
    std::cout << "Invalid options.\n" << options_description;      
  }  
  return exit_code;
}

void start_session(const session_proxy_ptr& session_proxy)
{
  session_proxy->session_->async_start
  (
    ma::make_custom_alloc_handler
    (
      session_proxy->start_wait_allocator_,
      boost::bind(session_started, session_proxy, _1)
    )
  );
  session_proxy->state_ = session_proxy_type::start_in_progress;
}

void wait_session(const session_proxy_ptr& session_proxy)
{
  session_proxy->session_->async_wait
  (
    ma::make_custom_alloc_handler
    (
      session_proxy->start_wait_allocator_,
      boost::bind(session_has_to_stop, session_proxy, _1)
    )
  );
}

void stop_session(const session_proxy_ptr& session_proxy)
{
  session_proxy->session_->async_stop
  (
    ma::make_custom_alloc_handler
    (
      session_proxy->stop_allocator_,
      boost::bind(session_stopped, session_proxy, _1)
    )
  );
  session_proxy->state_ = session_proxy_type::stop_in_progress;
}

void run_io_service(boost::asio::io_service& io_service, 
  exception_handler io_service_exception_handler)
{
  try 
  {
    io_service.run();
  }
  catch (...)
  {
    io_service_exception_handler();
  }
}

void handle_work_exception(const session_proxy_ptr& session_proxy)
{
  boost::unique_lock<boost::mutex> session_proxy_lock(session_proxy->mutex_);  
  session_proxy->state_ = session_proxy_type::stopped;  
  std::cout << "Terminating client work due to unexpected exception...\n";
  session_proxy_lock.unlock();
  session_proxy->state_changed_.notify_one();      
}

void handle_program_exit(const session_proxy_ptr& session_proxy)
{
  boost::unique_lock<boost::mutex> session_proxy_lock(session_proxy->mutex_);
  std::cout << "Program exit request detected.\n";
  if (session_proxy_type::stopped == session_proxy->state_)
  {    
    std::cout << "Client has already stopped.\n";
  }
  else if (session_proxy_type::stop_in_progress == session_proxy->state_)
  {    
    session_proxy->state_ = session_proxy_type::stopped;
    std::cout << "Client is already stopping. Terminating client work...\n";
    session_proxy_lock.unlock();
    session_proxy->state_changed_.notify_one();       
  }
  else
  {    
    // Start session stop
    stop_session(session_proxy);
    session_proxy->stopped_by_program_exit_ = true;
    std::cout << "Client is stopping. Press Ctrl+C (Ctrl+Break) to terminate client work...\n";
    session_proxy_lock.unlock();    
    session_proxy->state_changed_.notify_one();
  }  
}

void session_started(const session_proxy_ptr& session_proxy,
  const boost::system::error_code& error)
{   
  boost::unique_lock<boost::mutex> session_proxy_lock(session_proxy->mutex_);
  if (session_proxy_type::start_in_progress == session_proxy->state_)
  {   
    if (error)
    {       
      session_proxy->state_ = session_proxy_type::stopped;
      std::cout << "Client can't start due to error.\n";
      session_proxy_lock.unlock();
      session_proxy->state_changed_.notify_one();
    }
    else
    {
      session_proxy->state_ = session_proxy_type::started;
      wait_session(session_proxy);
      std::cout << "Client has started.\n";
    }
  }
}

void session_has_to_stop(const session_proxy_ptr& session_proxy,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> session_proxy_lock(session_proxy->mutex_);
  if (session_proxy_type::started == session_proxy->state_)
  {
    stop_session(session_proxy);
    std::cout << "Client can't continue work due to error. Client is stopping...\n";
    session_proxy_lock.unlock();
    session_proxy->state_changed_.notify_one();    
  }
}

void session_stopped(const session_proxy_ptr& session_proxy,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> session_proxy_lock(session_proxy->mutex_);
  if (session_proxy_type::stop_in_progress == session_proxy->state_)
  {      
    session_proxy->state_ = session_proxy_type::stopped;
    std::cout << "Client has stopped.\n";    
    session_proxy_lock.unlock();
    session_proxy->state_changed_.notify_one();
  }
}