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
#include <ma/echo/server/session_manager.hpp>
#include <console_controller.hpp>

const char *help_param = "help"; 
const char *port_param = "port";
const char *session_threads_param   = "session_threads";
const char *stop_timeout_param      = "stop_timeout";
const char *max_sessions_param      = "max_sessions";
const char *recycled_sessions_param = "recycled_sessions";
const char *listen_backlog_param    = "listen_backlog";
const char *buffer_size_param       = "buffer_size";

struct session_manager_data;
typedef boost::shared_ptr<session_manager_data> session_manager_data_ptr;
typedef boost::function<void (void)> exception_handler;

struct session_manager_data : private boost::noncopyable
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
  ma::echo::server::session_manager_ptr session_manager_;
  state_type state_;
  bool stopped_by_program_exit_;
  boost::condition_variable state_changed_;  
  ma::handler_allocator<256> start_wait_allocator_;
  ma::handler_allocator<256> stop_allocator_;

  explicit session_manager_data(boost::asio::io_service& io_service,
    boost::asio::io_service& session_io_service,
    const ma::echo::server::session_manager::settings& settings)
    : session_manager_(new ma::echo::server::session_manager(io_service, session_io_service, settings))    
    , state_(ready_to_start)
    , stopped_by_program_exit_(false)
  {
  }

  ~session_manager_data()
  {
  }
}; // session_manager_data

void fill_options_description(boost::program_options::options_description&);

void start_session_manager(const session_manager_data_ptr&);

void wait_session_manager(const session_manager_data_ptr&);

void stop_session_manager(const session_manager_data_ptr&);

void run_io_service(boost::asio::io_service&, exception_handler);

void handle_work_exception(const session_manager_data_ptr&);

void handle_program_exit(const session_manager_data_ptr&);

void handle_session_manager_start(const session_manager_data_ptr&,
  const boost::system::error_code&);

void handle_session_manager_wait(const session_manager_data_ptr&,
  const boost::system::error_code&);

void handle_session_manager_stop(const session_manager_data_ptr&,
  const boost::system::error_code&);

int _tmain(int argc, _TCHAR* argv[])
{   
  int exit_code = EXIT_SUCCESS;
  try 
  {
    boost::program_options::options_description options_description("Allowed options");
    fill_options_description(options_description);

    boost::program_options::variables_map options_values;  
    boost::program_options::store(
      boost::program_options::parse_command_line(argc, argv, options_description), options_values);
    boost::program_options::notify(options_values);

    if (options_values.count(help_param))
    {
      std::cout << options_description;
    }
    else if (!options_values.count(port_param))
    {    
      exit_code = EXIT_FAILURE;
      std::cout << "Port must be specified.\n" << options_description;
    }
    else
    {        
      std::size_t cpu_count = boost::thread::hardware_concurrency();
      std::size_t session_thread_count = options_values[session_threads_param].as<std::size_t>();
      if (!session_thread_count) 
      {
        session_thread_count = cpu_count ? cpu_count : 1;
      }
      std::size_t session_manager_thread_count = 1;

      unsigned short listen_port = options_values[port_param].as<unsigned short>();      
      boost::posix_time::time_duration stop_timeout = 
        boost::posix_time::seconds(options_values[stop_timeout_param].as<long>());

      ma::echo::server::session_manager::settings server_settings(
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), listen_port),
        options_values[max_sessions_param].as<std::size_t>(),
        options_values[recycled_sessions_param].as<std::size_t>(),
        options_values[listen_backlog_param].as<int>(),
        ma::echo::server::session::settings(options_values[buffer_size_param].as<std::size_t>()));      

      std::cout << "Number of found CPU(s)             : " << cpu_count << '\n'               
                << "Number of session manager's threads: " << session_manager_thread_count << '\n'
                << "Number of sessions' threads        : " << session_thread_count << '\n' 
                << "Total number of work threads       : " << session_thread_count + session_manager_thread_count << '\n'
                << "Server listen port                 : " << listen_port << '\n'
                << "Server stop timeout (seconds)      : " << stop_timeout.total_seconds() << '\n'
                << "Maximum number of active sessions  : " << server_settings.max_sessions_ << '\n'
                << "Maximum number of recycled sessions: " << server_settings.recycled_sessions_ << '\n'
                << "TCP listen backlog size            : " << server_settings.listen_backlog_ << '\n'
                << "Size of session's buffer (bytes)   : " << server_settings.session_settings_.buffer_size_ << '\n';
      
      // Before session_manager_io_service
      boost::asio::io_service session_io_service;
      // ... for the right destruction order
      boost::asio::io_service session_manager_io_service;

      // Create session_manager
      session_manager_data_ptr main_session_manager_data(
        new session_manager_data(session_manager_io_service, session_io_service, server_settings));
      
      std::cout << "Server is starting...\n";
      
      // Start the server
      boost::unique_lock<boost::mutex> session_manager_data_lock(main_session_manager_data->mutex_);    
      start_session_manager(main_session_manager_data);
      session_manager_data_lock.unlock();
      
      // Setup console controller
      ma::console_controller console_controller(
        boost::bind(handle_program_exit, main_session_manager_data));

      std::cout << "Press Ctrl+C (Ctrl+Break) to exit.\n";

      // Exception handler for automatic server aborting
      exception_handler work_exception_handler(
        boost::bind(handle_work_exception, main_session_manager_data));

      // Create work for sessions' io_service to prevent threads' stop
      boost::asio::io_service::work session_work(session_io_service);
      // Create work for session_manager's io_service to prevent threads' stop
      boost::asio::io_service::work session_manager_work(session_manager_io_service);    

      // Create work threads for session operations
      boost::thread_group server_work_threads;    
      for (std::size_t i = 0; i != session_thread_count; ++i)
      {
        server_work_threads.create_thread(
          boost::bind(run_io_service, boost::ref(session_io_service), work_exception_handler));
      }
      // Create work threads for session manager operations
      for (std::size_t i = 0; i != session_manager_thread_count; ++i)
      {
        server_work_threads.create_thread(
          boost::bind(run_io_service, boost::ref(session_manager_io_service), work_exception_handler));      
      }

      // Wait until server stops
      session_manager_data_lock.lock();
      while (session_manager_data::stop_in_progress != main_session_manager_data->state_ 
        && session_manager_data::stopped != main_session_manager_data->state_)
      {
        main_session_manager_data->state_changed_.wait(session_manager_data_lock);
      }
      if (session_manager_data::stopped != main_session_manager_data->state_)
      {
        if (!main_session_manager_data->state_changed_.timed_wait(session_manager_data_lock, stop_timeout))      
        {
          std::cout << "Server stop timeout expiration. Terminating server work...\n";
          exit_code = EXIT_FAILURE;
        }
        main_session_manager_data->state_ = session_manager_data::stopped;
      }
      if (!main_session_manager_data->stopped_by_program_exit_)
      {
        exit_code = EXIT_FAILURE;
      }
      session_manager_data_lock.unlock();
          
      session_manager_io_service.stop();
      session_io_service.stop();

      std::cout << "Server work was terminated. Waiting until all of the work threads will stop...\n";
      server_work_threads.join_all();
      std::cout << "Work threads have stopped. Process will close.\n";    
    }
  }
  catch (const boost::program_options::error& e)
  {
    exit_code = EXIT_FAILURE;
    std::cout << "Error reading options: " << e.what();      
  }
  catch (const std::exception& e)
  {
    exit_code = EXIT_FAILURE;
    std::cout << "Unexpected exception: " << e.what();      
  }
  return exit_code;
}

void fill_options_description(boost::program_options::options_description& options_description)
{
  options_description.add_options()
    (
      help_param, 
      "produce help message"
    )
    (
      port_param, 
      boost::program_options::value<unsigned short>(), 
      "set TCP port number for to listen for incoming connections"
    )
    (
      session_threads_param, 
      boost::program_options::value<std::size_t>()->default_value(0), 
      "set the number of sessions' threads, zero means set it equal to the number of CPUs"
    )    
    (
      stop_timeout_param, 
      boost::program_options::value<long>()->default_value(60), 
      "set server stop timeout, at one's expiration server work will be terminated (seconds)"
    )
    (
      max_sessions_param, 
      boost::program_options::value<std::size_t>()->default_value(10000), 
      "set maximum number of simultaneously active sessions"
    )
    (
      recycled_sessions_param, 
      boost::program_options::value<std::size_t>()->default_value(100), 
      "set maximum number of inactive sessions used for new sessions' acceptation"
    )
    (
      listen_backlog_param, 
      boost::program_options::value<int>()->default_value(6), 
      "set size of TCP listen backlog"
    )
    (
      buffer_size_param, 
      boost::program_options::value<std::size_t>()->default_value(1024),
      "set size of the session's buffer (bytes)"
    );
}

void start_session_manager(const session_manager_data_ptr& ready_session_manager_data)
{
  ready_session_manager_data->session_manager_->async_start
  (
    ma::make_custom_alloc_handler
    (
      ready_session_manager_data->start_wait_allocator_,
      boost::bind(handle_session_manager_start, ready_session_manager_data, _1)
    )
  );
  ready_session_manager_data->state_ = session_manager_data::start_in_progress;
}

void wait_session_manager(const session_manager_data_ptr& started_session_manager_data)
{
  started_session_manager_data->session_manager_->async_wait
  (
    ma::make_custom_alloc_handler
    (
      started_session_manager_data->start_wait_allocator_,
      boost::bind(handle_session_manager_wait, started_session_manager_data, _1)
    )
  );
}

void stop_session_manager(const session_manager_data_ptr& waited_session_manager_data)
{
  waited_session_manager_data->session_manager_->async_stop
  (
    ma::make_custom_alloc_handler
    (
      waited_session_manager_data->stop_allocator_,
      boost::bind(handle_session_manager_stop, waited_session_manager_data, _1)
    )
  );
  waited_session_manager_data->state_ = session_manager_data::stop_in_progress;
}

void run_io_service(boost::asio::io_service& io_service, exception_handler 
  io_service_exception_handler)
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

void handle_work_exception(const session_manager_data_ptr& current_session_manager_data)
{
  boost::unique_lock<boost::mutex> server_data_lock(current_session_manager_data->mutex_);  
  current_session_manager_data->state_ = session_manager_data::stopped;  
  std::cout << "Terminating server work due to unexpected exception...\n";
  server_data_lock.unlock();
  current_session_manager_data->state_changed_.notify_one();      
}

void handle_program_exit(const session_manager_data_ptr& current_session_manager_data)
{
  boost::unique_lock<boost::mutex> server_data_lock(current_session_manager_data->mutex_);
  std::cout << "Program exit request detected.\n";
  if (session_manager_data::stopped == current_session_manager_data->state_)
  {    
    std::cout << "Server has already stopped.\n";
  }
  else if (session_manager_data::stop_in_progress == current_session_manager_data->state_)
  {    
    current_session_manager_data->state_ = session_manager_data::stopped;
    std::cout << "Server is already stopping. Terminating server work...\n";
    server_data_lock.unlock();
    current_session_manager_data->state_changed_.notify_one();       
  }
  else
  {    
    // Start server stop
    stop_session_manager(current_session_manager_data);
    current_session_manager_data->stopped_by_program_exit_ = true;
    std::cout << "Server is stopping. Press Ctrl+C (Ctrl+Break) to terminate server work...\n";
    server_data_lock.unlock();    
    current_session_manager_data->state_changed_.notify_one();
  }  
}

void handle_session_manager_start(const session_manager_data_ptr& started_session_manager_data,
  const boost::system::error_code& error)
{   
  boost::unique_lock<boost::mutex> server_data_lock(started_session_manager_data->mutex_);
  if (session_manager_data::start_in_progress == started_session_manager_data->state_)
  {   
    if (error)
    {       
      started_session_manager_data->state_ = session_manager_data::stopped;
      std::cout << "Server can't start due to error.\n";
      server_data_lock.unlock();
      started_session_manager_data->state_changed_.notify_one();
    }
    else
    {
      started_session_manager_data->state_ = session_manager_data::started;
      wait_session_manager(started_session_manager_data);
      std::cout << "Server has started.\n";
    }
  }
}

void handle_session_manager_wait(const session_manager_data_ptr& waited_session_manager_data,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> server_data_lock(waited_session_manager_data->mutex_);
  if (session_manager_data::started == waited_session_manager_data->state_)
  {
    stop_session_manager(waited_session_manager_data);
    std::cout << "Server can't continue work due to error. Server is stopping...\n";
    server_data_lock.unlock();
    waited_session_manager_data->state_changed_.notify_one();    
  }
}

void handle_session_manager_stop(const session_manager_data_ptr& stopped_session_manager_data,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> server_data_lock(stopped_session_manager_data->mutex_);
  if (session_manager_data::stop_in_progress == stopped_session_manager_data->state_)
  {      
    stopped_session_manager_data->state_ = session_manager_data::stopped;
    std::cout << "Server has stopped.\n";    
    server_data_lock.unlock();
    stopped_session_manager_data->state_changed_.notify_one();
  }
}