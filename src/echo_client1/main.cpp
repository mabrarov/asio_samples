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
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/echo/client1/session.hpp>
#include <console_controller.hpp>

struct  session_data;
typedef boost::shared_ptr<session_data> session_data_ptr;
typedef boost::function<void (void)>    exception_handler;

struct session_data : private boost::noncopyable
{  
  enum state
  {
    ready_to_start,
    start_in_progress,
    started,
    stop_in_progress,
    stopped
  };

  boost::mutex mutex_;  
  ma::echo::client1::session_ptr session_;
  state state_;
  bool stopped_by_program_exit_;
  boost::condition_variable state_changed_;  
  ma::in_place_handler_allocator<256> start_wait_allocator_;
  ma::in_place_handler_allocator<256> stop_allocator_;

  explicit session_data(boost::asio::io_service& io_service,    
    const ma::echo::client1::session::settings& settings)
    : session_(new ma::echo::client1::session(io_service, settings))    
    , state_(ready_to_start)
    , stopped_by_program_exit_(false)
  {
  }

  ~session_data()
  {
  }
}; // session_data

void start_session(const session_data_ptr&);

void wait_session(const session_data_ptr&);

void stop_session(const session_data_ptr&);

void run_io_service(boost::asio::io_service&, exception_handler);

void handle_work_exception(const session_data_ptr&);

void handle_program_exit(const session_data_ptr&);

void handle_session_start(const session_data_ptr&,
  const boost::system::error_code&);

void handle_session_wait(const session_data_ptr&,
  const boost::system::error_code&);

void handle_session_stop(const session_data_ptr&,
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
      std::size_t cpu_count = boost::thread::hardware_concurrency();
      std::size_t session_thread_count = cpu_count > 1 ? cpu_count : 2;
    }
  }
  catch (const boost::program_options::error&)
  {
    exit_code = EXIT_FAILURE;
    std::cout << "Invalid options.\n" << options_description;      
  }  
  return exit_code;
}

void start_session(const session_data_ptr& ready_session_data)
{
  ready_session_data->session_->async_start
  (
    ma::make_custom_alloc_handler
    (
      ready_session_data->start_wait_allocator_,
      boost::bind(handle_session_start, ready_session_data, _1)
    )
  );
  ready_session_data->state_ = session_data::start_in_progress;
}

void wait_session(const session_data_ptr& started_session_data)
{
  started_session_data->session_->async_wait
  (
    ma::make_custom_alloc_handler
    (
      started_session_data->start_wait_allocator_,
      boost::bind(handle_session_wait, started_session_data, _1)
    )
  );
}

void stop_session(const session_data_ptr& started_session_data)
{
  started_session_data->session_->async_stop
  (
    ma::make_custom_alloc_handler
    (
      started_session_data->stop_allocator_,
      boost::bind(handle_session_stop, started_session_data, _1)
    )
  );
  started_session_data->state_ = session_data::stop_in_progress;
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

void handle_work_exception(const session_data_ptr& working_session_data)
{
  boost::unique_lock<boost::mutex> session_proxy_lock(working_session_data->mutex_);  
  working_session_data->state_ = session_data::stopped;  
  std::cout << "Terminating client work due to unexpected exception...\n";
  session_proxy_lock.unlock();
  working_session_data->state_changed_.notify_one();      
}

void handle_program_exit(const session_data_ptr& working_session_data)
{
  boost::unique_lock<boost::mutex> session_proxy_lock(working_session_data->mutex_);
  std::cout << "Program exit request detected.\n";
  if (session_data::stopped == working_session_data->state_)
  {    
    std::cout << "Client has already stopped.\n";
  }
  else if (session_data::stop_in_progress == working_session_data->state_)
  {    
    working_session_data->state_ = session_data::stopped;
    std::cout << "Client is already stopping. Terminating client work...\n";
    session_proxy_lock.unlock();
    working_session_data->state_changed_.notify_one();       
  }
  else
  {    
    // Start session stop
    stop_session(working_session_data);
    working_session_data->stopped_by_program_exit_ = true;
    std::cout << "Client is stopping. Press Ctrl+C (Ctrl+Break) to terminate client work...\n";
    session_proxy_lock.unlock();    
    working_session_data->state_changed_.notify_one();
  }  
}

void handle_session_start(const session_data_ptr& started_session_data,
  const boost::system::error_code& error)
{   
  boost::unique_lock<boost::mutex> session_proxy_lock(started_session_data->mutex_);
  if (session_data::start_in_progress == started_session_data->state_)
  {   
    if (error)
    {       
      started_session_data->state_ = session_data::stopped;
      std::cout << "Client can't start due to error.\n";
      session_proxy_lock.unlock();
      started_session_data->state_changed_.notify_one();
    }
    else
    {
      started_session_data->state_ = session_data::started;
      wait_session(started_session_data);
      std::cout << "Client has started.\n";
    }
  }
}

void handle_session_wait(const session_data_ptr& waited_session_data,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> session_proxy_lock(waited_session_data->mutex_);
  if (session_data::started == waited_session_data->state_)
  {
    stop_session(waited_session_data);
    std::cout << "Client can't continue work due to error. Client is stopping...\n";
    session_proxy_lock.unlock();
    waited_session_data->state_changed_.notify_one();    
  }
}

void handle_session_stop(const session_data_ptr& stopped_session_data,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> session_proxy_lock(stopped_session_data->mutex_);
  if (session_data::stop_in_progress == stopped_session_data->state_)
  {      
    stopped_session_data->state_ = session_data::stopped;
    std::cout << "Client has stopped.\n";    
    session_proxy_lock.unlock();
    stopped_session_data->state_changed_.notify_one();
  }
}